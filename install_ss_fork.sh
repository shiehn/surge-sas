#!/bin/bash
echo "Installing Surge XT S&S Fork..."
sudo rm -rf "/Library/Audio/Plug-Ins/VST3/Surge XT.vst3"
sudo cp -r "build-arm64/src/surge-xt/surge-xt_artefacts/Release/VST3/Surge XT.vst3" "/Library/Audio/Plug-Ins/VST3/"
sudo xattr -dr com.apple.quarantine "/Library/Audio/Plug-Ins/VST3/Surge XT.vst3" 2>/dev/null
echo "Done! Restart Reaper and check:"
echo "1. The Settings menu should show: 🔮 Signals & Sorcery Fork (TCP Control Enabled)"
echo "2. The About screen should show the custom version"
echo "3. Check /tmp/surge-tcp.log after loading in Reaper"
