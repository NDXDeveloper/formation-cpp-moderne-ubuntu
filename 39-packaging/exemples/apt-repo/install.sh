#!/bin/sh
# ============================================================================
# Section 39.4 : Distribution via gestionnaires de paquets
# Description : Script d'installation depuis GitHub Releases
# Fichier source : 04-distribution-apt-snap.md
# ============================================================================
set -eu

REPO="exemple/syswatch"
LATEST=$(curl -s "https://api.github.com/repos/${REPO}/releases/latest" \
         | grep -o '"tag_name": *"[^"]*"' | cut -d'"' -f4)
DEB_URL="https://github.com/${REPO}/releases/download/${LATEST}/syswatch_${LATEST#v}-1_amd64.deb"

TMPFILE=$(mktemp)
curl -sL "$DEB_URL" -o "$TMPFILE"
sudo dpkg -i "$TMPFILE"
sudo apt install -f -y
rm -f "$TMPFILE"

echo "syswatch ${LATEST} installe avec succes"
