#!/bin/bash

echo "Force Installing Surge XT S&S Fork v1.8.0..."
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
if strings "/Library/Audio/Plug-Ins/VST3/Surge XT.vst3/Contents/MacOS/Surge XT" | grep -q "S&S Fork v1.8.0"; then
    echo "✅ SUCCESS: S&S Fork v1.8.0 installed correctly!"
    echo ""
    echo "Installed file info:"
    ls -la "/Library/Audio/Plug-Ins/VST3/Surge XT.vst3/Contents/MacOS/Surge XT"
    echo ""
    echo "KEY IMPROVEMENTS IN v1.8.0:"
    echo "• Each Surge instance gets unique, stable plugin signature"
    echo "• Plugin signatures persist across reconnections"
    echo "• Better instance identification in logs"
    echo "• Reduced heartbeat logging (1 in 10)"
    echo ""
    echo "IMPORTANT: You must do the following:"
    echo "1. Completely quit Reaper (Cmd+Q)"
    echo "2. Clear plugin cache (run this command):"
    echo "   rm ~/Library/Application\ Support/REAPER/reaper-vstplugins_arm64.ini"
    echo "3. Restart Reaper"
    echo "4. Rescan for VST plugins (Options → Preferences → VST → Re-scan)"
    echo "5. Remove old Surge XT from your tracks"
    echo "6. Re-add Surge XT to your tracks"
    echo "7. Check /tmp/surge-router.log to see instance IDs"
else
    echo "❌ ERROR: Installation verification failed!"
fi