#!/bin/bash
# Install Surge v1.31.0 with PIID State Serialization

echo "Installing Surge XT v1.31.0 with PIID State Serialization..."
echo ""
echo "This version includes:"
echo "  - JSON envelope state serialization"
echo "  - PIID embedded in VST3 state"
echo "  - Live state reading support"
echo "  - Fixed PIID registration format"
echo ""

# Check if build exists
if [ ! -d "build-arm64/src/surge-xt/surge-xt_artefacts/Release/VST3/Surge XT.vst3" ]; then
    echo "❌ ERROR: Build not found. Run: cmake --build build-arm64 --target surge-xt_VST3"
    exit 1
fi

echo "The following command needs sudo access to install to /Library/Audio/Plug-Ins/VST3/"
echo ""
echo "Run this command manually:"
echo ""
echo "sudo rm -rf '/Library/Audio/Plug-Ins/VST3/Surge XT.vst3' && \\"
echo "sudo cp -R 'build-arm64/src/surge-xt/surge-xt_artefacts/Release/VST3/Surge XT.vst3' '/Library/Audio/Plug-Ins/VST3/'"
echo ""
echo "After installation:"
echo "1. Quit REAPER completely (Cmd+Q)"
echo "2. Clear plugin cache:"
echo "   rm ~/Library/Application\\ Support/REAPER/reaper-vstplugins_arm64.ini"
echo "3. Restart REAPER"
echo "4. Rescan VST plugins (Options → Preferences → VST → Re-scan)"
echo "5. Remove and re-add Surge instances to tracks"
echo ""
echo "To verify installation worked:"
echo "1. Run test_complete_system.lua in REAPER"
echo "2. Look for '✅ JSON envelope detected!'"
echo "3. PIIDs should appear after preset changes"