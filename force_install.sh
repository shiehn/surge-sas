#!/bin/bash

echo "Force Installing Surge XT S&S Fork..."
echo ""
echo "Step 1: Removing old installation..."
sudo rm -rf "/Library/Audio/Plug-Ins/VST3/Surge XT.vst3"
sudo rm -rf "/Library/Audio/Plug-Ins/VST3/Surge XT.vst3.backup"

echo "Step 2: Copying new build..."
sudo cp -R "build-arm64/src/surge-xt/surge-xt_artefacts/Release/VST3/Surge XT.vst3" "/Library/Audio/Plug-Ins/VST3/"

echo "Step 3: Setting permissions..."
sudo chmod -R 755 "/Library/Audio/Plug-Ins/VST3/Surge XT.vst3"
sudo xattr -cr "/Library/Audio/Plug-Ins/VST3/Surge XT.vst3"

echo "Step 4: Verifying installation..."
if strings "/Library/Audio/Plug-Ins/VST3/Surge XT.vst3/Contents/MacOS/Surge XT" | grep -q "S&S Fork v1.29.0"; then
    echo "✅ SUCCESS: S&S Fork v1.29.0 Host Discovery (with file logging) installed correctly!"
    echo ""
    echo "Installed file info:"
    ls -la "/Library/Audio/Plug-Ins/VST3/Surge XT.vst3/Contents/MacOS/Surge XT"
    echo ""
    echo "IMPORTANT: You must do the following:"
    echo "1. Completely quit Reaper (Cmd+Q)"
    echo "2. Clear plugin cache (run this command):"
    echo "   rm ~/Library/Application\ Support/REAPER/reaper-vstplugins_arm64.ini"
    echo "3. Restart Reaper"
    echo "4. Rescan for VST plugins (Options → Preferences → VST → Re-scan)"
    echo "5. Remove old Surge XT from your track"
    echo "6. Re-add Surge XT to your track"
    echo "7. Click Settings menu - look for 'S&S Fork v1.29.0 Host Discovery'"
else
    echo "❌ ERROR: Installation verification failed!"
fi