#pragma once

#include <thread>
#include <atomic>
#include <string>
#include <functional>
#include <memory>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <chrono>

namespace Surge {
namespace TCPControl {

class TCPController {
public:
    TCPController();
    ~TCPController();
    
    // Initialize the controller
    void initialize();
    void shutdown();
    
    // Callbacks for handling router commands
    using PresetCallback = std::function<bool(const std::string&)>;
    using ParamCallback = std::function<bool(int, float)>;
    using ListCallback = std::function<std::vector<std::string>()>;
    using GetParamsCallback = std::function<std::string()>;
    
    void setPresetLoadCallback(PresetCallback cb) { presetLoadCb = cb; }
    void setParamSetCallback(ParamCallback cb) { paramSetCb = cb; }
    void setPresetListCallback(ListCallback cb) { presetListCb = cb; }
    void setGetParamsCallback(GetParamsCallback cb) { getParamsCb = cb; }
    
    // Send responses back to router
    void sendResponse(const std::string& requestId, bool success, const std::string& data = "");
    
    // Get plugin signature parts for VST parameter exposure
    uint32_t getSignaturePart(int part) const;
    
private:
    // Connection management
    void connectToRouter();
    void connectionLoop();
    void reconnectWithBackoff();
    bool tryConnectIPC();
    bool tryConnectTCP();
    void closeConnection();
    
    // Message handling
    void handleMessage(const std::string& message);
    void processRouterCommand(const std::string& jsonMsg);
    void sendHeartbeat();
    void sendRegistration();
    std::string createJsonResponse(bool ok, const std::string& requestId, const std::string& data = "");
    
    // Worker thread
    std::thread workerThread;
    std::atomic<bool> running{false};
    std::atomic<bool> connected{false};
    
    // Socket/connection
    int socketFd{-1};
    std::mutex socketMutex;
    
    // Plugin signature (stable 128-bit identifier)
    std::string pluginSig;
    uint32_t instanceId{0};  // Stable instance counter
    std::string generatePluginSignature();
    
    // Reconnection backoff
    int reconnectDelay{500}; // milliseconds
    static constexpr int MAX_RECONNECT_DELAY = 5000; // 5 seconds
    
    // Heartbeat timing
    std::chrono::steady_clock::time_point lastHeartbeat;
    static constexpr int HEARTBEAT_INTERVAL_MS = 5000; // 5 seconds
    
    // Message queue for outgoing messages
    std::queue<std::string> outgoingQueue;
    std::mutex queueMutex;
    std::condition_variable queueCV;
    
    // Callbacks
    PresetCallback presetLoadCb;
    ParamCallback paramSetCb;
    ListCallback presetListCb;
    GetParamsCallback getParamsCb;
    
    // Platform-specific helpers
    std::string getHomeDirectory();
    bool fileExists(const std::string& path);
    std::string readJsonFile(const std::string& path);
    
    // Signature parts for parameter reflection (optional)
    void updateSignatureParameters();
    static constexpr int SIGNATURE_PARAM_COUNT = 4;
};

} // namespace TCPControl
} // namespace Surge