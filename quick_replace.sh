#!/bin/bash
echo "Quick Surge v1.1.0 replacement"
echo "=============================="

# Verify we have the right build
if strings "build-arm64/surge_xt_products/Surge XT.vst3/Contents/MacOS/Surge XT" 2>/dev/null | grep -q "S&S Fork v1.1.0"; then
    echo "✓ Found v1.1.0 build"
else
    echo "ERROR: v1.1.0 build not found in build-arm64/surge_xt_products/"
    exit 1
fi

echo ""
echo "Installing (needs sudo)..."
sudo rm -rf "/Library/Audio/Plug-Ins/VST3/Surge XT.vst3"
sudo cp -r "build-arm64/surge_xt_products/Surge XT.vst3" "/Library/Audio/Plug-Ins/VST3/"
sudo xattr -cr "/Library/Audio/Plug-Ins/VST3/Surge XT.vst3" 2>/dev/null

# Verify installation
echo ""
if strings "/Library/Audio/Plug-Ins/VST3/Surge XT.vst3/Contents/MacOS/Surge XT" | grep -q "S&S Fork v1.1.0"; then
    echo "✅ SUCCESS! v1.1.0 installed"
    strings "/Library/Audio/Plug-Ins/VST3/Surge XT.vst3/Contents/MacOS/Surge XT" | grep "Fork v" | head -1
    echo ""
    echo "Now: Quit Reaper → Clear cache → Restart → Rescan VST"
else
    echo "❌ Installation failed!"
fi