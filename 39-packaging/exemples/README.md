# Chapitre 39 — Packaging et Distribution : Exemples

## syswatch/ — Projet central (Sections 39.1 a 39.4)

Projet C++ complet illustrant tout le cycle de packaging : compilation, installation staging, construction .deb, scripts de controle, spec RPM, AppImage, Snap.

### Fichiers C++ et CMake

| Fichier | Section | Fichier source | Description | Sortie attendue |
|---------|---------|----------------|-------------|-----------------|
| `src/main.cpp` | 39.1.3 | `01.3-dpkg-deb.md` | Point d'entree syswatch | `syswatch --version` → `syswatch 1.2.0` ; sans argument → `syswatch: monitoring systeme demarre` |
| `include/syswatch/collector.h` | 39.1.3 | `01.3-dpkg-deb.md` | Header d'interface collecte (placeholder) | N/A (header seul) |
| `CMakeLists.txt` | 39.1.3 | `01.3-dpkg-deb.md` | Configuration CMake avec install() et DESTDIR | Compilation + install staging dans `/usr/bin/`, `/etc/syswatch/`, `/usr/lib/systemd/system/` |

**Compilation et test :**
```bash
cd syswatch/  
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release  
cmake --build build --parallel $(nproc)  
./build/syswatch --version    # syswatch 1.2.0
./build/syswatch              # syswatch: monitoring systeme demarre
```

### Fichiers de configuration et metadonnees DEB

| Fichier | Section | Fichier source | Description | Comportement attendu |
|---------|---------|----------------|-------------|---------------------|
| `config/syswatch.yaml` | 39.1.1 | `01.1-structure-deb.md` | Configuration par defaut (Prometheus, port 9090) | Installe dans `/etc/syswatch/`, protege par conffiles |
| `systemd/syswatch.service` | 39.1.1 | `01.1-structure-deb.md` | Unit systemd pour le daemon syswatch | Installe dans `/usr/lib/systemd/system/` |
| `debian/control` | 39.1.1 | `01.1-structure-deb.md` | Metadonnees du paquet .deb (nom, version, depends) | Valide par `dpkg-deb --info` |
| `debian/conffiles` | 39.1.1 | `01.1-structure-deb.md` | Declaration des fichiers de configuration proteges | `/etc/syswatch/syswatch.yaml` protege lors des mises a jour |

### Scripts de controle (POSIX sh)

| Fichier | Section | Fichier source | Description | Comportement attendu |
|---------|---------|----------------|-------------|---------------------|
| `debian/postinst` | 39.1.2 | `01.2-scripts-controle.md` | Post-installation : cree utilisateur `syswatch`, repertoires `/var/lib/` et `/var/log/`, active le service | Idempotent, compatible dash, teste `$1` (configure/abort-*) |
| `debian/prerm` | 39.1.2 | `01.2-scripts-controle.md` | Pre-desinstallation : arrete et desactive le service | Distingue `remove` (stop+disable) et `upgrade` (stop seul) |
| `debian/postrm` | 39.1.2 | `01.2-scripts-controle.md` | Post-desinstallation : nettoie donnees, logs, utilisateur | `remove` → daemon-reload ; `purge` → suppression complete |

**Verification syntaxe POSIX :**
```bash
dash -n debian/postinst    # Pas d'erreur  
dash -n debian/prerm       # Pas d'erreur  
dash -n debian/postrm      # Pas d'erreur  
```

### Script de build .deb

| Fichier | Section | Fichier source | Description | Comportement attendu |
|---------|---------|----------------|-------------|---------------------|
| `build-deb.sh` | 39.1.3 | `01.3-dpkg-deb.md` | Script complet : compile, staging, strip, DEBIAN/, dpkg-deb | Produit `syswatch_1.2.0-1_amd64.deb` (~4 Ko), affiche contenu et info |

**Execution :**
```bash
./build-deb.sh
# Produit syswatch_1.2.0-1_amd64.deb
dpkg-deb --contents syswatch_1.2.0-1_amd64.deb
# ./usr/bin/syswatch, ./etc/syswatch/syswatch.yaml, ./usr/lib/systemd/system/syswatch.service
```

### Fichier spec RPM

| Fichier | Section | Fichier source | Description | Comportement attendu |
|---------|---------|----------------|-------------|---------------------|
| `syswatch.spec` | 39.2 | `02-paquets-rpm.md` | Spec RPM complete avec %pre/%post/%preun/%postun, macros systemd, %config(noreplace) | Reference — construction dans un conteneur Rocky Linux 9 |

### Configuration nfpm

| Fichier | Section | Fichier source | Description | Comportement attendu |
|---------|---------|----------------|-------------|---------------------|
| `nfpm.yaml` | 39.1.3 | `01.3-dpkg-deb.md` | Config nfpm pour generer .deb et .rpm depuis un meme fichier | Reference — necessite l'outil nfpm |

### Fichiers AppImage

| Fichier | Section | Fichier source | Description | Comportement attendu |
|---------|---------|----------------|-------------|---------------------|
| `appimage/AppRun` | 39.3 | `03-appimage.md` | Script point d'entree de l'AppImage | Configure LD_LIBRARY_PATH et lance syswatch |
| `appimage/syswatch.desktop` | 39.3 | `03-appimage.md` | Fichier .desktop obligatoire pour AppImage | Metadonnees pour integration desktop |

**Verification syntaxe :**
```bash
dash -n appimage/AppRun    # Pas d'erreur
```

### Configuration Snap

| Fichier | Section | Fichier source | Description | Comportement attendu |
|---------|---------|----------------|-------------|---------------------|
| `snap/snapcraft.yaml` | 39.3 | `03-appimage.md` | Config Snap avec confinement strict, plugin cmake, plugs reseau | Reference — construction avec `snapcraft` |

---

## cpack-demo/ — Demo CPack (Section 39.1)

Exemple minimal de generation .deb via CPack integre a CMake.

| Fichier | Section | Fichier source | Description | Sortie attendue |
|---------|---------|----------------|-------------|-----------------|
| `CMakeLists.txt` | 39.1 | `01-paquets-deb.md` | CMake avec CPACK_GENERATOR="DEB" et include(CPack) | `cd build && cpack` produit un .deb |
| `src/main.cpp` | 39.1 | `01-paquets-deb.md` | Binaire minimal pour demo CPack | `syswatch-cpack --version` → `syswatch-cpack 1.2.0` |

**Compilation et test :**
```bash
cd cpack-demo/  
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release  
cmake --build build --parallel $(nproc)  
./build/syswatch --version    # syswatch-cpack 1.2.0
cd build && cpack             # Produit syswatch-cpack-1.2.0-Linux.deb  
dpkg-deb --info syswatch-cpack-1.2.0-Linux.deb  
```

---

## apt-repo/ — Distribution (Section 39.4)

Fichiers de reference pour la mise en place d'un depot APT.

| Fichier | Section | Fichier source | Description | Comportement attendu |
|---------|---------|----------------|-------------|---------------------|
| `distributions` | 39.4 | `04-distribution-apt-snap.md` | Configuration reprepro (conf/distributions) | Reference — pour `reprepro -b /var/www/apt-repo includedeb stable *.deb` |
| `install.sh` | 39.4 | `04-distribution-apt-snap.md` | Script d'installation depuis GitHub Releases | Telecharge et installe le dernier .deb via curl + dpkg |

**Verification syntaxe :**
```bash
sh -n apt-repo/install.sh    # Pas d'erreur
```

---

## Nettoyage

Apres les tests, supprimer les artefacts de compilation et de packaging :

```bash
# syswatch/
cd syswatch/  
rm -rf build syswatch_1.2.0-1_amd64 syswatch_1.2.0-1_amd64.deb pkg-root  

# cpack-demo/
cd ../cpack-demo/  
rm -rf build  
```
