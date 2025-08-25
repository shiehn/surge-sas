#!/bin/bash
# Simple shell script to test Surge preset control through broker
# This simulates what SAS Assistant would send

SOCKET="/tmp/sas-plugin-router.sock"
ALT_SOCKET="$HOME/.sas/router.sock"

# Check which socket exists
if [ -S "$SOCKET" ]; then
    echo "Using socket: $SOCKET"
elif [ -S "$ALT_SOCKET" ]; then
    SOCKET="$ALT_SOCKET"
    echo "Using socket: $SOCKET"
else
    echo "❌ No broker socket found!"
    echo "Please ensure SAS Assistant is running"
    exit 1
fi

# Check if Surge is connected
echo -e "\n📍 Checking Surge connection..."
if [ -f "/tmp/surge-router.log" ]; then
    echo "Recent Surge activity:"
    tail -5 /tmp/surge-router.log | grep -E "(plugin_sig|heartbeat|Sending registration)"
    
    # Extract plugin_sig from log
    PLUGIN_SIG=$(tail -20 /tmp/surge-router.log | grep "plugin_sig:" | tail -1 | sed 's/.*plugin_sig: //' | cut -d' ' -f1 | tr -d '\n')
    
    if [ -n "$PLUGIN_SIG" ]; then
        echo "Found plugin_sig: $PLUGIN_SIG"
    fi
else
    echo "No Surge log found"
fi

echo -e "\n🧪 Starting Surge Preset Tests\n"
echo "================================"

# Function to send JSON command to broker
send_command() {
    local json="$1"
    echo "→ Sending: $json"
    echo "$json" | nc -U "$SOCKET" -w 2
    echo ""
    sleep 1
}

# Test 1: List presets
echo -e "\n[TEST 1] List Presets"
echo "------------------------"
if [ -n "$PLUGIN_SIG" ]; then
    send_command '{"op":"list_presets","plugin_sig":"'$PLUGIN_SIG'","request_id":"test-001"}'
else
    send_command '{"op":"list_presets","request_id":"test-001"}'
fi

# Test 2: Load preset "Init Saw"
echo -e "\n[TEST 2] Load Preset: Init Saw"
echo "------------------------"
if [ -n "$PLUGIN_SIG" ]; then
    send_command '{"op":"load_preset","preset":"Init Saw","plugin_sig":"'$PLUGIN_SIG'","request_id":"test-002"}'
else
    send_command '{"op":"load_preset","preset":"Init Saw","request_id":"test-002"}'
fi

# Test 3: Set parameter (cutoff)
echo -e "\n[TEST 3] Set Parameter: Cutoff to 0.5"
echo "------------------------"
if [ -n "$PLUGIN_SIG" ]; then
    send_command '{"op":"set_param","index":0,"value":0.5,"plugin_sig":"'$PLUGIN_SIG'","request_id":"test-003"}'
else
    send_command '{"op":"set_param","index":0,"value":0.5,"request_id":"test-003"}'
fi

# Test 4: Get parameters
echo -e "\n[TEST 4] Get Parameters"
echo "------------------------"
if [ -n "$PLUGIN_SIG" ]; then
    send_command '{"op":"get_params","plugin_sig":"'$PLUGIN_SIG'","request_id":"test-004"}'
else
    send_command '{"op":"get_params","request_id":"test-004"}'
fi

# Test 5: Load another preset
echo -e "\n[TEST 5] Load Preset: Aggro Growlbass"
echo "------------------------"
if [ -n "$PLUGIN_SIG" ]; then
    send_command '{"op":"load_preset","preset":"Aggro Growlbass","plugin_sig":"'$PLUGIN_SIG'","request_id":"test-005"}'
else
    send_command '{"op":"load_preset","preset":"Aggro Growlbass","request_id":"test-005"}'
fi

echo -e "\n================================"
echo "Tests complete!"
echo ""
echo "Check /tmp/surge-router.log for Surge's response"
echo "tail -f /tmp/surge-router.log"