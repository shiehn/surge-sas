#!/bin/bash

# Surge XT S&S Fork v1.27.0 Production Ready Install Script
# All critical issues fixed, production-ready system

echo "================================================"
echo " Installing Surge XT S&S Fork v1.27.0"
echo " Production Ready Release"
echo "================================================"
echo ""
echo "Features in v1.27.0:"
echo "  ✅ Direct command forwarding in broker"
echo "  ✅ Improved socket error detection (30s timeout)"
echo "  ✅ Full PIID support with persistence"
echo "  ✅ Handles set_piid commands"
echo "  ✅ Re-registration after PIID assignment"
echo "  ✅ Production-ready routing system"
echo ""

# Check if build exists
if [ ! -d "build-arm64/src/surge-xt/surge-xt_artefacts/Release/VST3/Surge XT.vst3" ]; then
    echo "❌ ERROR: Build not found!"
    echo "Please run: cmake --build build-arm64 --target surge-xt_VST3"
    exit 1
fi

echo "This script requires sudo access to install the VST3 plugin."
echo "Press Enter to continue or Ctrl-C to cancel..."
read

echo "Step 1: Removing old Surge XT installation..."
sudo rm -rf "/Library/Audio/Plug-Ins/VST3/Surge XT.vst3"

echo "Step 2: Installing new v1.27.0 build..."
sudo cp -r "build-arm64/src/surge-xt/surge-xt_artefacts/Release/VST3/Surge XT.vst3" "/Library/Audio/Plug-Ins/VST3/"

echo "Step 3: Setting permissions..."
sudo chmod -R 755 "/Library/Audio/Plug-Ins/VST3/Surge XT.vst3"
sudo xattr -dr com.apple.quarantine "/Library/Audio/Plug-Ins/VST3/Surge XT.vst3" 2>/dev/null

echo "Step 4: Verifying installation..."
if [ -d "/Library/Audio/Plug-Ins/VST3/Surge XT.vst3" ]; then
    echo "✅ Installation successful!"
    echo ""
    echo "================================================"
    echo " WHAT'S NEW IN v1.27.0"
    echo "================================================"
    echo ""
    echo "1. FIXED BROKER COMMAND FORWARDING"
    echo "   - Direct op commands now work"
    echo "   - No wrapper scripts needed"
    echo ""
    echo "2. IMPROVED ERROR DETECTION"
    echo "   - 30-second connection timeout"
    echo "   - Automatic reconnection on errors"
    echo "   - Better write error handling"
    echo ""
    echo "3. FULL PIID SUPPORT"
    echo "   - Accepts set_piid commands"
    echo "   - Persists routing in state"
    echo "   - Re-registers with PIID"
    echo ""
    echo "================================================"
    echo " STARTING THE PRODUCTION SYSTEM"
    echo "================================================"
    echo ""
    echo "1. Start the fixed broker:"
    echo "   node /Users/stevehiehn/sas-m4l-project/sas-assistant/dist/mcp-server/broker/smart-broker-v2.js &"
    echo ""
    echo "2. Start the production routing system:"
    echo "   node /Users/stevehiehn/sas-m4l-project/sas-assistant/production-routing-system.js &"
    echo ""
    echo "3. Restart REAPER and add/reload Surge instances"
    echo ""
    echo "4. The system will:"
    echo "   - Auto-connect to broker"
    echo "   - Auto-assign PIID routing"
    echo "   - Persist routing across restarts"
    echo "   - Handle errors gracefully"
    echo ""
    echo "================================================"
    echo " TESTING THE SYSTEM"
    echo "================================================"
    echo ""
    echo "Test preset changes:"
    echo "   node /Users/stevehiehn/sas-m4l-project/sas-assistant/test-preset-change.js"
    echo ""
    echo "Monitor connections:"
    echo "   tail -f /tmp/surge-router.log"
    echo ""
    echo "Check routing persistence:"
    echo "   cat /tmp/surge-routing.json"
    echo ""
    echo "================================================"
    echo " v1.27.0 - PRODUCTION READY ✅"
    echo "================================================"
else
    echo "❌ ERROR: Installation failed!"
    echo "Please check permissions and try again."
    exit 1
fi