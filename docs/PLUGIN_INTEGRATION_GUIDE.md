# Plugin Integration Guide for SAS Broker

This guide explains how to add SAS Broker support to any audio plugin.

## Quick Start

To add broker support to your plugin, you need to:

1. **Connect** to the broker socket
2. **Register** your plugin with a unique signature
3. **Send heartbeats** to maintain the connection
4. **Process commands** from the broker
5. **Send responses** back to the broker

## Minimal Implementation (C++)

```cpp
// BrokerClient.h
class BrokerClient {
public:
    BrokerClient(const std::string& pluginType, const std::string& pluginName);
    ~BrokerClient();
    
    // Callbacks your plugin implements
    void onLoadPreset(std::function<bool(const std::string&)> callback);
    void onSetParameter(std::function<bool(int, float)> callback);
    void onListPresets(std::function<std::vector<std::string>()> callback);
    
    void start();
    void stop();
    
private:
    void connectLoop();
    void processMessage(const std::string& json);
    void sendRegistration();
    void sendHeartbeat();
    void sendResponse(const std::string& requestId, bool success, const std::string& data);
    
    std::string pluginSig;
    std::string pluginType;
    std::string pluginName;
    int socketFd = -1;
    std::atomic<bool> running{false};
    std::thread workerThread;
};
```

## Step-by-Step Integration

### Step 1: Generate Unique Plugin Signature

```cpp
std::string generatePluginSignature() {
    static std::atomic<uint32_t> instanceCounter{0};
    uint32_t pid = getpid();
    uint32_t instance = instanceCounter++;
    
    // Create deterministic signature
    std::stringstream ss;
    ss << std::hex << pid << "-" << instance << "-" 
       << std::hash<std::string>{}(pluginName);
    return ss.str();
}
```

### Step 2: Connect to Broker

```cpp
bool connectToBroker() {
    // Try primary socket
    const char* primaryPath = "/tmp/sas-plugin-router.sock";
    const char* fallbackPath = "~/.sas/router.sock";
    
    int sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock < 0) return false;
    
    struct sockaddr_un addr;
    addr.sun_family = AF_UNIX;
    
    // Try primary path
    strcpy(addr.sun_path, primaryPath);
    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) == 0) {
        socketFd = sock;
        return true;
    }
    
    // Try fallback path
    strcpy(addr.sun_path, expandPath(fallbackPath).c_str());
    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) == 0) {
        socketFd = sock;
        return true;
    }
    
    close(sock);
    return false;
}
```

### Step 3: Send Registration

```cpp
void sendRegistration() {
    json msg = {
        {"type", "register"},
        {"plugin_sig", pluginSig},
        {"plugin_type", pluginType},    // e.g., "vital", "dexed", "surge-xt"
        {"plugin_name", pluginName},     // e.g., "Vital Instance 0"
        {"plugin_version", PLUGIN_VERSION},
        {"build", "vst3"}               // or "au", "lv2", etc.
    };
    
    std::string jsonStr = msg.dump() + "\n";
    send(socketFd, jsonStr.c_str(), jsonStr.length(), 0);
}
```

### Step 4: Heartbeat Loop

```cpp
void heartbeatLoop() {
    while (running) {
        std::this_thread::sleep_for(std::chrono::seconds(5));
        
        json heartbeat = {
            {"type", "heartbeat"},
            {"plugin_sig", pluginSig}
        };
        
        std::string msg = heartbeat.dump() + "\n";
        if (send(socketFd, msg.c_str(), msg.length(), 0) < 0) {
            // Connection lost, reconnect
            reconnect();
        }
    }
}
```

### Step 5: Process Commands

```cpp
void processCommand(const json& cmd) {
    std::string op = cmd["op"];
    std::string requestId = cmd["request_id"];
    
    if (op == "load_preset" || op == "set_preset") {
        std::string presetName = cmd["preset"];
        bool success = presetCallback(presetName);
        
        json response = {
            {"ok", success},
            {"request_id", requestId},
            {"data", {{"preset", presetName}}}
        };
        sendResponse(response);
    }
    else if (op == "set_param") {
        int index = cmd["index"];
        float value = cmd["value"];
        bool success = paramCallback(index, value);
        
        json response = {
            {"ok", success},
            {"request_id", requestId},
            {"data", {{"index", index}, {"value", value}}}
        };
        sendResponse(response);
    }
    else if (op == "list_presets") {
        auto presets = listPresetsCallback();
        
        json response = {
            {"ok", true},
            {"request_id", requestId},
            {"data", {{"presets", presets}}}
        };
        sendResponse(response);
    }
    else if (op == "get_params") {
        auto params = getParamsCallback();
        
        json response = {
            {"ok", true},
            {"request_id", requestId},
            {"data", params}
        };
        sendResponse(response);
    }
}
```

### Step 6: Connection Retry Logic

```cpp
void connectionLoop() {
    int retryDelay = 500; // Start with 500ms
    const int maxDelay = 5000; // Max 5 seconds
    
    while (running) {
        if (!connected) {
            if (connectToBroker()) {
                connected = true;
                sendRegistration();
                retryDelay = 500; // Reset delay on success
            } else {
                // Exponential backoff
                std::this_thread::sleep_for(std::chrono::milliseconds(retryDelay));
                retryDelay = std::min(retryDelay * 2, maxDelay);
                continue;
            }
        }
        
        // Read messages
        char buffer[4096];
        int bytes = recv(socketFd, buffer, sizeof(buffer) - 1, 0);
        
        if (bytes > 0) {
            buffer[bytes] = '\0';
            processMessage(std::string(buffer));
        } else if (bytes == 0 || (bytes < 0 && errno != EAGAIN)) {
            // Connection lost
            connected = false;
            close(socketFd);
            socketFd = -1;
        }
    }
}
```

## Integration for Different Plugin Types

### JUCE Plugin

```cpp
class MyJucePlugin : public AudioProcessor {
    std::unique_ptr<BrokerClient> brokerClient;
    
    MyJucePlugin() {
        brokerClient = std::make_unique<BrokerClient>("myplugin", getName());
        
        brokerClient->onLoadPreset([this](const std::string& name) {
            return loadPresetByName(name);
        });
        
        brokerClient->onSetParameter([this](int idx, float val) {
            setParameterNotifyingHost(idx, val);
            return true;
        });
        
        brokerClient->start();
    }
};
```

### VST3 Plugin

```cpp
class MyVST3Controller : public Vst::EditController {
    BrokerClient broker{"myvst3", "My VST3 Plugin"};
    
    tresult PLUGIN_API initialize(FUnknown* context) override {
        // Regular VST3 init...
        
        broker.onLoadPreset([this](const std::string& preset) {
            return loadProgram(findPresetIndex(preset));
        });
        
        broker.start();
        return kResultTrue;
    }
};
```

### Pure Data External

```c
typedef struct _broker {
    t_object x_obj;
    void* client;
} t_broker;

static void broker_loadpreset(t_broker* x, t_symbol* s) {
    broker_client_load_preset(x->client, s->s_name);
}

void broker_setup(void) {
    broker_class = class_new(gensym("broker"),
        (t_newmethod)broker_new,
        (t_method)broker_free,
        sizeof(t_broker), 0, 0);
    
    class_addmethod(broker_class,
        (t_method)broker_loadpreset,
        gensym("preset"), A_SYMBOL, 0);
}
```

## Language Bindings

### Python

```python
import socket
import json
import threading

class BrokerClient:
    def __init__(self, plugin_type, plugin_name):
        self.plugin_type = plugin_type
        self.plugin_name = plugin_name
        self.plugin_sig = self.generate_signature()
        self.callbacks = {}
        
    def connect(self):
        self.sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        self.sock.connect('/tmp/sas-plugin-router.sock')
        self.register()
        
    def register(self):
        msg = {
            'type': 'register',
            'plugin_sig': self.plugin_sig,
            'plugin_type': self.plugin_type,
            'plugin_name': self.plugin_name
        }
        self.send(msg)
        
    def on_preset_load(self, callback):
        self.callbacks['load_preset'] = callback
        
    def process_command(self, cmd):
        op = cmd.get('op')
        if op in self.callbacks:
            result = self.callbacks[op](cmd)
            self.send_response(cmd.get('request_id'), result)
```

### Rust

```rust
use serde::{Deserialize, Serialize};
use std::os::unix::net::UnixStream;

pub struct BrokerClient {
    plugin_sig: String,
    plugin_type: String,
    socket: Option<UnixStream>,
}

impl BrokerClient {
    pub fn new(plugin_type: &str, plugin_name: &str) -> Self {
        Self {
            plugin_sig: generate_signature(),
            plugin_type: plugin_type.to_string(),
            socket: None,
        }
    }
    
    pub fn connect(&mut self) -> Result<(), Box<dyn Error>> {
        let socket = UnixStream::connect("/tmp/sas-plugin-router.sock")?;
        self.socket = Some(socket);
        self.send_registration()?;
        Ok(())
    }
    
    pub fn on_preset_load<F>(&mut self, callback: F) 
    where
        F: Fn(&str) -> bool + 'static
    {
        self.preset_callback = Some(Box::new(callback));
    }
}
```

## Testing Your Integration

### 1. Basic Connection Test

```bash
# Check if your plugin connects
tail -f /tmp/surge-router.log | grep "register"

# You should see:
# [Instance 0] Sending registration with plugin_sig: abc-0-def
```

### 2. Command Test

```bash
# Send a test command (replace YOUR_PLUGIN_SIG)
echo '{"op":"list_presets","plugin_sig":"YOUR_PLUGIN_SIG","request_id":"test"}' | \
  nc -U /tmp/sas-plugin-router.sock
```

### 3. Integration Test Script

```python
#!/usr/bin/env python3
import socket
import json

sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
sock.connect('/tmp/sas-plugin-router.sock')

# List plugins
cmd = {"op": "list_plugins", "request_id": "test-001"}
sock.send(json.dumps(cmd).encode() + b'\n')
response = sock.recv(4096)
print("Plugins:", response.decode())

# Load preset
plugins = json.loads(response)
if plugins.get('plugins'):
    sig = plugins['plugins'][0]['plugin_sig']
    cmd = {
        "op": "load_preset",
        "preset": "Init",
        "plugin_sig": sig,
        "request_id": "test-002"
    }
    sock.send(json.dumps(cmd).encode() + b'\n')
    response = sock.recv(4096)
    print("Preset response:", response.decode())
```

## Best Practices

### 1. Error Handling
- Always validate incoming JSON
- Return meaningful error messages
- Log errors for debugging

### 2. Thread Safety
- Use separate thread for broker communication
- Protect shared data with mutexes
- Don't block audio thread

### 3. Preset Name Format
- Use exact names from your preset list
- Include category if part of the name
- Handle case sensitivity properly

### 4. Performance
- Batch parameter updates when possible
- Cache preset lists
- Use efficient JSON parsing

### 5. Debugging
```cpp
// Add debug logging
#ifdef DEBUG
#define LOG(msg) fprintf(logFile, "[%s] %s\n", pluginSig.c_str(), msg)
#else
#define LOG(msg)
#endif
```

## Common Issues

| Problem | Solution |
|---------|----------|
| Can't connect | Check broker is running, socket exists |
| No heartbeat | Ensure heartbeat thread is running |
| Commands ignored | Verify plugin_sig matches |
| Preset not found | Check exact name format |
| Crashes on exit | Properly stop threads before destruction |

## Example Plugins with Broker Support

- **Surge XT**: Full implementation in C++
- **Vital**: (Planned) Modern wavetable synth
- **Dexed**: (Planned) DX7 emulator
- **ZynAddSubFX**: (Planned) Additive/subtractive synth

## License Considerations

The broker client code can be:
- Embedded directly (MIT/BSD style)
- Linked as a library (LGPL compatible)
- Implemented independently (Protocol is open)

---

*For the latest updates and examples, see: https://github.com/your-org/sas-broker-client*