# SAS Broker API Reference

## Protocol Overview

- **Transport**: Unix Domain Socket (IPC) or TCP
- **Format**: JSON Lines (newline-delimited JSON)
- **Encoding**: UTF-8
- **Direction**: Bidirectional
- **Connection**: Persistent with heartbeat

## Socket Locations

| Priority | Path | Type | Notes |
|----------|------|------|-------|
| 1 | `/tmp/sas-plugin-router.sock` | Unix Socket | Primary location |
| 2 | `~/.sas/router.sock` | Unix Socket | User-specific fallback |
| 3 | `127.0.0.1:7833` | TCP | Network fallback |

## Message Types

### 1. Registration Messages

#### Plugin → Broker: Register

Sent immediately after connection to identify the plugin.

```json
{
  "type": "register",
  "plugin_sig": "string",       // Unique plugin signature
  "plugin_type": "string",      // Plugin identifier (e.g., "surge-xt-sas")
  "plugin_name": "string",      // Display name
  "plugin_version": "string",   // Version string
  "instance_id": number,        // Instance counter (optional)
  "build": "string"            // Build type: "vst3", "au", "lv2", etc.
}
```

**Example:**
```json
{
  "type": "register",
  "plugin_sig": "12345-0-a1b2c3d4",
  "plugin_type": "surge-xt-sas",
  "plugin_name": "Surge XT [S&S Fork Instance 0]",
  "plugin_version": "2.0.0",
  "instance_id": 0,
  "build": "vst3"
}
```

### 2. Heartbeat Messages

#### Plugin → Broker: Heartbeat

Sent every 5 seconds to maintain connection.

```json
{
  "type": "heartbeat",
  "plugin_sig": "string"    // Plugin signature (optional but recommended)
}
```

**Example:**
```json
{
  "type": "heartbeat",
  "plugin_sig": "12345-0-a1b2c3d4"
}
```

### 3. Command Messages

#### Broker → Plugin: Command Wrapper

All commands from broker to plugin are wrapped with type "command".

```json
{
  "type": "command",
  "op": "string",           // Operation name
  "request_id": "string",   // Request correlation ID
  "plugin_sig": "string",   // Target plugin (optional if routed)
  ...                       // Operation-specific fields
}
```

### 4. Response Messages

#### Plugin → Broker: Response

Response to a command, correlated by request_id.

```json
{
  "ok": boolean,            // Success/failure
  "request_id": "string",   // Correlation ID from request
  "data": object,          // Success data (optional)
  "error": "string"        // Error message (optional)
}
```

## Operations

### Preset Operations

#### load_preset / set_preset

Load a preset by name.

**Request:**
```json
{
  "type": "command",
  "op": "load_preset",
  "preset": "string",       // Exact preset name
  "plugin_sig": "string",
  "request_id": "string"
}
```

**Success Response:**
```json
{
  "ok": true,
  "request_id": "string",
  "data": {
    "preset": "string"     // Loaded preset name
  }
}
```

**Error Response:**
```json
{
  "ok": false,
  "request_id": "string",
  "error": "Preset not found: Init Saw"
}
```

#### list_presets

Get list of available presets.

**Request:**
```json
{
  "type": "command",
  "op": "list_presets",
  "plugin_sig": "string",
  "request_id": "string"
}
```

**Response:**
```json
{
  "ok": true,
  "request_id": "string",
  "data": {
    "presets": ["Init Saw", "Aggro Growlbass", "Creamy Keys"]
  }
}
```

### Parameter Operations

#### set_param

Set a parameter value.

**Request:**
```json
{
  "type": "command",
  "op": "set_param",
  "index": number,          // Parameter index
  "value": number,          // Value (typically 0.0-1.0)
  "plugin_sig": "string",
  "request_id": "string"
}
```

**Response:**
```json
{
  "ok": true,
  "request_id": "string",
  "data": {
    "index": number,
    "value": number
  }
}
```

#### get_params

Get all parameter values.

**Request:**
```json
{
  "type": "command",
  "op": "get_params",
  "plugin_sig": "string",
  "request_id": "string"
}
```

**Response:**
```json
{
  "ok": true,
  "request_id": "string",
  "data": {
    "params": [
      {"index": 0, "name": "Cutoff", "value": 0.5},
      {"index": 1, "name": "Resonance", "value": 0.3}
    ]
  }
}
```

### Discovery Operations

#### list_plugins

Get list of connected plugins (Client → Broker).

**Request:**
```json
{
  "op": "list_plugins",
  "request_id": "string"
}
```

**Response:**
```json
{
  "ok": true,
  "request_id": "string",
  "plugins": [
    {
      "plugin_sig": "string",
      "plugin_type": "string",
      "plugin_name": "string",
      "instance_id": number,
      "last_heartbeat": "string"    // ISO 8601 timestamp
    }
  ]
}
```

#### probe

Probe for responsive plugins (Client → Broker).

**Request:**
```json
{
  "op": "probe",
  "request_id": "string"
}
```

**Response:**
```json
{
  "ok": true,
  "request_id": "string",
  "data": {
    "plugins": [
      {
        "plugin_sig": "string",
        "responsive": boolean,
        "latency_ms": number
      }
    ]
  }
}
```

## Plugin Signature Specification

Plugin signatures uniquely identify each plugin instance.

### Format
```
{PID}-{INSTANCE}-{HASH}
```

### Components
- **PID**: Process ID (8 hex digits)
- **INSTANCE**: Instance counter (4 hex digits)
- **HASH**: Deterministic hash (12+ hex digits)

### Examples
```
00003039-0000-a1b2c3d4e5f6    // First instance
00003039-0001-b2c3d4e5f6a7    // Second instance
00003039-0002-c3d4e5f6a7b8    // Third instance
```

### Generation Algorithm
```cpp
std::string generateSignature() {
    uint32_t pid = getpid();
    uint32_t instance = instanceCounter++;
    
    std::mt19937_64 gen((uint64_t(pid) << 32) | instance);
    uint64_t hash = gen();
    
    std::stringstream ss;
    ss << std::hex << std::setfill('0')
       << std::setw(8) << pid << "-"
       << std::setw(4) << instance << "-"
       << std::setw(12) << (hash & 0xFFFFFFFFFFFF);
    
    return ss.str();
}
```

## Error Codes

| Code | Message | Description |
|------|---------|-------------|
| `PLUGIN_NOT_FOUND` | "Plugin not found: {sig}" | No plugin with given signature |
| `PRESET_NOT_FOUND` | "Preset not found: {name}" | Preset doesn't exist |
| `PARAM_OUT_OF_RANGE` | "Parameter index out of range: {idx}" | Invalid parameter index |
| `INVALID_VALUE` | "Invalid value: {val}" | Value outside valid range |
| `NOT_IMPLEMENTED` | "Operation not implemented: {op}" | Plugin doesn't support operation |
| `PARSE_ERROR` | "Failed to parse JSON" | Malformed JSON message |
| `TIMEOUT` | "Operation timed out" | No response within timeout |

## Connection Lifecycle

### 1. Connection Establishment
```
Plugin → Socket Connect → Broker
Plugin → Register Message → Broker
Broker → Store Mapping → Internal
```

### 2. Steady State
```
Plugin → Heartbeat (every 5s) → Broker
Broker → Command → Plugin
Plugin → Response → Broker
```

### 3. Disconnection
```
Plugin → Close Socket → Broker
Broker → Remove Mapping → Internal
Broker → Notify Clients → SAS Assistant
```

### 4. Reconnection
```
Plugin → Exponential Backoff → Retry
Plugin → Socket Connect → Broker
Plugin → Register (new session) → Broker
```

## Rate Limits

| Operation | Limit | Window |
|-----------|-------|--------|
| Commands | 100/sec | Per plugin |
| Heartbeats | 1/sec | Per plugin |
| Registrations | 10/min | Per socket |
| List operations | 10/sec | Global |

## Example Message Flow

### Complete Preset Change

```
1. Client → Broker
{
  "op": "list_plugins",
  "request_id": "disc-001"
}

2. Broker → Client
{
  "ok": true,
  "request_id": "disc-001",
  "plugins": [
    {"plugin_sig": "3039-0-abc", "plugin_type": "surge-xt-sas", ...}
  ]
}

3. Client → Broker
{
  "op": "list_presets",
  "plugin_sig": "3039-0-abc",
  "request_id": "list-001"
}

4. Broker → Plugin
{
  "type": "command",
  "op": "list_presets",
  "request_id": "list-001"
}

5. Plugin → Broker
{
  "ok": true,
  "request_id": "list-001",
  "data": {
    "presets": ["Init Saw", "Bass Wobble"]
  }
}

6. Broker → Client
{
  "ok": true,
  "request_id": "list-001",
  "data": {
    "presets": ["Init Saw", "Bass Wobble"]
  }
}

7. Client → Broker
{
  "op": "load_preset",
  "preset": "Init Saw",
  "plugin_sig": "3039-0-abc",
  "request_id": "load-001"
}

8. Broker → Plugin
{
  "type": "command",
  "op": "load_preset",
  "preset": "Init Saw",
  "request_id": "load-001"
}

9. Plugin → Broker
{
  "ok": true,
  "request_id": "load-001",
  "data": {
    "preset": "Init Saw"
  }
}

10. Broker → Client
{
  "ok": true,
  "request_id": "load-001",
  "data": {
    "preset": "Init Saw"
  }
}
```

## Versioning

The protocol uses semantic versioning:

- **1.x.x**: Current stable protocol
- **2.x.x**: (Future) Adds track awareness
- **3.x.x**: (Future) Adds batch operations

Plugins should include their protocol version in registration:
```json
{
  "type": "register",
  "protocol_version": "1.0.0",
  ...
}
```

## Security Notes

1. **Local Only**: Unix sockets are local-only by default
2. **No Auth**: Relies on filesystem permissions
3. **Validation**: All inputs must be validated
4. **Injection**: Preset names must be sanitized
5. **DOS**: Rate limiting prevents flooding

---

*Last Updated: August 2024*
*Protocol Version: 1.0.0*