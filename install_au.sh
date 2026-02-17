#!/bin/bash

echo "Installing Surge XT AU component..."

# Check if AU component exists
if [ -d "build-arm64/src/surge-xt/surge-xt_artefacts/Release/AU/Surge XT.component" ]; then
    # Backup existing if present
    if [ -d "/Library/Audio/Plug-Ins/Components/Surge XT.component" ]; then
        echo "Backing up existing AU component..."
        sudo mv "/Library/Audio/Plug-Ins/Components/Surge XT.component" "/Library/Audio/Plug-Ins/Components/Surge XT.component.backup"
    fi
    
    # Install new AU
    echo "Installing new AU component..."
    sudo cp -R "build-arm64/src/surge-xt/surge-xt_artefacts/Release/AU/Surge XT.component" "/Library/Audio/Plug-Ins/Components/"
    
    # Remove quarantine
    sudo xattr -rd com.apple.quarantine "/Library/Audio/Plug-Ins/Components/Surge XT.component" 2>/dev/null
    
    echo "✓ AU component installed successfully"
else
    echo "Error: AU component not found in build directory"
    exit 1
fi