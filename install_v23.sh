#!/bin/bash

echo "Force Installing Surge XT S&S Fork v2.3.0..."
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
if strings "/Library/Audio/Plug-Ins/VST3/Surge XT.vst3/Contents/MacOS/Surge XT" | grep -q "S&S Fork v2.3.0"; then
    echo "✅ SUCCESS: S&S Fork v2.3.0 installed correctly!"
    echo ""
    echo "Installed file info:"
    ls -la "/Library/Audio/Plug-Ins/VST3/Surge XT.vst3/Contents/MacOS/Surge XT"
    echo ""
    echo "🎯 KEY IMPROVEMENTS IN v2.3.0:"
    echo ""
    echo "RESPONSE FORMAT FIXES:"
    echo "• All responses now include type:'response' for broker routing"
    echo "• Plugin signature included in every response"
    echo "• Request IDs properly preserved for correlation"
    echo ""
    echo "LIST_PRESETS SUPPORT:"
    echo "• Returns full preset list (3000+ presets)"
    echo "• Response format: {type:'response', ok:true, data:{presets:[...]}}"
    echo "• Supports PresetCacheManager integration"
    echo ""
    echo "MESSAGE FLOW:"
    echo "1. Client → Broker: {op:'list_presets', request_id:'req_123'}"
    echo "2. Broker → Surge: {type:'command', op:'list_presets', request_id:'req_123'}"
    echo "3. Surge → Broker: {type:'response', ok:true, plugin_sig:'...', request_id:'req_123', data:{presets:[...]}}"
    echo "4. Broker → Client: Routes response back using plugin_sig"
    echo ""
    echo "This enables the AI-driven preset selection system!"
else
    echo "❌ ERROR: Installation verification failed!"
fi