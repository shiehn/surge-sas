#!/bin/bash

echo "Force Installing Surge XT S&S Fork v1.9.0..."
echo ""
echo "Step 1: Removing old installation..."
sudo rm -rf "/Library/Audio/Plug-Ins/VST3/Surge XT.vst3"
sudo rm -rf "/Library/Audio/Plug-Ins/VST3/Surge XT.vst3.backup"

echo "Step 2: Copying new build..."
sudo cp -R "build-arm64/surge_xt_products/Surge XT.vst3" "/Library/Audio/Plug-Ins/VST3/"

echo "Step 3: Setting permissions..."
sudo chmod -R 755 "/Library/Audio/Plug-Ins/VST3/Surge XT.vst3"
sudo xattr -cr "/Library/Audio/Plug-Ins/VST3/Surge XT.vst3"

echo "Step 4: Verifying installation..."
if strings "/Library/Audio/Plug-Ins/VST3/Surge XT.vst3/Contents/MacOS/Surge XT" | grep -q "S&S Fork v1.9.0"; then
    echo "✅ SUCCESS: S&S Fork v1.9.0 installed correctly!"
    echo ""
    echo "Installed file info:"
    ls -la "/Library/Audio/Plug-Ins/VST3/Surge XT.vst3/Contents/MacOS/Surge XT"
    echo ""
    echo "KEY IMPROVEMENTS IN v1.9.0:"
    echo "• Persistent retry logic - keeps trying to connect forever"
    echo "• Better connection logging shows retry attempts"
    echo "• Works even if broker starts after Surge"
    echo "• Logs connection attempts every 10 tries"
    echo "• Shows backoff delays when broker unavailable"
    echo ""
    echo "IMPORTANT NOTES:"
    echo "• Existing Surge instances will now auto-connect when broker starts"
    echo "• No need to remove/re-add plugins anymore"
    echo "• Check /tmp/surge-router.log to see connection attempts"
    echo ""
    echo "To use:"
    echo "1. Start REAPER with Surge tracks (broker can be off)"
    echo "2. Start SAS Assistant broker at any time"
    echo "3. Surge will auto-connect within seconds"
else
    echo "❌ ERROR: Installation verification failed!"
fi