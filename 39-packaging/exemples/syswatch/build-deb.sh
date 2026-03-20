#!/bin/sh
# ============================================================================
# Section 39.1.3 : Construction avec dpkg-deb
# Description : Script complet de construction du paquet .deb syswatch
# Fichier source : 01.3-dpkg-deb.md
# ============================================================================
set -eu

# --- Configuration ---
PKG_NAME="syswatch"
PKG_VERSION="1.2.0"
PKG_REVISION="1"
PKG_ARCH="amd64"
PKG_DIR="${PKG_NAME}_${PKG_VERSION}-${PKG_REVISION}_${PKG_ARCH}"
DEB_FILE="${PKG_DIR}.deb"

# --- Nettoyage ---
rm -rf build "${PKG_DIR}" "${DEB_FILE}"

# --- Compilation ---
cmake -B build -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_CXX_STANDARD=23
cmake --build build

# --- Installation staging ---
DESTDIR="$(pwd)/${PKG_DIR}" cmake --install build --prefix /usr

# --- Stripping ---
strip --strip-unneeded "${PKG_DIR}/usr/bin/${PKG_NAME}"

# --- Metadonnees DEBIAN/ ---
mkdir -p "${PKG_DIR}/DEBIAN"

# Taille installee
SIZE=$(du -sk "${PKG_DIR}" --exclude=DEBIAN | awk '{print $1}')

cat > "${PKG_DIR}/DEBIAN/control" << EOF
Package: ${PKG_NAME}
Version: ${PKG_VERSION}-${PKG_REVISION}
Section: utils
Priority: optional
Architecture: ${PKG_ARCH}
Depends: libc6 (>= 2.35), libstdc++6 (>= 12)
Installed-Size: ${SIZE}
Maintainer: Equipe SRE <sre@exemple.com>
Homepage: https://github.com/exemple/${PKG_NAME}
Description: Outil de monitoring systeme haute performance
 Syswatch collecte des metriques CPU, memoire et disque
 en temps reel et les expose via un endpoint Prometheus.
EOF

# conffiles
printf '/etc/syswatch/syswatch.yaml\n' > "${PKG_DIR}/DEBIAN/conffiles"

# Scripts
cp debian/postinst "${PKG_DIR}/DEBIAN/postinst"
cp debian/postrm   "${PKG_DIR}/DEBIAN/postrm"
chmod 0755 "${PKG_DIR}/DEBIAN/postinst" "${PKG_DIR}/DEBIAN/postrm"

# --- Construction ---
fakeroot dpkg-deb --build "${PKG_DIR}" "${DEB_FILE}"

# --- Verification ---
echo "=== Contenu du paquet ==="
dpkg-deb --contents "${DEB_FILE}"

echo ""
echo "=== Informations du paquet ==="
dpkg-deb --info "${DEB_FILE}"

echo ""
echo "Paquet construit : ${DEB_FILE}"
ls -lh "${DEB_FILE}"
