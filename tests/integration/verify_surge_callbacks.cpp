/*
 * Test program to verify Surge's TCP callbacks are working
 * Compile: g++ -o verify_callbacks verify_surge_callbacks.cpp
 */

#include <iostream>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <cstring>
#include <string>

int main() {
    std::cout << "Surge Callback Verification Test\n";
    std::cout << "=================================\n\n";
    
    // Connect to Surge's broker socket
    const char* socket_path = "/tmp/sas-plugin-router.sock";
    
    int sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock < 0) {
        std::cerr << "Failed to create socket\n";
        return 1;
    }
    
    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, socket_path, sizeof(addr.sun_path) - 1);
    
    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        std::cerr << "Failed to connect to " << socket_path << "\n";
        std::cerr << "Is the broker running?\n";
        close(sock);
        return 1;
    }
    
    std::cout << "✅ Connected to broker\n\n";
    
    // Register as Surge plugin to receive commands
    std::string register_msg = R"({"type":"register","plugin_sig":"test-surge-123","plugin_name":"Test Surge","plugin_type":"surge-xt-test"})" "\n";
    
    std::cout << "→ Registering as fake Surge plugin...\n";
    write(sock, register_msg.c_str(), register_msg.length());
    
    // Send heartbeat
    std::string heartbeat = R"({"type":"heartbeat"})" "\n";
    write(sock, heartbeat.c_str(), heartbeat.length());
    
    std::cout << "Listening for commands...\n";
    std::cout << "Now run test_surge_presets.sh with plugin_sig: test-surge-123\n\n";
    
    // Listen for incoming commands
    char buffer[4096];
    while (true) {
        int bytes = read(sock, buffer, sizeof(buffer) - 1);
        if (bytes > 0) {
            buffer[bytes] = '\0';
            std::cout << "← Received: " << buffer << std::endl;
            
            // Parse and respond to commands
            if (strstr(buffer, "list_presets")) {
                std::string response = R"({"ok":true,"request_id":"test","data":{"presets":["Init","Test Preset"]}})" "\n";
                write(sock, response.c_str(), response.length());
                std::cout << "→ Sent preset list response\n";
            }
            else if (strstr(buffer, "load_preset")) {
                std::string response = R"({"ok":true,"request_id":"test","data":{"preset":"loaded"}})" "\n";
                write(sock, response.c_str(), response.length());
                std::cout << "→ Sent preset load response\n";
            }
            else if (strstr(buffer, "set_param")) {
                std::string response = R"({"ok":true,"request_id":"test","data":{"param":"set"}})" "\n";
                write(sock, response.c_str(), response.length());
                std::cout << "→ Sent param set response\n";
            }
        }
        else if (bytes == 0) {
            std::cout << "Connection closed\n";
            break;
        }
    }
    
    close(sock);
    return 0;
}