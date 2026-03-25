#!/bin/bash

# Sysmon Installation Script
# This script installs the new Sysmon and removes the old version (sysmon-v3).

set -e

# 1. Define paths
PROJECT_DIR="$(pwd)"
NEW_BINARY="$PROJECT_DIR/sysmon"
NEW_ICON="$PROJECT_DIR/sysmon.png"
OLD_BINARY="/usr/local/bin/sysmon-v3"
OLD_DESKTOP="$HOME/.local/share/applications/sysmon-v3.desktop"
NEW_DESKTOP_SRC="$PROJECT_DIR/Sysmon.desktop"
NEW_DESKTOP_DEST="$HOME/.local/share/applications/sysmon.desktop"
INSTALL_BINARY="/usr/local/bin/sysmon"
ICON_DEST="/usr/share/icons/hicolor/512x512/apps/sysmon.png"

echo "Starting Sysmon installation..."

# 2. Check for required files
if [ ! -f "$NEW_BINARY" ]; then
    echo "Error: $NEW_BINARY not found. Please build the project first."
    exit 1
fi

# 3. Remove old version (requires sudo for binary)
echo "Removing old version..."
if [ -f "$OLD_BINARY" ]; then
    sudo rm -f "$OLD_BINARY"
    echo "Deleted $OLD_BINARY"
fi
if [ -f "$OLD_DESKTOP" ]; then
    rm -f "$OLD_DESKTOP"
    echo "Deleted $OLD_DESKTOP"
fi

# 4. Install new binary
echo "Installing new binary to $INSTALL_BINARY..."
sudo cp "$NEW_BINARY" "$INSTALL_BINARY"
sudo chmod +x "$INSTALL_BINARY"

# 5. Install icon
echo "Installing icon..."
sudo mkdir -p "$(dirname "$ICON_DEST")"
sudo cp "$NEW_ICON" "$ICON_DEST"

# 6. Install desktop file with absolute paths
echo "Configuring desktop menu entry..."
sed -e "s|Exec=sysmon|Exec=$INSTALL_BINARY|" \
    -e "s|Icon=sysmon|Icon=$ICON_DEST|" \
    "$NEW_DESKTOP_SRC" > "$NEW_DESKTOP_DEST"
chmod +x "$NEW_DESKTOP_DEST"

# 7. Update desktop database
if command -v update-desktop-database >/dev/null 2>&1; then
    update-desktop-database "$HOME/.local/share/applications"
fi

echo "--------------------------------------------------"
echo "Installation Complete!"
echo "You can now launch 'Sysmon' from your system menu."
echo "--------------------------------------------------"
