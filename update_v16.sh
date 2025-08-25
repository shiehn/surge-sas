#!/bin/bash
echo "Quick update of Surge v1.6.0 with heartbeat fix"
sudo cp -R "/Users/stevehiehn/surge-sas/build-arm64/src/surge-xt/surge-xt_artefacts/Release/VST3/Surge XT.vst3" "/Library/Audio/Plug-Ins/VST3/"
echo "✅ Updated! Please restart REAPER"