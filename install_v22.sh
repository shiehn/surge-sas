#!/bin/bash

echo "Force Installing Surge XT S&S Fork v2.2.0..."
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
if strings "/Library/Audio/Plug-Ins/VST3/Surge XT.vst3/Contents/MacOS/Surge XT" | grep -q "S&S Fork v2.2.0"; then
    echo "✅ SUCCESS: S&S Fork v2.2.0 installed correctly!"
    echo ""
    echo "Installed file info:"
    ls -la "/Library/Audio/Plug-Ins/VST3/Surge XT.vst3/Contents/MacOS/Surge XT"
    echo ""
    echo "🎉 MAJOR FIX IN v2.2.0:"
    echo "• Automatically strips category prefix from preset names"
    echo "• 'Basses/Acid Saw' → searches for 'Acid Saw'"
    echo "• 'Leads/Sync Lead' → searches for 'Sync Lead'"
    echo ""
    echo "This fixes the preset callback failures!"
    echo ""
    echo "The client can now send either:"
    echo "• 'Category/PresetName' (will be stripped)"
    echo "• 'PresetName' (used directly)"
    echo ""
    echo "Both will work correctly!"
else
    echo "❌ ERROR: Installation verification failed!"
fi