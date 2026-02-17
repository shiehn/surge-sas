#!/bin/bash

echo "Force Installing Surge XT S&S Fork v2.1.0..."
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
if strings "/Library/Audio/Plug-Ins/VST3/Surge XT.vst3/Contents/MacOS/Surge XT" | grep -q "S&S Fork v2.1.0"; then
    echo "✅ SUCCESS: S&S Fork v2.1.0 installed correctly!"
    echo ""
    echo "Installed file info:"
    ls -la "/Library/Audio/Plug-Ins/VST3/Surge XT.vst3/Contents/MacOS/Surge XT"
    echo ""
    echo "KEY FINDINGS IN v2.1.0:"
    echo "• Shows first 20 preset names in log"
    echo "• Identifies all presets containing 'Lead' or 'Mono'"
    echo "• Logs what's sent in list_presets response"
    echo ""
    echo "THE ISSUE:"
    echo "SAS Assistant is sending 'Solo Lead' and 'Mono Lead'"
    echo "But Surge has presets like 'FM Bass 1', 'Distorted FM', etc."
    echo ""
    echo "SOLUTION:"
    echo "1. Run list_presets to get actual preset names"
    echo "2. SAS Assistant must use EXACT preset names from that list"
    echo "3. No partial matching - names must match exactly"
    echo ""
    echo "Check /tmp/surge-router.log after running list_presets to see"
    echo "all presets containing 'Lead' that are actually available."
else
    echo "❌ ERROR: Installation verification failed!"
fi