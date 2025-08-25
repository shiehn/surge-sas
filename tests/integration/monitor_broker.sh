#!/bin/bash
# Monitor broker communication in real-time

echo "🔍 Broker Communication Monitor"
echo "================================"
echo ""

# Check sockets
echo "📍 Checking sockets:"
ls -la /tmp/sas-plugin-router.sock 2>/dev/null && echo "✅ Found /tmp/sas-plugin-router.sock"
ls -la ~/.sas/router.sock 2>/dev/null && echo "✅ Found ~/.sas/router.sock"
echo ""

# Check processes
echo "📍 Checking processes:"
ps aux | grep -i surge | grep -v grep | head -2
ps aux | grep -i "sas" | grep -v grep | head -2
echo ""

# Monitor Surge log
echo "📍 Starting monitors..."
echo ""
echo "Surge log (last 10 lines):"
tail -10 /tmp/surge-router.log
echo ""

echo "================================"
echo "Monitoring in real-time..."
echo "(Press Ctrl+C to stop)"
echo "================================"
echo ""

# Start monitoring in background
tail -f /tmp/surge-router.log | sed 's/^/[SURGE] /' &
SURGE_PID=$!

# Give user instructions
echo "Now you can:"
echo "1. Run: ./test_surge_presets.sh (in another terminal)"
echo "2. Use REAPER to change presets"
echo "3. Watch this terminal for activity"
echo ""

# Wait for interrupt
trap "kill $SURGE_PID 2>/dev/null; echo 'Stopped monitoring'" INT
wait