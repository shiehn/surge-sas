#!/bin/bash

echo "===================================="
echo "Surge XT v1.6.0 Installation Script"
echo "===================================="
echo ""

# Check if running as root
if [ "$EUID" -ne 0 ]; then 
    echo "Please run with sudo: sudo ./install_v16.sh"
    exit 1
fi

echo "Step 1: Verifying v1.6.0 build exists..."
if [ ! -f "/Users/stevehiehn/surge-sas/build-arm64/src/surge-xt/surge-xt_artefacts/Release/VST3/Surge XT.vst3/Contents/MacOS/Surge XT" ]; then
    echo "ERROR: v1.6.0 build not found!"
    exit 1
fi

if strings "/Users/stevehiehn/surge-sas/build-arm64/src/surge-xt/surge-xt_artefacts/Release/VST3/Surge XT.vst3/Contents/MacOS/Surge XT" | grep -q "S&S Fork v1.6.0"; then
    echo "✓ Found v1.6.0 build"
else
    echo "ERROR: Build is not v1.6.0!"
    exit 1
fi

echo ""
echo "Step 2: Backing up current installation..."
if [ -d "/Library/Audio/Plug-Ins/VST3/Surge XT.vst3" ]; then
    rm -rf "/Library/Audio/Plug-Ins/VST3/Surge XT.vst3.old"
    mv "/Library/Audio/Plug-Ins/VST3/Surge XT.vst3" "/Library/Audio/Plug-Ins/VST3/Surge XT.vst3.old"
    echo "✓ Backed up old version"
fi

echo ""
echo "Step 3: Installing v1.6.0..."
cp -R "/Users/stevehiehn/surge-sas/build-arm64/src/surge-xt/surge-xt_artefacts/Release/VST3/Surge XT.vst3" "/Library/Audio/Plug-Ins/VST3/"
echo "✓ Copied new version"

echo ""
echo "Step 4: Setting permissions..."
chmod -R 755 "/Library/Audio/Plug-Ins/VST3/Surge XT.vst3"
chown -R root:wheel "/Library/Audio/Plug-Ins/VST3/Surge XT.vst3"
xattr -cr "/Library/Audio/Plug-Ins/VST3/Surge XT.vst3"
echo "✓ Permissions set"

echo ""
echo "Step 5: Verifying installation..."
if strings "/Library/Audio/Plug-Ins/VST3/Surge XT.vst3/Contents/MacOS/Surge XT" | grep -q "S&S Fork v1.6.0"; then
    echo "✅ SUCCESS! v1.6.0 is now installed"
    echo ""
    strings "/Library/Audio/Plug-Ins/VST3/Surge XT.vst3/Contents/MacOS/Surge XT" | grep "Fork v" | head -1
    echo ""
    ls -la "/Library/Audio/Plug-Ins/VST3/Surge XT.vst3/Contents/MacOS/Surge XT"
else
    echo "❌ Installation verification failed!"
    exit 1
fi

echo ""
echo "Step 6: Clearing Reaper cache..."
rm -f /Users/stevehiehn/Library/Application\ Support/REAPER/reaper-vstplugins_arm64.ini
echo "✓ Cache cleared"

echo ""
echo "===================================="
echo "Installation Complete!"
echo "===================================="
echo ""
echo "🎉 v1.6.0 with Smart Broker Support!"
echo ""
echo "Key Changes:"
echo "- Generates 128-bit plugin_sig on startup"
echo "- Sends registration with plugin signature"
echo "- Compatible with multi-REAPER broker architecture"
echo "- No more instance ID issues!"
echo ""
echo "Registration format:"
echo '{"type":"register","plugin_sig":"<128-bit>","plugin_version":"1.6.0","plugin_type":"surge-xt-sas","plugin_name":"Surge XT [S&S Fork]","build":"vst3"}'
echo ""
echo "Now you must:"
echo "1. Completely quit Reaper (Cmd+Q)"
echo "2. Restart Reaper"
echo "3. Let it rescan VST plugins"
echo "4. The broker will handle instance matching via probe/response"
echo ""