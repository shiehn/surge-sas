#!/bin/bash

echo "Surge XT Custom Build Installer"
echo "================================"
echo ""
echo "This script will replace your current Surge XT installation with your custom build."
echo "It will create backups of the existing plugins first."
echo ""

# Set the build directory
BUILD_DIR="/Users/stevehiehn/surge-sas/build-arm64"
VST3_DIR="/Library/Audio/Plug-Ins/VST3"

# Check if the custom builds exist
if [ ! -d "$BUILD_DIR/src/surge-xt/surge-xt_artefacts/Release/VST3/Surge XT.vst3" ]; then
    echo "Error: Cannot find Surge XT.vst3 in build directory!"
    exit 1
fi

if [ ! -d "$BUILD_DIR/src/surge-fx/surge-fx_artefacts/Release/VST3/Surge XT Effects.vst3" ]; then
    echo "Error: Cannot find Surge XT Effects.vst3 in build directory!"
    exit 1
fi

echo "Step 1: Creating backups of existing plugins..."
if [ -d "$VST3_DIR/Surge XT.vst3" ]; then
    sudo cp -r "$VST3_DIR/Surge XT.vst3" "$VST3_DIR/Surge XT.vst3.backup"
    echo "  ✓ Backed up Surge XT.vst3"
fi

if [ -d "$VST3_DIR/Surge XT Effects.vst3" ]; then
    sudo cp -r "$VST3_DIR/Surge XT Effects.vst3" "$VST3_DIR/Surge XT Effects.vst3.backup"
    echo "  ✓ Backed up Surge XT Effects.vst3"
fi

echo ""
echo "Step 2: Removing existing plugins..."
sudo rm -rf "$VST3_DIR/Surge XT.vst3"
sudo rm -rf "$VST3_DIR/Surge XT Effects.vst3"
echo "  ✓ Removed existing plugins"

echo ""
echo "Step 3: Installing custom builds..."
# Try the products directory first, then fall back to artefacts
if [ -d "$BUILD_DIR/surge_xt_products/Surge XT.vst3" ]; then
    sudo cp -r "$BUILD_DIR/surge_xt_products/Surge XT.vst3" "$VST3_DIR/"
else
    sudo cp -r "$BUILD_DIR/src/surge-xt/surge-xt_artefacts/Release/VST3/Surge XT.vst3" "$VST3_DIR/"
fi

if [ -d "$BUILD_DIR/surge_xt_products/Surge XT Effects.vst3" ]; then
    sudo cp -r "$BUILD_DIR/surge_xt_products/Surge XT Effects.vst3" "$VST3_DIR/"
else
    sudo cp -r "$BUILD_DIR/src/surge-fx/surge-fx_artefacts/Release/VST3/Surge XT Effects.vst3" "$VST3_DIR/"
fi
echo "  ✓ Installed custom Surge XT.vst3"
echo "  ✓ Installed custom Surge XT Effects.vst3"

echo ""
echo "Step 4: Removing quarantine attributes (prevents macOS security warnings)..."
sudo xattr -dr com.apple.quarantine "$VST3_DIR/Surge XT.vst3" 2>/dev/null
sudo xattr -dr com.apple.quarantine "$VST3_DIR/Surge XT Effects.vst3" 2>/dev/null
echo "  ✓ Quarantine attributes removed"

echo ""
echo "Installation complete!"
echo ""
echo "Your custom Surge XT build with TCP control API has been installed."
echo ""
echo "To restore the original version, run:"
echo "  sudo mv '$VST3_DIR/Surge XT.vst3.backup' '$VST3_DIR/Surge XT.vst3'"
echo "  sudo mv '$VST3_DIR/Surge XT Effects.vst3.backup' '$VST3_DIR/Surge XT Effects.vst3'"