#!/bin/bash

echo "Force Installing Surge XT S&S Fork v2.0.0..."
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
if strings "/Library/Audio/Plug-Ins/VST3/Surge XT.vst3/Contents/MacOS/Surge XT" | grep -q "S&S Fork v2.0.0"; then
    echo "✅ SUCCESS: S&S Fork v2.0.0 installed correctly!"
    echo ""
    echo "Installed file info:"
    ls -la "/Library/Audio/Plug-Ins/VST3/Surge XT.vst3/Contents/MacOS/Surge XT"
    echo ""
    echo "KEY IMPROVEMENTS IN v2.0.0:"
    echo "• Enhanced command processing with detailed logging"
    echo "• Logs show exactly what presets are being searched"
    echo "• Shows preset callback execution status"
    echo "• Displays actual preset names from storage"
    echo "• Confirms if callback is NULL or working"
    echo ""
    echo "DEBUGGING PRESET ISSUES:"
    echo "1. Try loading a preset through the broker"
    echo "2. Check /tmp/surge-router.log for:"
    echo "   - [PresetLoad] entries showing what's being searched"
    echo "   - Actual preset names in Surge's storage"
    echo "   - Whether the callback is being called"
    echo "3. This will reveal the exact preset name format needed"
    echo ""
    echo "The log will show if:"
    echo "• Preset names need categories (e.g., 'Bass/Init Saw')"
    echo "• Names are case-sensitive"
    echo "• Callback is actually being invoked"
else
    echo "❌ ERROR: Installation verification failed!"
fi