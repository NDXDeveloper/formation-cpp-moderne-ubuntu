🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 39.3 — AppImage et distribution universelle

## Un binaire, n'importe quelle distribution

---

## Introduction

Les sections 39.1 et 39.2 ont couvert les deux grandes familles de paquets natifs — DEB pour Debian/Ubuntu, RPM pour Red Hat/Fedora. Ces formats sont solidement ancrés dans leurs écosystèmes respectifs, mais ils posent un problème évident : un `.deb` ne s'installe pas sur Fedora, un `.rpm` ne s'installe pas sur Ubuntu. Si votre cible est un parc de serveurs homogène, ce n'est pas un problème. Mais si vous distribuez un outil CLI à une communauté, à des développeurs sur des postes hétérogènes, ou à des clients dont vous ne contrôlez pas l'environnement, maintenir deux (voire trois, quatre) pipelines de packaging devient un fardeau.

Les formats de **distribution universelle** répondent à ce problème en embarquant les dépendances avec l'application, éliminant la dépendance envers les librairies système de la distribution hôte. Le binaire fonctionne sur n'importe quelle distribution Linux suffisamment récente, sans installation, sans droits root, sans gestionnaire de paquets.

Cette section couvre en profondeur **AppImage**, le format le plus pertinent pour distribuer du C++ compilé, puis présente **Snap** et **Flatpak** comme alternatives avec leurs compromis respectifs.

---

## Le problème que ces formats résolvent

Pour comprendre l'intérêt des formats universels, revenons au problème fondamental de la distribution de binaires C++ sur Linux.

Un binaire C++ compilé dynamiquement dépend de la `glibc`, de `libstdc++`, et de toutes les librairies tierces liées au build. Chaque distribution (et chaque version d'une même distribution) embarque des versions différentes de ces librairies. Un binaire compilé sur Ubuntu 24.04 (glibc 2.39) ne fonctionnera pas sur Rocky Linux 9 (glibc 2.34) car il s'attend à des symboles qui n'existent pas dans la version plus ancienne. L'inverse fonctionne généralement — la glibc maintient une compatibilité ascendante — mais ce n'est pas toujours suffisant quand des librairies tierces entrent dans l'équation.

Les paquets natifs gèrent ce problème en déclarant des dépendances de versions, mais ils sont liés à une distribution précise. Les formats universels prennent l'approche inverse : plutôt que de déclarer "j'ai besoin de telle version de telle librairie système", ils disent "j'embarque mes propres librairies, je n'ai besoin de rien".

---

## AppImage : l'approche la plus simple

### Concept

Un AppImage est un fichier unique, autonome et exécutable. Il contient le binaire de l'application, toutes ses librairies dépendantes (sauf les composants de base du système comme la glibc et le noyau), et un petit runtime qui monte le tout dans un filesystem virtuel au lancement. L'utilisateur télécharge le fichier, le rend exécutable, et le lance — pas d'installation, pas de root, pas de gestionnaire de paquets.

```bash
chmod +x syswatch-1.2.0-x86_64.AppImage
./syswatch-1.2.0-x86_64.AppImage
```

C'est tout. C'est cette simplicité qui fait l'attrait du format.

### Fonctionnement interne

Un fichier AppImage est structuré en deux parties concaténées : un **runtime** (un petit exécutable ELF d'environ 180 Ko) suivi d'un **filesystem SquashFS** compressé contenant l'application et ses dépendances. Quand l'utilisateur exécute l'AppImage, le runtime monte le SquashFS via FUSE (Filesystem in Userspace), puis lance le point d'entrée défini dans un fichier de métadonnées appelé `AppRun`.

L'arborescence interne suit la convention **AppDir** :

```
syswatch.AppDir/
├── AppRun                      # Point d'entrée (script ou lien symbolique)
├── syswatch.desktop            # Fichier .desktop (métadonnées desktop)
├── syswatch.png                # Icône (obligatoire, même pour un CLI)
└── usr/
    ├── bin/
    │   └── syswatch            # Binaire principal
    └── lib/
        ├── libssl.so.3         # Dépendances embarquées
        ├── libcrypto.so.3
        └── ...
```

Le fichier `AppRun` est typiquement un script shell qui configure le `LD_LIBRARY_PATH` pour que le linker dynamique trouve les librairies embarquées plutôt que celles du système, puis lance le binaire :

```bash
#!/bin/sh
SELF=$(readlink -f "$0")  
HERE=$(dirname "$SELF")  
export LD_LIBRARY_PATH="${HERE}/usr/lib:${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"  
exec "${HERE}/usr/bin/syswatch" "$@"  
```

### Construire un AppImage pour un projet C++

La construction se fait en trois étapes : préparer un AppDir, y copier les dépendances, puis le convertir en AppImage.

**Étape 1 — Préparer le AppDir**

Repartez de l'installation staging CMake :

```bash
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release  
cmake --build build  
DESTDIR="$(pwd)/syswatch.AppDir" cmake --install build --prefix /usr  
```

Ajoutez les fichiers de métadonnées obligatoires :

```bash
# Fichier desktop (obligatoire même pour un CLI)
cat > syswatch.AppDir/syswatch.desktop << 'EOF'
[Desktop Entry]
Type=Application  
Name=syswatch  
Exec=syswatch  
Icon=syswatch  
Categories=Utility;  
Terminal=true  
EOF  

# Icône (PNG, au moins 256x256 — un placeholder suffit pour un CLI)
# En pratique, utilisez une vraie icône ou générez-en une
convert -size 256x256 xc:steelblue syswatch.AppDir/syswatch.png 2>/dev/null \
    || cp /usr/share/icons/hicolor/256x256/apps/utilities-terminal.png \
          syswatch.AppDir/syswatch.png 2>/dev/null \
    || touch syswatch.AppDir/syswatch.png
```

Le fichier `.desktop` et l'icône peuvent sembler superflus pour un outil CLI, mais le format AppImage les exige. Ils sont utilisés par les environnements de bureau qui intègrent les AppImages dans les menus d'applications.

**Étape 2 — Embarquer les dépendances**

C'est l'étape critique. Il faut copier dans le AppDir toutes les librairies partagées dont le binaire dépend, **sauf** celles qui font partie du système de base (glibc, libpthread, libdl, etc.) — ces dernières doivent venir du système hôte pour assurer la compatibilité avec le noyau.

L'outil **linuxdeploy** automatise ce travail :

```bash
# Télécharger linuxdeploy
wget -q "https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage"  
chmod +x linuxdeploy-x86_64.AppImage  

# Déployer les dépendances dans le AppDir
./linuxdeploy-x86_64.AppImage \
    --appdir syswatch.AppDir \
    --executable syswatch.AppDir/usr/bin/syswatch \
    --desktop-file syswatch.AppDir/syswatch.desktop \
    --icon-file syswatch.AppDir/syswatch.png
```

`linuxdeploy` analyse le binaire avec `ldd`, copie les librairies nécessaires dans `usr/lib/`, génère le script `AppRun`, et exclut automatiquement les librairies système de base via une liste de blocage maintenue par la communauté. Cette liste (souvent appelée *excludelist*) contient les librairies qui doivent provenir du système hôte : `linux-vdso.so`, `ld-linux-x86-64.so`, `libc.so.6`, `libpthread.so.0`, `libdl.so.2`, `libm.so.6`, etc.

**Étape 3 — Générer l'AppImage**

```bash
# Télécharger appimagetool
wget -q "https://github.com/AppImage/appimagetool/releases/download/continuous/appimagetool-x86_64.AppImage"  
chmod +x appimagetool-x86_64.AppImage  

# Construire l'AppImage
./appimagetool-x86_64.AppImage syswatch.AppDir syswatch-1.2.0-x86_64.AppImage
```

Ou bien, `linuxdeploy` peut produire l'AppImage directement si on lui passe l'option `--output appimage` :

```bash
./linuxdeploy-x86_64.AppImage \
    --appdir syswatch.AppDir \
    --executable syswatch.AppDir/usr/bin/syswatch \
    --desktop-file syswatch.AppDir/syswatch.desktop \
    --icon-file syswatch.AppDir/syswatch.png \
    --output appimage
```

Le résultat est un fichier unique :

```bash
$ ls -lh syswatch-1.2.0-x86_64.AppImage
-rwxr-xr-x 1 user user 4.2M ... syswatch-1.2.0-x86_64.AppImage

$ file syswatch-1.2.0-x86_64.AppImage
syswatch-1.2.0-x86_64.AppImage: ELF 64-bit LSB executable, ...
```

### La question de la glibc

Le principal facteur limitant de la portabilité d'un AppImage est la version de la **glibc** du système hôte. Le binaire et les librairies embarquées sont compilés contre une certaine version de la glibc, et le linker dynamique du système hôte doit être compatible. En pratique, cela signifie que votre AppImage fonctionnera sur toute distribution dont la glibc est **égale ou supérieure** à celle utilisée lors de la compilation.

La stratégie recommandée est de compiler sur la distribution la plus ancienne que vous voulez supporter. Si vous ciblez les distributions Linux encore en support en 2026, compiler sur une image basée sur Debian 11 (glibc 2.31) ou Rocky Linux 8 (glibc 2.28) couvrira la grande majorité des systèmes en circulation. Les projets `manylinux` de l'écosystème Python et `holy-build-box` suivent exactement cette logique.

```dockerfile
# Dockerfile de build ciblant une glibc ancienne
FROM debian:11-slim AS builder  
RUN apt-get update && apt-get install -y \  
    g++ cmake ninja-build libssl-dev wget fuse
WORKDIR /src  
COPY . .  
RUN cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release \  
    && cmake --build build
# ... puis les étapes AppImage
```

### Limites du format AppImage

**Pas de mise à jour automatique intégrée.** AppImage propose un mécanisme de mise à jour optionnel basé sur des fichiers `zsync` (delta updates), mais il n'est pas universel. La plupart des utilisateurs téléchargent manuellement la nouvelle version.

**Pas de sandboxing.** Un AppImage s'exécute avec les mêmes permissions que l'utilisateur. Il a accès au système de fichiers, au réseau, et à tout ce que l'utilisateur peut faire. C'est un avantage pour les outils système (pas de restriction artificielle) mais un inconvénient du point de vue de la sécurité par rapport à Snap et Flatpak.

**Taille plus importante.** Embarquer les librairies augmente la taille du fichier. Un binaire C++ qui pèse 500 Ko en paquet DEB peut produire un AppImage de 5 à 15 Mo selon les dépendances. La compression SquashFS aide, mais le surcoût reste notable.

**FUSE requis.** Le runtime AppImage nécessite FUSE pour monter le filesystem SquashFS. FUSE est présent par défaut sur la majorité des distributions desktop, mais peut être absent dans des environnements serveur minimaux ou des conteneurs. L'option `--appimage-extract-and-run` permet de contourner cette limitation en extrayant le contenu dans un répertoire temporaire.

---

## Snap : l'approche Canonical

### Concept

**Snap** est le format de distribution universelle développé par Canonical (l'éditeur d'Ubuntu). Contrairement à AppImage qui produit un fichier autonome, Snap repose sur un écosystème complet : un daemon (`snapd`), un magasin centralisé (Snap Store), et un système de sandboxing basé sur AppArmor et seccomp.

Un snap est un fichier `.snap` (en réalité une image SquashFS) installé et géré par `snapd`. L'installation, les mises à jour, et la désinstallation passent par la commande `snap` :

```bash
sudo snap install syswatch
```

### Création d'un snap pour du C++

La définition d'un snap repose sur un fichier `snapcraft.yaml` à la racine du projet :

```yaml
name: syswatch  
version: '1.2.0'  
summary: Outil de monitoring systeme haute performance  
description: |  
  Syswatch collecte des metriques CPU, memoire et disque
  en temps reel et les expose via un endpoint Prometheus.
base: core24          # Runtime de base (Ubuntu 24.04)  
confinement: strict   # Sandboxing actif  

grade: stable

apps:
  syswatch:
    command: usr/bin/syswatch
    daemon: simple
    plugs:
      - network
      - network-bind
      - system-observe

parts:
  syswatch:
    plugin: cmake
    source: .
    cmake-parameters:
      - -DCMAKE_BUILD_TYPE=Release
      - -G Ninja
    build-packages:
      - g++
      - ninja-build
      - libssl-dev
    stage-packages:
      - libssl3
```

La construction utilise l'outil `snapcraft` :

```bash
snapcraft
```

`snapcraft` lance la compilation dans un conteneur LXD isolé basé sur `core24` (Ubuntu 24.04), puis empaquète le résultat en un fichier `.snap`.

### Avantages de Snap

**Mises à jour automatiques.** `snapd` vérifie et applique les mises à jour en arrière-plan, avec rollback automatique en cas d'échec. Pour un outil déployé sur de nombreuses machines, c'est un avantage opérationnel significatif.

**Sandboxing.** Les snaps tournent dans un confinement strict par défaut. L'application ne peut accéder qu'aux ressources explicitement déclarées via les `plugs` (réseau, fichiers, matériel, etc.). Cela limite la surface d'attaque.

**Canaux de distribution.** Le Snap Store propose des canaux (`stable`, `candidate`, `beta`, `edge`) qui facilitent les stratégies de déploiement progressif.

### Limites de Snap

**`snapd` est obligatoire.** Le daemon doit être installé sur le système cible. Il est préinstallé sur Ubuntu mais nécessite une installation manuelle sur Fedora, Arch, et d'autres distributions — et certaines distributions (Linux Mint, par exemple) le bloquent activement.

**Temps de démarrage.** Le premier lancement d'un snap est plus lent qu'un binaire natif car `snapd` doit monter l'image SquashFS et configurer le confinement. Pour un outil CLI lancé fréquemment, cette latence peut être perceptible.

**Contrôle centralisé.** Le Snap Store est géré par Canonical. Il n'existe pas de moyen officiellement supporté d'héberger un dépôt snap privé, ce qui peut être un frein pour les entreprises qui veulent garder le contrôle de leur chaîne de distribution.

---

## Flatpak : l'approche desktop communautaire

### Concept

**Flatpak** est un format de distribution universelle porté par la communauté (avec un fort soutien de Red Hat et du projet GNOME). Comme Snap, il repose sur un runtime partagé, un système de sandboxing (basé sur Bubblewrap et les namespaces Linux), et un magasin centralisé (Flathub).

Flatpak est conçu principalement pour les **applications graphiques desktop**. Son modèle de sandboxing est centré sur les portails XDG (accès aux fichiers, impression, notifications, etc.) — des concepts qui n'ont pas de sens pour un outil CLI serveur ou un daemon système.

### Pourquoi Flatpak est rarement pertinent pour du C++ système

Pour un outil CLI ou un service de monitoring comme `syswatch`, Flatpak n'est pas le bon choix. Le format n'est pas conçu pour les daemons, ne supporte pas nativement les fichiers unit systemd dans le contexte du sandbox, et ajoute une couche d'abstraction inutile pour des binaires qui ont besoin d'un accès direct au système.

Flatpak excelle pour distribuer des éditeurs de code, des navigateurs, des outils graphiques de développement — des applications de bureau que les développeurs C++ *utilisent* mais ne *distribuent* généralement pas dans ce format. Si votre projet C++ est une application graphique desktop (un éditeur d'images, un outil de visualisation 3D), Flatpak mérite considération. Pour tout le reste, AppImage ou Snap sont plus adaptés.

---

## Comparaison des formats universels

| Critère | AppImage | Snap | Flatpak |
|---|---|---|---|
| **Fichier unique** | Oui | Non (nécessite snapd) | Non (nécessite runtime Flatpak) |
| **Installation** | Aucune (chmod +x) | `snap install` | `flatpak install` |
| **Root requis** | Non | Oui (snapd) | Oui (première installation) |
| **Sandboxing** | Non | Oui (AppArmor) | Oui (Bubblewrap) |
| **Mises à jour auto** | Optionnel (zsync) | Oui (intégré) | Oui (intégré) |
| **Daemons/services** | Oui (pas de restriction) | Oui (mode daemon) | Non adapté |
| **Dépôt privé** | N/A (distribution directe) | Limité | Possible |
| **Adapté aux CLI serveur** | Oui | Oui | Non |
| **Surcoût de taille** | Modéré | Élevé (base snap) | Élevé (runtime) |
| **Distributions supportées** | Toutes (avec FUSE) | Toutes (avec snapd) | Toutes (avec portals) |

---

## Quand utiliser quel format ?

Le choix du format de distribution dépend du contexte de déploiement. Voici un arbre de décision pragmatique.

**Déploiement sur un parc de serveurs homogène** (même distribution, même version) — Utilisez le paquet natif (DEB ou RPM). C'est le format le mieux intégré, le plus léger, et le plus familier pour les équipes d'exploitation. Le packaging natif est détaillé en 39.1 et 39.2.

**Déploiement sur des serveurs hétérogènes** (mix Ubuntu/RHEL, ou versions variées) — Deux options s'offrent à vous. Soit vous maintenez un pipeline DEB + RPM avec nfpm ou des matrix builds CI (voir 39.1.3 et 39.2). Soit vous distribuez un conteneur Docker (chapitre 37), qui est de facto le format universel du monde serveur.

**Distribution d'un outil CLI à des développeurs sur des postes variés** — AppImage est le choix naturel. Un fichier unique, pas d'installation, pas de root, fonctionne partout. Publiez l'AppImage sur GitHub Releases avec un script d'installation optionnel.

**Distribution d'un outil CLI avec mises à jour fréquentes** — Snap offre les mises à jour automatiques et les canaux de release. Le surcoût d'installation de `snapd` est acceptable si vos utilisateurs sont sur Ubuntu (où il est préinstallé) ou acceptent de l'installer.

**Distribution d'une application desktop graphique** — Flatpak si vous ciblez l'écosystème Linux desktop large (Flathub est le standard de fait). Snap si vous ciblez principalement Ubuntu.

**Binaire statique** — Pour les outils CLI simples sans dépendances complexes, la compilation statique (avec `musl` via Alpine Linux ou directement avec `-static`) produit un binaire unique sans aucune dépendance externe. C'est la solution la plus simple quand elle est faisable :

```bash
# Compilation statique avec musl (dans un conteneur Alpine)
docker run --rm -v "$(pwd):/src" -w /src alpine:3.19 sh -c '
    apk add g++ cmake ninja make linux-headers openssl-dev openssl-libs-static
    cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release \
          -DCMAKE_EXE_LINKER_FLAGS="-static"
    cmake --build build
'
```

Le binaire résultant ne dépend de rien — pas même de la glibc. C'est l'approche utilisée par de nombreux outils CLI de l'écosystème Go et Rust, et elle est parfaitement applicable au C++ pour les projets dont les dépendances sont statiquement linkables.

---

## Intégration CI : production d'un AppImage

Voici un extrait de workflow GitHub Actions qui produit un AppImage en compilant sur Debian 11 pour maximiser la compatibilité :

```yaml
jobs:
  build-appimage:
    runs-on: ubuntu-latest
    container: debian:11

    steps:
      - uses: actions/checkout@v4

      - name: Install dependencies
        run: |
          apt-get update
          apt-get install -y g++ cmake ninja-build libssl-dev \
                             wget file fuse libfuse2

      - name: Build
        run: |
          cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
          cmake --build build
          DESTDIR="$(pwd)/syswatch.AppDir" cmake --install build --prefix /usr

      - name: Create AppImage
        run: |
          wget -q https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage
          chmod +x linuxdeploy-x86_64.AppImage
          
          # linuxdeploy nécessite FUSE ou --appimage-extract-and-run en CI
          ./linuxdeploy-x86_64.AppImage --appimage-extract-and-run \
              --appdir syswatch.AppDir \
              --executable syswatch.AppDir/usr/bin/syswatch \
              --desktop-file syswatch.AppDir/syswatch.desktop \
              --icon-file syswatch.AppDir/syswatch.png \
              --output appimage

      - uses: actions/upload-artifact@v4
        with:
          name: syswatch-appimage
          path: "*.AppImage"
```

Le flag `--appimage-extract-and-run` est nécessaire en CI car les runners Docker n'ont généralement pas FUSE monté. Ce flag demande à `linuxdeploy` de s'auto-extraire dans un répertoire temporaire plutôt que de se monter via FUSE.

---

## Résumé

Les formats de distribution universelle résolvent le problème de la fragmentation Linux en embarquant les dépendances avec l'application. AppImage est le format le plus adapté au C++ dans un contexte CLI et serveur : un fichier unique, autonome, sans installation, exécutable partout où FUSE est disponible. Snap ajoute le sandboxing et les mises à jour automatiques au prix d'une dépendance à `snapd`. Flatpak excelle pour les applications desktop mais n'est pas adapté aux outils système.

Pour la plupart des projets C++ couverts par cette formation, le choix se résume à trois options : paquets natifs (DEB/RPM) pour les déploiements serveur contrôlés, conteneurs Docker pour les architectures cloud native, et AppImage pour la distribution d'outils CLI à une audience hétérogène. La section suivante (39.4) clôt le chapitre en montrant comment publier ces artefacts dans des dépôts accessibles aux gestionnaires de paquets.

⏭️ [Distribution via gestionnaires de paquets (apt, snap)](/39-packaging/04-distribution-apt-snap.md)
