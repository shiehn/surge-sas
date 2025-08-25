# Surge XT Broker Integration Test Report

## Test Overview
Testing preset control in Surge XT v1.7.0 through the SAS Plugin Router broker architecture.

## Test Setup

### Prerequisites
1. **Surge XT v1.7.0** - Custom fork with outbound broker connection
2. **SAS Assistant** - Running broker at `/tmp/sas-plugin-router.sock`
3. **REAPER** - With Surge XT loaded as VST3

### Architecture Being Tested
```
[SAS Assistant] ← JSON → [Broker Socket] ← JSON → [Surge XT Plugin]
                            ↑
                    /tmp/sas-plugin-router.sock
```

## Test Files Created

### 1. `test_real_broker.py`
- **Purpose**: Python client that connects to real broker
- **Features**:
  - Connects to broker socket
  - Sends preset commands
  - Receives and validates responses
  - No mocks - uses real implementation

### 2. `test_surge_presets.sh`
- **Purpose**: Shell script for simple command testing
- **Features**:
  - Sends raw JSON commands via netcat
  - Tests 5 core operations:
    1. List presets
    2. Load preset "Init Saw"
    3. Set parameter (cutoff)
    4. Get parameters
    5. Load preset "Aggro Growlbass"

### 3. `monitor_broker.sh`
- **Purpose**: Real-time monitoring of broker communication
- **Features**:
  - Shows active sockets
  - Monitors Surge log output
  - Tracks heartbeats and messages

### 4. `verify_surge_callbacks.cpp`
- **Purpose**: C++ test to verify callback handling
- **Features**:
  - Simulates a Surge plugin connection
  - Receives and responds to broker commands
  - Validates message flow

## Test Execution

### Test Run 1: Direct Command Sending
```bash
./test_surge_presets.sh
```

**Result**: Commands sent successfully to broker socket

**Observations**:
- Broker socket is active at `/tmp/sas-plugin-router.sock`
- Commands are sent but no responses received
- Surge log shows only heartbeats, no command processing

### Test Run 2: Monitoring
```bash
tail -f /tmp/surge-router.log
```

**Result**: Continuous heartbeat messages

**Observations**:
- Surge is connected and sending heartbeats every ~100ms
- No incoming commands logged
- Registration appears successful based on earlier logs

## Key Findings

### ✅ Working Components
1. **Socket Connection** - Surge connects to broker successfully
2. **Registration** - Surge sends registration with plugin_sig
3. **Heartbeat** - Regular heartbeats maintain connection
4. **Broker Socket** - Broker accepts connections at expected path

### ❌ Issues Identified
1. **Command Routing** - Commands sent to broker not reaching Surge
2. **Plugin Discovery** - No mechanism to discover plugin_sig from broker
3. **Response Path** - Responses from Surge (if any) not reaching test client

## Potential Root Causes

### 1. Broker Routing Logic
The broker may need explicit routing rules to forward commands to specific plugins. Current implementation might be missing:
- Plugin signature matching in command routing
- Request/response correlation

### 2. Message Format Mismatch
Surge expects:
```json
{"op":"load_preset","preset":"name","request_id":"123"}
```

But broker might expect different format or additional fields.

### 3. Callback Registration
Surge's callbacks might not be properly registered. Check:
- `presetLoadCb` initialization in SurgeSynthesizer.cpp
- `paramSetCb` initialization
- Callback invocation in TCPController::processRouterCommand()

## Recommendations

### Immediate Actions
1. **Add Debug Logging** - Log all received messages in Surge:
   ```cpp
   // In handleMessage()
   FILE* log = fopen("/tmp/surge-router.log", "a");
   fprintf(log, "Received message: %s\n", message.c_str());
   ```

2. **Verify Callbacks** - Ensure callbacks are set:
   ```cpp
   // In SurgeSynthesizer constructor
   tcpController.setPresetLoadCallback([this](name) {
       // Log and load preset
   });
   ```

3. **Test Broker Forwarding** - Create simple test to verify broker forwards messages

### Long-term Improvements
1. **Protocol Documentation** - Create detailed spec for broker protocol
2. **Integration Tests** - Automated tests in CI/CD pipeline
3. **Error Handling** - Better error responses from Surge
4. **Discovery Mechanism** - API to query connected plugins

## Test Commands Reference

### List Presets
```json
{"op":"list_presets","plugin_sig":"<sig>","request_id":"test-001"}
```

### Load Preset
```json
{"op":"load_preset","preset":"Init Saw","plugin_sig":"<sig>","request_id":"test-002"}
```

### Set Parameter
```json
{"op":"set_param","index":0,"value":0.5,"plugin_sig":"<sig>","request_id":"test-003"}
```

### Get Parameters
```json
{"op":"get_params","plugin_sig":"<sig>","request_id":"test-004"}
```

## Conclusion

The integration tests prove that:
1. ✅ Surge successfully connects to the broker
2. ✅ Registration and heartbeat mechanisms work
3. ❌ Preset commands are not being routed from broker to Surge
4. ❌ No responses are received from preset operations

**Next Steps**: Debug the broker's message routing to ensure commands with matching `plugin_sig` are forwarded to the correct Surge instance.