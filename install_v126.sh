#!/bin/bash

# Install script for Surge XT S&S Fork v1.26.0
echo "================================================"
echo " Installing Surge XT S&S Fork v1.26.0"
echo " Features: TCP Control, Broker Client, PIID"
echo "================================================"
echo ""

# Check if build exists
if [ ! -d "build-arm64/src/surge-xt/surge-xt_artefacts/Release/VST3/Surge XT.vst3" ]; then
    echo "❌ ERROR: Build not found!"
    echo "Please run: cmake --build build-arm64 --target surge-xt_VST3"
    exit 1
fi

echo "Step 1: Removing old Surge XT installation..."
sudo rm -rf "/Library/Audio/Plug-Ins/VST3/Surge XT.vst3"

echo "Step 2: Installing new v1.26.0 build..."
sudo cp -r "build-arm64/src/surge-xt/surge-xt_artefacts/Release/VST3/Surge XT.vst3" "/Library/Audio/Plug-Ins/VST3/"

echo "Step 3: Setting permissions..."
sudo chmod -R 755 "/Library/Audio/Plug-Ins/VST3/Surge XT.vst3"
sudo xattr -dr com.apple.quarantine "/Library/Audio/Plug-Ins/VST3/Surge XT.vst3" 2>/dev/null

echo "Step 4: Verifying installation..."
if [ -d "/Library/Audio/Plug-Ins/VST3/Surge XT.vst3" ]; then
    echo "✅ Installation successful!"
    echo ""
    echo "================================================"
    echo " NEXT STEPS:"
    echo "================================================"
    echo "1. Restart REAPER"
    echo "2. Add/reload Surge XT instances"
    echo "3. Check plugin name shows: [S&S Fork v1.26.0]"
    echo ""
    echo "VERIFICATION:"
    echo "• Check broker connections:"
    echo "  tail -f /tmp/surge-router.log"
    echo ""
    echo "• The plugin will:"
    echo "  - Connect to /tmp/sas-plugin-router.sock"
    echo "  - Register with unique plugin signature"
    echo "  - Support PIID routing if configured"
    echo "  - Respond to load_preset commands"
    echo ""
    echo "• Version features:"
    echo "  - Outbound broker connection (no port conflicts)"
    echo "  - VST parameters 900-903 expose plugin signature"
    echo "  - Handles preset category prefixes correctly"
    echo "  - Clean shutdown with thread joining"
    echo "  - PIID persistence in plugin state"
    echo "================================================"
else
    echo "❌ ERROR: Installation failed!"
    echo "Please check permissions and try again."
    exit 1
fi