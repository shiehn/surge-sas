#include "SurgeTCPController.h"
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <sstream>
#include <vector>
#include <errno.h>
#include <cstdio>
#include <fstream>
#include <pwd.h>
#include <sys/stat.h>
#include <iostream>
#include <random>
#include <iomanip>
#include <atomic>

// Simple JSON parsing/generation (could use a library like nlohmann/json in production)
#include <regex>

namespace Surge {
namespace TCPControl {

// Global instance counter for stable plugin signatures
static std::atomic<uint32_t> g_instanceCounter{0};

// Generate a UUID v4
static std::string generateUUID() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(0, 15);
    static std::uniform_int_distribution<> dis2(8, 11);
    
    std::stringstream ss;
    int i;
    ss << std::hex;
    for (i = 0; i < 8; i++) ss << dis(gen);
    ss << "-";
    for (i = 0; i < 4; i++) ss << dis(gen);
    ss << "-4";  // Version 4 UUID
    for (i = 0; i < 3; i++) ss << dis(gen);
    ss << "-";
    ss << dis2(gen);
    for (i = 0; i < 3; i++) ss << dis(gen);
    ss << "-";
    for (i = 0; i < 12; i++) ss << dis(gen);
    return ss.str();
}

TCPController::TCPController() {
    // Generate stable plugin signature using instance counter
    // This ensures each instance gets a unique but stable ID
    instanceId = g_instanceCounter.fetch_add(1);
    pluginSig = generatePluginSignature();
    
    // Log for debugging
    FILE* log = fopen("/tmp/surge-router.log", "a");
    if (log) {
        fprintf(log, "[Instance %u] Created TCPController with plugin_sig: %s\n", instanceId, pluginSig.c_str());
        fclose(log);
    }
}

std::string TCPController::generatePluginSignature() {
    // Create a stable signature based on:
    // 1. Process ID (stable for this REAPER session)
    // 2. Instance counter (unique per instance)
    // 3. Fixed seed for deterministic generation
    
    uint32_t pid = static_cast<uint32_t>(getpid());
    uint32_t instance = instanceId;
    
    // Use PID and instance as seed for deterministic generation
    std::mt19937_64 gen(((uint64_t)pid << 32) | instance);
    std::uniform_int_distribution<uint64_t> dist;
    
    uint64_t hi = dist(gen);
    uint64_t lo = dist(gen);
    
    std::stringstream ss;
    ss << std::hex << std::setfill('0')
       << std::setw(8) << pid
       << "-"
       << std::setw(4) << instance
       << "-"
       << std::setw(16) << (hi & 0xFFFFFFFFFFFF);
    return ss.str();
}

TCPController::~TCPController() {
    shutdown();
}

void TCPController::initialize() {
    if (running) return;
    
    running = true;
    workerThread = std::thread(&TCPController::connectionLoop, this);
}

void TCPController::shutdown() {
    if (!running) return;
    
    // Log shutdown
    FILE* log = fopen("/tmp/surge-router.log", "a");
    if (log) {
        fprintf(log, "[Instance %u] Shutting down TCP controller\n", instanceId);
        fclose(log);
    }
    
    // Signal thread to stop
    running = false;
    
    // Send disconnect message if connected
    if (connected && socketFd >= 0) {
        std::string msg = "{\"type\":\"disconnect\","
                          "\"plugin_sig\":\"" + pluginSig + "\"}\n";
        std::lock_guard<std::mutex> sockLock(socketMutex);
        write(socketFd, msg.c_str(), msg.length());
    }
    
    // Wake up any waiting threads
    queueCV.notify_all();
    
    // Close the connection
    closeConnection();
    
    // Wait for worker thread to finish
    if (workerThread.joinable()) {
        workerThread.join();
    }
    
    // Final log
    log = fopen("/tmp/surge-router.log", "a");
    if (log) {
        fprintf(log, "[Instance %u] TCP controller shutdown complete\n", instanceId);
        fclose(log);
    }
}

// Removed setInstanceId - no longer needed with plugin_sig approach

void TCPController::connectionLoop() {
    while (running) {
        if (!connected) {
            connectToRouter();
            if (!connected) {
                reconnectWithBackoff();
                continue;
            }
        }
        
        // Check for heartbeat
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastHeartbeat).count();
        if (elapsed >= HEARTBEAT_INTERVAL_MS) {
            sendHeartbeat();
            lastHeartbeat = now;
        }
        
        // Check for connection timeout (no data received for 30 seconds)
        static auto lastReceived = std::chrono::steady_clock::now();
        auto timeSinceReceived = std::chrono::duration_cast<std::chrono::seconds>(now - lastReceived).count();
        if (timeSinceReceived > 30) {
            FILE* log = fopen("/tmp/surge-router.log", "a");
            if (log) {
                fprintf(log, "[Instance %u] Connection timeout - no data for %lld seconds\n", 
                        instanceId, (long long)timeSinceReceived);
                fclose(log);
            }
            closeConnection();
            lastReceived = now;  // Reset to avoid immediate reconnect loop
            continue;
        }
        
        // Read incoming messages
        char buffer[4096];
        fd_set readSet;
        FD_ZERO(&readSet);
        FD_SET(socketFd, &readSet);
        
        struct timeval timeout;
        timeout.tv_sec = 0;
        timeout.tv_usec = 100000; // 100ms
        
        int result = select(socketFd + 1, &readSet, nullptr, nullptr, &timeout);
        if (result > 0) {
            int bytesRead = read(socketFd, buffer, sizeof(buffer) - 1);
            if (bytesRead > 0) {
                buffer[bytesRead] = '\0';
                lastReceived = std::chrono::steady_clock::now();  // Update last received time
                
                // Process JSON lines (one JSON object per line)
                std::stringstream ss(buffer);
                std::string line;
                while (std::getline(ss, line)) {
                    if (!line.empty()) {
                        handleMessage(line);
                    }
                }
            } else if (bytesRead == 0 || (bytesRead < 0 && errno != EAGAIN)) {
                // Connection closed or error
                closeConnection();
            }
        }
        
        // Send any queued outgoing messages
        std::unique_lock<std::mutex> lock(queueMutex);
        while (!outgoingQueue.empty() && connected) {
            std::string msg = outgoingQueue.front();
            outgoingQueue.pop();
            lock.unlock();
            
            std::lock_guard<std::mutex> sockLock(socketMutex);
            if (socketFd >= 0) {
                write(socketFd, msg.c_str(), msg.length());
            }
            
            lock.lock();
        }
    }
}

void TCPController::connectToRouter() {
    // Log connection attempt
    static int attemptCount = 0;
    attemptCount++;
    
    // Log every 10th attempt to avoid spam
    if (attemptCount == 1 || attemptCount % 10 == 0) {
        FILE* log = fopen("/tmp/surge-router.log", "a");
        if (log) {
            fprintf(log, "[Instance %u] Connection attempt #%d to broker...\n", instanceId, attemptCount);
            fclose(log);
        }
    }
    
    // Try IPC first, then TCP fallback
    if (tryConnectIPC()) {
        connected = true;
        sendRegistration();
        lastHeartbeat = std::chrono::steady_clock::now();
        reconnectDelay = 500; // Reset backoff
        
        FILE* log = fopen("/tmp/surge-router.log", "a");
        if (log) {
            fprintf(log, "[Instance %u] ✅ Connected via IPC after %d attempts\n", instanceId, attemptCount);
            fclose(log);
        }
        attemptCount = 0; // Reset counter on success
    } else if (tryConnectTCP()) {
        connected = true;
        sendRegistration();
        lastHeartbeat = std::chrono::steady_clock::now();
        reconnectDelay = 500; // Reset backoff
        
        FILE* log = fopen("/tmp/surge-router.log", "a");
        if (log) {
            fprintf(log, "[Instance %u] ✅ Connected via TCP after %d attempts\n", instanceId, attemptCount);
            fclose(log);
        }
        attemptCount = 0; // Reset counter on success
    }
}

bool TCPController::tryConnectIPC() {
#ifdef _WIN32
    // Windows Named Pipe - not implemented in this example
    return false;
#else
    // Unix Domain Socket - Try multiple locations
    std::string socketPath = "/tmp/sas-plugin-router.sock";  // Primary location
    
    // If not found, try home directory
    if (!fileExists(socketPath)) {
        socketPath = getHomeDirectory() + "/.sas/router.sock";
    }
    
    if (!fileExists(socketPath)) {
        return false;
    }
    
    int sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock < 0) {
        return false;
    }
    
    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, socketPath.c_str(), sizeof(addr.sun_path) - 1);
    
    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        close(sock);
        return false;
    }
    
    std::lock_guard<std::mutex> lock(socketMutex);
    socketFd = sock;
    return true;
#endif
}

bool TCPController::tryConnectTCP() {
    std::string host = "127.0.0.1";
    int port = 7833;  // SAS Assistant default port
    
    // Check for discovery file
    std::string discoveryPath = getHomeDirectory() + "/.sas/router.json";
    if (fileExists(discoveryPath)) {
        std::string json = readJsonFile(discoveryPath);
        
        // Simple JSON parsing for port and host
        std::regex portRegex("\"port\"\\s*:\\s*(\\d+)");
        std::regex hostRegex("\"host\"\\s*:\\s*\"([^\"]+)\"");
        std::smatch match;
        
        if (std::regex_search(json, match, portRegex)) {
            port = std::stoi(match[1]);
        }
        if (std::regex_search(json, match, hostRegex)) {
            host = match[1];
        }
    }
    
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        return false;
    }
    
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, host.c_str(), &addr.sin_addr);
    
    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        close(sock);
        return false;
    }
    
    std::lock_guard<std::mutex> lock(socketMutex);
    socketFd = sock;
    return true;
}

void TCPController::closeConnection() {
    connected = false;
    std::lock_guard<std::mutex> lock(socketMutex);
    if (socketFd >= 0) {
#ifdef _WIN32
        closesocket(socketFd);
#else
        // Shutdown the socket properly before closing
        ::shutdown(socketFd, SHUT_RDWR);
        close(socketFd);
#endif
        socketFd = -1;
    }
}

void TCPController::reconnectWithBackoff() {
    // Log backoff info occasionally
    static int backoffCount = 0;
    if (++backoffCount % 5 == 1) {  // Log every 5th backoff
        FILE* log = fopen("/tmp/surge-router.log", "a");
        if (log) {
            fprintf(log, "[Instance %u] Waiting %dms before retry (broker not available)\n", instanceId, reconnectDelay);
            fclose(log);
        }
    }
    
    std::this_thread::sleep_for(std::chrono::milliseconds(reconnectDelay));
    reconnectDelay = std::min(reconnectDelay * 2, MAX_RECONNECT_DELAY);
}

void TCPController::sendRegistration() {
    std::string msg = "{\"type\":\"register\","
                      "\"plugin_sig\":\"" + pluginSig + "\","
                      "\"plugin_version\":\"1.31.0\","
                      "\"plugin_type\":\"surge-xt-sas\","  // Unique identifier for this fork
                      "\"plugin_name\":\"Surge XT [S&S Fork v1.31.0 Instance " + std::to_string(instanceId) + "]\","
                      "\"instance_id\":" + std::to_string(instanceId) + ",";
    
    // Add routing field if PIID is available (parse PIID format: project/track/fx)
    if (!piid.empty()) {
        size_t pos1 = piid.find('/');
        size_t pos2 = piid.find('/', pos1 + 1);
        
        if (pos1 != std::string::npos && pos2 != std::string::npos) {
            std::string projectGuid = piid.substr(0, pos1);
            std::string trackGuid = piid.substr(pos1 + 1, pos2 - pos1 - 1);
            std::string fxGuid = piid.substr(pos2 + 1);
            
            msg += "\"routing\":{"
                   "\"project_guid\":\"" + projectGuid + "\","
                   "\"track_guid\":\"" + trackGuid + "\","
                   "\"fx_guid\":\"" + fxGuid + "\""
                   "},";
        }
    }
    
    msg += "\"build\":\"vst3\"}\n";
    
    // Debug logging
    FILE* log = fopen("/tmp/surge-router.log", "a");
    if (log) {
        fprintf(log, "[Instance %u] Sending registration with plugin_sig: %s\n", instanceId, pluginSig.c_str());
        if (!piid.empty()) {
            fprintf(log, "[Instance %u] PIID routing: %s\n", instanceId, piid.c_str());
        }
        fprintf(log, "[Instance %u] Full message: %s", instanceId, msg.c_str());
        fclose(log);
    }
    
    std::lock_guard<std::mutex> sockLock(socketMutex);
    if (socketFd >= 0) {
        write(socketFd, msg.c_str(), msg.length());
    }
}

void TCPController::sendHeartbeat() {
    std::string msg = "{\"type\":\"heartbeat\",\"plugin_sig\":\"" + pluginSig + "\"}\n";
    
    // Debug logging only every 10th heartbeat to reduce log spam
    static int heartbeatCount = 0;
    if (++heartbeatCount % 10 == 0) {
        FILE* log = fopen("/tmp/surge-router.log", "a");
        if (log) {
            fprintf(log, "[Instance %u] Sent heartbeat #%d\n", instanceId, heartbeatCount);
            fclose(log);
        }
    }
    
    std::lock_guard<std::mutex> sockLock(socketMutex);
    if (socketFd >= 0) {
        ssize_t result = write(socketFd, msg.c_str(), msg.length());
        if (result < 0) {
            // Write failed, connection is broken
            FILE* log = fopen("/tmp/surge-router.log", "a");
            if (log) {
                fprintf(log, "[Instance %u] Heartbeat write failed: %s (errno=%d)\n", 
                        instanceId, strerror(errno), errno);
                fclose(log);
            }
            closeConnection();
        } else if (result != (ssize_t)msg.length()) {
            // Partial write, also indicates a problem
            FILE* log = fopen("/tmp/surge-router.log", "a");
            if (log) {
                fprintf(log, "[Instance %u] Heartbeat partial write: %zd/%zu bytes\n", 
                        instanceId, result, msg.length());
                fclose(log);
            }
            closeConnection();
        }
    }
}

void TCPController::handleMessage(const std::string& message) {
    // Log received message for debugging
    FILE* log = fopen("/tmp/surge-router.log", "a");
    if (log) {
        fprintf(log, "[Instance %u] Received: %s\n", instanceId, message.c_str());
        fclose(log);
    }
    
    // Simple JSON parsing - in production use a proper JSON library
    processRouterCommand(message);
}

void TCPController::processRouterCommand(const std::string& jsonMsg) {
    // Parse the JSON message
    std::regex opRegex("\"op\"\\s*:\\s*\"([^\"]+)\"");
    std::regex requestIdRegex("\"request_id\"\\s*:\\s*\"([^\"]+)\"");
    std::smatch match;
    
    std::string op;
    std::string requestId;
    
    if (std::regex_search(jsonMsg, match, opRegex)) {
        op = match[1];
    }
    if (std::regex_search(jsonMsg, match, requestIdRegex)) {
        requestId = match[1];
    }
    
    // Log what operation we're processing
    FILE* log = fopen("/tmp/surge-router.log", "a");
    if (log) {
        fprintf(log, "[Instance %u] Processing op='%s' requestId='%s'\n", instanceId, op.c_str(), requestId.c_str());
        fclose(log);
    }
    
    if (op == "set_preset" || op == "load_preset") {
        std::regex presetRegex("\"preset\"\\s*:\\s*\"([^\"]+)\"");
        if (std::regex_search(jsonMsg, match, presetRegex)) {
            std::string presetName = match[1];
            
            FILE* log = fopen("/tmp/surge-router.log", "a");
            if (log) {
                fprintf(log, "[Instance %u] Loading preset: '%s'\n", instanceId, presetName.c_str());
                if (presetLoadCb) {
                    fprintf(log, "[Instance %u] Preset callback is SET, calling it...\n", instanceId);
                } else {
                    fprintf(log, "[Instance %u] ERROR: Preset callback is NULL!\n", instanceId);
                }
                fclose(log);
            }
            
            if (presetLoadCb) {
                bool success = presetLoadCb(presetName);
                
                FILE* log2 = fopen("/tmp/surge-router.log", "a");
                if (log2) {
                    fprintf(log2, "[Instance %u] Preset callback returned: %s\n", instanceId, success ? "SUCCESS" : "FAILURE");
                    fclose(log2);
                }
                
                sendResponse(requestId, success, success ? "{\"preset\":\"" + presetName + "\"}" : "{\"error\":\"Failed to load preset\"}");
            } else {
                sendResponse(requestId, false, "{\"error\":\"Preset callback not set\"}");
            }
        }
    }
    else if (op == "list_presets") {
        FILE* log = fopen("/tmp/surge-router.log", "a");
        if (log) {
            fprintf(log, "[Instance %u] Processing list_presets request (id: %s)\n", 
                    instanceId, requestId.c_str());
            fclose(log);
        }
        
        if (presetListCb) {
            auto presets = presetListCb();
            
            log = fopen("/tmp/surge-router.log", "a");
            if (log) {
                fprintf(log, "[Instance %u] Sending %zu preset names in response\n", 
                        instanceId, presets.size());
                fclose(log);
            }
            
            std::string data = "{\"presets\":[";
            for (size_t i = 0; i < presets.size(); ++i) {
                data += "\"" + presets[i] + "\"";
                if (i < presets.size() - 1) data += ",";
            }
            data += "]}";
            sendResponse(requestId, true, data);
        } else {
            sendResponse(requestId, false, "{\"error\":\"Preset list callback not set\"}");
        }
    }
    else if (op == "set_piid") {
        // Handle PIID assignment from routing system
        std::regex projectRegex("\"project_guid\"\\s*:\\s*\"([^\"]+)\"");
        std::regex trackRegex("\"track_guid\"\\s*:\\s*\"([^\"]+)\"");
        std::regex fxRegex("\"fx_guid\"\\s*:\\s*\"([^\"]+)\"");
        
        std::string projectGuid, trackGuid, fxGuid;
        
        if (std::regex_search(jsonMsg, match, projectRegex)) {
            projectGuid = match[1];
        }
        if (std::regex_search(jsonMsg, match, trackRegex)) {
            trackGuid = match[1];
        }
        if (std::regex_search(jsonMsg, match, fxRegex)) {
            fxGuid = match[1];
        }
        
        if (!projectGuid.empty() && !trackGuid.empty() && !fxGuid.empty()) {
            setPIID(projectGuid, trackGuid, fxGuid);
            sendResponse(requestId, true, "{\"message\":\"PIID set successfully\"}");
            
            // Re-send registration with updated PIID
            sendRegistration();
        } else {
            sendResponse(requestId, false, "{\"error\":\"Invalid PIID data\"}");
        }
    }
    else if (op == "set_param") {
        std::regex indexRegex("\"index\"\\s*:\\s*(\\d+)");
        std::regex valueRegex("\"value\"\\s*:\\s*([\\d.+-]+)");
        
        int index = -1;
        float value = 0.0f;
        
        if (std::regex_search(jsonMsg, match, indexRegex)) {
            index = std::stoi(match[1]);
        }
        if (std::regex_search(jsonMsg, match, valueRegex)) {
            value = std::stof(match[1]);
        }
        
        if (index >= 0 && paramSetCb) {
            bool success = paramSetCb(index, value);
            sendResponse(requestId, success, success ? "{\"index\":" + std::to_string(index) + ",\"value\":" + std::to_string(value) + "}" : "{\"error\":\"Failed to set parameter\"}");
        } else {
            sendResponse(requestId, false, "{\"error\":\"Invalid parameter or callback not set\"}");
        }
    }
    else if (op == "get_params") {
        if (getParamsCb) {
            std::string params = getParamsCb();
            sendResponse(requestId, true, params);
        } else {
            sendResponse(requestId, false, "{\"error\":\"Get params callback not set\"}");
        }
    }
}

void TCPController::sendResponse(const std::string& requestId, bool success, const std::string& data) {
    std::string response = createJsonResponse(success, requestId, data);
    
    // Log response being sent
    FILE* log = fopen("/tmp/surge-router.log", "a");
    if (log) {
        // Truncate long responses for logging
        std::string logResp = response;
        if (logResp.length() > 200) {
            logResp = logResp.substr(0, 200) + "...";
        }
        fprintf(log, "[Instance %u] Sending response: %s", instanceId, logResp.c_str());
        fclose(log);
    }
    
    std::lock_guard<std::mutex> lock(queueMutex);
    outgoingQueue.push(response);
    queueCV.notify_one();
}

std::string TCPController::createJsonResponse(bool ok, const std::string& requestId, const std::string& data) {
    // Include type:"response" for broker routing
    std::string response = "{\"type\":\"response\",\"ok\":" + std::string(ok ? "true" : "false");
    
    // Always include plugin_sig for broker routing
    response += ",\"plugin_sig\":\"" + pluginSig + "\"";
    
    if (!requestId.empty()) {
        response += ",\"request_id\":\"" + requestId + "\"";
    }
    if (!data.empty()) {
        if (data[0] == '{' || data[0] == '[') {
            // Already JSON formatted
            response += ",\"data\":" + data;
        } else {
            response += ",\"data\":\"" + data + "\"";
        }
    }
    response += "}\n";
    return response;
}

std::string TCPController::getHomeDirectory() {
    const char* home = getenv("HOME");
    if (home) {
        return std::string(home);
    }
    
    struct passwd* pw = getpwuid(getuid());
    if (pw) {
        return std::string(pw->pw_dir);
    }
    
    return "";
}

bool TCPController::fileExists(const std::string& path) {
    struct stat buffer;
    return (stat(path.c_str(), &buffer) == 0);
}

std::string TCPController::readJsonFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        return "";
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

uint32_t TCPController::getSignaturePart(int part) const {
    // Parse the plugin signature to extract 32-bit parts
    // Signature format: "PPPPPPPP-IIII-HHHHHHHHHHHH"
    // Part 0: PID (first 8 hex chars)
    // Part 1: Instance (4 hex chars)
    // Part 2: Hash high (first 8 of 12 hex chars)
    // Part 3: Hash low (last 4 of 12 hex chars + padding)
    
    if (pluginSig.empty() || part < 0 || part > 3) {
        return 0;
    }
    
    try {
        switch (part) {
            case 0: {
                // PID part (first 8 hex chars)
                std::string pidStr = pluginSig.substr(0, 8);
                return static_cast<uint32_t>(std::stoul(pidStr, nullptr, 16));
            }
            case 1: {
                // Instance part (4 hex chars after first dash)
                size_t dash1 = pluginSig.find('-');
                if (dash1 != std::string::npos && dash1 + 5 <= pluginSig.length()) {
                    std::string instStr = pluginSig.substr(dash1 + 1, 4);
                    return static_cast<uint32_t>(std::stoul(instStr, nullptr, 16));
                }
                break;
            }
            case 2: {
                // Hash high part (first 8 chars of hash)
                size_t dash2 = pluginSig.find('-', pluginSig.find('-') + 1);
                if (dash2 != std::string::npos && dash2 + 9 <= pluginSig.length()) {
                    std::string hashHi = pluginSig.substr(dash2 + 1, 8);
                    return static_cast<uint32_t>(std::stoul(hashHi, nullptr, 16));
                }
                break;
            }
            case 3: {
                // Hash low part (last 4 chars of hash)
                size_t dash2 = pluginSig.find('-', pluginSig.find('-') + 1);
                if (dash2 != std::string::npos && dash2 + 13 <= pluginSig.length()) {
                    std::string hashLo = pluginSig.substr(dash2 + 9, 4);
                    return static_cast<uint32_t>(std::stoul(hashLo, nullptr, 16));
                }
                break;
            }
        }
    } catch (...) {
        // Return 0 on any parse error
    }
    
    return 0;
}

void TCPController::setPIID(const std::string& projectGuid, const std::string& trackGuid, const std::string& fxGuid) {
    if (!projectGuid.empty() && !trackGuid.empty() && !fxGuid.empty()) {
        piid = projectGuid + "/" + trackGuid + "/" + fxGuid;
        
        // Log PIID update
        FILE* log = fopen("/tmp/surge-router.log", "a");
        if (log) {
            fprintf(log, "[Instance %u] PIID set: %s\n", instanceId, piid.c_str());
            fclose(log);
        }
    }
}

void TCPController::setPIIDFromString(const std::string& piidString) {
    if (!piidString.empty()) {
        // Parse PIID string format: projectGuid/trackGuid/fxGuid
        size_t pos1 = piidString.find('/');
        size_t pos2 = piidString.find('/', pos1 + 1);
        
        if (pos1 != std::string::npos && pos2 != std::string::npos) {
            std::string projectGuid = piidString.substr(0, pos1);
            std::string trackGuid = piidString.substr(pos1 + 1, pos2 - pos1 - 1);
            std::string fxGuid = piidString.substr(pos2 + 1);
            
            setPIID(projectGuid, trackGuid, fxGuid);
        } else {
            // If it's not in the expected format, store it directly
            piid = piidString;
            
            FILE* log = fopen("/tmp/surge-router.log", "a");
            if (log) {
                fprintf(log, "[Instance %u] PIID set from string: %s\n", instanceId, piid.c_str());
                fclose(log);
            }
        }
    }
}

} // namespace TCPControl
} // namespace Surge