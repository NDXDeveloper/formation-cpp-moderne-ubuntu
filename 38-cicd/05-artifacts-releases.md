🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 38.5 Artifacts et gestion des releases

## Introduction

Dans un pipeline C++, le terme "artifact" recouvre deux réalités très différentes. Les **artifacts intra-pipeline** sont des fichiers transitoires — binaires compilés, rapports de test, compilation database — qui circulent entre les jobs d'un même pipeline et n'ont pas vocation à survivre au-delà de quelques heures. Les **artifacts de release** sont les livrables du projet : paquets DEB/RPM, images Docker, binaires statiques, archives sources — des fichiers destinés aux utilisateurs finaux et qui doivent être versionnés, reproductibles et conservés à long terme.

La confusion entre ces deux catégories est une source fréquente de problèmes dans les pipelines C++ : des binaires de test traités comme des livrables (non reproductibles, non versionnés), ou des paquets de release stockés comme des artifacts temporaires (expirés après 24 heures, perdus).

Cette section clarifie cette distinction, détaille les formats de livraison pertinents pour un projet C++, et présente les mécanismes de publication sur GitLab et GitHub.

## Artifacts intra-pipeline : rappel et bonnes pratiques

Les artifacts intra-pipeline ont été couverts dans les sections précédentes (38.1.1, 38.1.2, 38.2.1). Récapitulons les bonnes pratiques spécifiques aux projets C++ :

### Minimiser la taille des artifacts

Un répertoire `build/` CMake complet peut peser des centaines de mégaoctets : fichiers objets, librairies statiques intermédiaires, fichiers CMake internes, compilation database. Seule une fraction est nécessaire pour les jobs suivants.

```yaml
# ❌ Trop large — transfère des centaines de Mo inutiles
artifacts:
  paths:
    - build/

# ✅ Ciblé — seuls les fichiers nécessaires aux tests et au packaging
artifacts:
  paths:
    - build/bin/                        # Exécutables
    - build/lib/                        # Librairies partagées
    - build/tests/                      # Binaires de test
    - build/CTestTestfile.cmake         # Découverte des tests par CTest
    - build/tests/CTestTestfile.cmake   # Idem pour les sous-répertoires
    - build/compile_commands.json       # Pour clang-tidy en aval
```

Sur un projet de taille moyenne, cette sélection réduit l'artifact de 300-500 Mo à 20-50 Mo, accélérant considérablement le transfert entre jobs.

### Durée de rétention adaptée au contexte

| Type d'artifact | Durée recommandée | Justification |
|----------------|-------------------|---------------|
| Binaires intra-pipeline (build → test) | 1-2 heures | Ne servent qu'au pipeline courant |
| Rapports de test (JUnit XML) | 7 jours | Consultables après coup pour diagnostic |
| Rapports de couverture (HTML) | 7-14 jours | Consultables par l'équipe |
| Paquets DEB/RPM de release | 90 jours à permanent | Livrables distribués aux utilisateurs |
| Binaires de release | 90 jours à permanent | Attachés aux releases |

Sur GitLab CI, la durée se configure via `expire_in`. Sur GitHub Actions, via `retention-days` dans `actions/upload-artifact`.

## Formats de livraison pour un projet C++

Un projet C++ peut être distribué sous plusieurs formes, chacune adaptée à un mode de consommation différent.

### Binaires statiques

Un binaire compilé statiquement (`-static` ou linkage statique de toutes les dépendances) est le format le plus simple à distribuer : un seul fichier exécutable sans dépendance externe. L'utilisateur le télécharge, le rend exécutable, et le lance.

```yaml
# Build avec linkage statique
- name: Build static binary
  run: |
    cmake -B build-static \
      -G Ninja \
      -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_EXE_LINKER_FLAGS="-static" \
      -DCMAKE_FIND_LIBRARY_SUFFIXES=".a" \
      -DBUILD_SHARED_LIBS=OFF
    cmake --build build-static --parallel $(nproc)
    strip build-static/bin/mon-application    # Réduire la taille
```

**`strip`** supprime les symboles de debug du binaire, réduisant sa taille de 50-80%. Un binaire C++ Release de 15 Mo peut descendre à 3-5 Mo après strip. Les symboles de debug peuvent être sauvegardés séparément (via `objcopy --only-keep-debug`) pour le debugging post-mortem si nécessaire.

Le linkage statique n'est pas toujours possible : certaines librairies (glibc, NSS) ne se linkent pas facilement en statique. L'alternative est de compiler avec musl libc (via une image Alpine ou un cross-compilateur musl) pour obtenir un binaire véritablement autosuffisant :

```dockerfile
# Build avec musl pour un binaire 100% statique
FROM alpine:3.20 AS builder  
RUN apk add --no-cache g++ cmake ninja-build linux-headers  
COPY . /src  
WORKDIR /src  
RUN cmake -B build -G Ninja \  
      -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_EXE_LINKER_FLAGS="-static" \
    && cmake --build build --parallel $(nproc) \
    && strip build/bin/mon-application
```

### Archives de distribution

Pour les projets qui produisent plusieurs exécutables ou incluent des fichiers de configuration, une archive (tar.gz, zip) est plus adaptée qu'un binaire isolé :

```yaml
- name: Create release archive
  run: |
    VERSION="${GITHUB_REF_NAME#v}"
    ARCHIVE_NAME="mon-projet-${VERSION}-linux-amd64"
    mkdir -p ${ARCHIVE_NAME}/bin ${ARCHIVE_NAME}/etc ${ARCHIVE_NAME}/share

    cp build/bin/* ${ARCHIVE_NAME}/bin/
    cp config/default.yaml ${ARCHIVE_NAME}/etc/
    cp -r share/templates ${ARCHIVE_NAME}/share/
    cp LICENSE README.md ${ARCHIVE_NAME}/

    # Archive tar.gz (Linux)
    tar czf "${ARCHIVE_NAME}.tar.gz" ${ARCHIVE_NAME}/

    # Archive zip (cross-platform)
    zip -r "${ARCHIVE_NAME}.zip" ${ARCHIVE_NAME}/

    # Checksums pour la vérification d'intégrité
    sha256sum "${ARCHIVE_NAME}.tar.gz" "${ARCHIVE_NAME}.zip" > checksums-sha256.txt
```

**Les checksums SHA256** sont une bonne pratique de distribution : ils permettent aux utilisateurs de vérifier l'intégrité du téléchargement. Les publier dans un fichier séparé (ou directement dans les notes de release) est une convention largement adoptée.

### Paquets DEB (Debian/Ubuntu)

La création de paquets DEB a été introduite en section 38.1.2. Voici une version plus complète intégrant les scripts de maintainer et les dépendances :

```yaml
- name: Build DEB package
  run: |
    VERSION="${GITHUB_REF_NAME#v}"
    PKG="mon-projet"
    ARCH="amd64"
    PKG_DIR="pkg/${PKG}_${VERSION}_${ARCH}"

    # Structure du paquet
    mkdir -p ${PKG_DIR}/DEBIAN
    mkdir -p ${PKG_DIR}/usr/bin
    mkdir -p ${PKG_DIR}/etc/mon-projet
    mkdir -p ${PKG_DIR}/lib/systemd/system
    mkdir -p ${PKG_DIR}/usr/share/doc/${PKG}

    # Copie des fichiers
    cp build/bin/mon-application ${PKG_DIR}/usr/bin/
    cp config/default.yaml ${PKG_DIR}/etc/mon-projet/config.yaml
    cp deploy/mon-projet.service ${PKG_DIR}/lib/systemd/system/
    cp LICENSE ${PKG_DIR}/usr/share/doc/${PKG}/copyright

    # Fichier control
    cat > ${PKG_DIR}/DEBIAN/control << EOF
    Package: ${PKG}
    Version: ${VERSION}
    Section: utils
    Priority: optional
    Architecture: ${ARCH}
    Depends: libstdc++6 (>= 13), libc6 (>= 2.39)
    Maintainer: Equipe Dev <dev@exemple.com>
    Description: Mon application C++
     Description longue de l'application
     sur plusieurs lignes.
    EOF

    # Script post-installation
    cat > ${PKG_DIR}/DEBIAN/postinst << 'POSTINST'
    #!/bin/sh
    set -e
    if [ "$1" = "configure" ]; then
        systemctl daemon-reload
        systemctl enable mon-projet.service || true
    fi
    POSTINST
    chmod 755 ${PKG_DIR}/DEBIAN/postinst

    # Construction
    dpkg-deb --build --root-owner-group ${PKG_DIR}
    mv pkg/*.deb .

    # Vérification
    dpkg-deb --info *.deb
    lintian *.deb || true    # Vérification qualité (non bloquant)
```

**`Depends:`** dans le fichier `control` déclare les dépendances runtime du paquet. Pour un binaire C++ linké dynamiquement, les dépendances minimales sont `libstdc++6` (runtime de la Standard Library) et `libc6` (runtime C). Les versions minimales correspondent à celles disponibles sur la distribution cible. La commande `dpkg-shlibdeps` peut calculer automatiquement ces dépendances à partir du binaire :

```bash
dpkg-shlibdeps -O build/bin/mon-application
```

**`--root-owner-group`** force tous les fichiers du paquet à appartenir à root:root, quelle que soit l'identité de l'utilisateur qui construit le paquet. Sans cette option, les fichiers appartiennent à l'utilisateur du runner CI (souvent `root` dans Docker, mais pas toujours), ce qui peut causer des avertissements lintian.

**`lintian`** est l'outil de vérification de qualité des paquets Debian. Il détecte les problèmes de structure, les permissions incorrectes, les descriptions manquantes, etc. L'exécuter en mode non-bloquant (`|| true`) permet de voir les avertissements sans bloquer le pipeline.

### Paquets RPM (RedHat/CentOS/Fedora)

Pour les distributions basées sur RPM, la construction passe par `rpmbuild` et un fichier `.spec` :

```yaml
- name: Build RPM package
  run: |
    VERSION="${GITHUB_REF_NAME#v}"
    sudo apt-get install -y rpm       # rpmbuild sur Ubuntu

    mkdir -p ~/rpmbuild/{BUILD,RPMS,SOURCES,SPECS,SRPMS}

    # Créer l'archive source attendue par rpmbuild
    tar czf ~/rpmbuild/SOURCES/mon-projet-${VERSION}.tar.gz \
      --transform "s,^,mon-projet-${VERSION}/," \
      build/bin/ config/ deploy/ LICENSE

    # Copier le spec file
    sed "s/@VERSION@/${VERSION}/g" packaging/mon-projet.spec \
      > ~/rpmbuild/SPECS/mon-projet.spec

    # Build
    rpmbuild -bb ~/rpmbuild/SPECS/mon-projet.spec
    cp ~/rpmbuild/RPMS/x86_64/*.rpm .
```

Le fichier `.spec` est versionné dans le dépôt (dans un répertoire `packaging/`) avec la version remplacée par un placeholder `@VERSION@` :

```spec
Name:           mon-projet  
Version:        @VERSION@  
Release:        1%{?dist}  
Summary:        Mon application C++  
License:        MIT  
Source0:        %{name}-%{version}.tar.gz  

%description
Description longue de l'application.

%install
mkdir -p %{buildroot}/usr/bin  
mkdir -p %{buildroot}/etc/mon-projet  
cp -a build/bin/mon-application %{buildroot}/usr/bin/  
cp -a config/default.yaml %{buildroot}/etc/mon-projet/config.yaml  

%files
/usr/bin/mon-application
%config(noreplace) /etc/mon-projet/config.yaml
```

### Images Docker

La création d'images Docker a été abordée en section 38.1.2 (GitLab) et 38.2.2 (GitHub). En contexte de release, l'image doit être taguée avec la version sémantique et des tags additionnels pour faciliter le déploiement :

```yaml
- name: Tag and push Docker image
  run: |
    VERSION="${GITHUB_REF_NAME#v}"
    MAJOR=$(echo $VERSION | cut -d. -f1)
    MINOR=$(echo $VERSION | cut -d. -f1-2)
    IMAGE="ghcr.io/${{ github.repository }}"

    # Tag avec la version exacte
    docker tag local-build ${IMAGE}:${VERSION}
    # Tag avec la version mineure (flottant)
    docker tag local-build ${IMAGE}:${MINOR}
    # Tag avec la version majeure (flottant)
    docker tag local-build ${IMAGE}:${MAJOR}
    # Tag latest
    docker tag local-build ${IMAGE}:latest

    docker push ${IMAGE}:${VERSION}
    docker push ${IMAGE}:${MINOR}
    docker push ${IMAGE}:${MAJOR}
    docker push ${IMAGE}:latest
```

Ce pattern de **multi-tagging** est une convention standard dans l'écosystème Docker. L'image `mon-projet:1.2.3` est immuable et pointe toujours vers le même contenu. Les tags `1.2`, `1` et `latest` sont des tags flottants qui pointent vers la dernière release de leur gamme. Un utilisateur qui veut la stabilité utilise `1.2.3` ; un utilisateur qui veut les derniers correctifs sans changement d'API utilise `1.2` ; un utilisateur qui veut toujours la dernière version utilise `latest`.

## Versioning automatique

La gestion de la version est un élément central de la release. Pour un projet C++, la version doit être cohérente entre le binaire, le paquet, l'image Docker et la release.

### Extraction de la version depuis le tag Git

Le pattern le plus fiable est d'utiliser le tag Git comme source unique de vérité :

```yaml
# GitHub Actions
- name: Extract version
  id: version
  run: |
    if [[ "${GITHUB_REF_TYPE}" == "tag" ]]; then
      VERSION="${GITHUB_REF_NAME#v}"       # v1.2.3 → 1.2.3
    else
      VERSION="0.0.0-dev+$(git rev-parse --short HEAD)"
    fi
    echo "version=${VERSION}" >> "$GITHUB_OUTPUT"
    echo "Version : ${VERSION}"
```

```yaml
# GitLab CI
variables:
  VERSION: "${CI_COMMIT_TAG}"

.extract_version: &extract_version
  - |
    if [ -n "${CI_COMMIT_TAG}" ]; then
      VERSION="${CI_COMMIT_TAG#v}"
    else
      VERSION="0.0.0-dev+$(git rev-parse --short HEAD)"
    fi
    echo "VERSION=${VERSION}" >> build.env
```

La version est ensuite injectée dans le build CMake et dans les métadonnées des paquets :

```yaml
- name: CMake configure with version
  run: |
    cmake -B build \
      -DPROJECT_VERSION=${{ steps.version.outputs.version }} \
      -DCMAKE_BUILD_TYPE=Release
```

Dans le `CMakeLists.txt`, cette version peut alimenter un header généré automatiquement :

```cmake
project(mon-projet VERSION ${PROJECT_VERSION})

configure_file(
  "${CMAKE_SOURCE_DIR}/src/version.hpp.in"
  "${CMAKE_BINARY_DIR}/generated/version.hpp"
)
```

```cpp
// version.hpp.in
#pragma once
#define PROJECT_VERSION "@PROJECT_VERSION@"
#define PROJECT_VERSION_MAJOR @PROJECT_VERSION_MAJOR@
#define PROJECT_VERSION_MINOR @PROJECT_VERSION_MINOR@
#define PROJECT_VERSION_PATCH @PROJECT_VERSION_PATCH@
```

Le binaire résultant peut afficher sa version via un flag `--version` :

```cpp
if (args.has("--version")) {
    std::println("mon-application v{}", PROJECT_VERSION);
    return 0;
}
```

### Versioning basé sur `git describe`

Pour les builds intermédiaires (pas sur un tag), `git describe` fournit une version descriptive automatique :

```bash
$ git describe --tags --always
v1.2.3-17-gabcdef0
# Signifie : 17 commits après le tag v1.2.3, au commit abcdef0
```

```yaml
- name: Dynamic version
  id: version
  run: |
    VERSION=$(git describe --tags --always --dirty 2>/dev/null || echo "0.0.0-unknown")
    VERSION="${VERSION#v}"
    echo "version=${VERSION}" >> "$GITHUB_OUTPUT"
```

Le suffixe `--dirty` ajoute `-dirty` si le working tree contient des modifications non commitées — utile pour détecter les builds non reproductibles.

> ⚠️ **`git describe` nécessite l'historique complet.** Sur les runners CI qui effectuent un shallow clone (`fetch-depth: 1`), `git describe` échoue car les tags et l'historique ne sont pas disponibles. Configurez `fetch-depth: 0` dans le step de checkout pour les jobs qui utilisent `git describe`.

## Publication des releases

### Release GitHub

Le workflow complet de publication d'une release GitHub combine la création de la release, l'attachement des assets, et la génération automatique des notes :

```yaml
publish-release:
  runs-on: ubuntu-latest
  needs: [package-deb, package-rpm, build-static, docker-image]
  permissions:
    contents: write
  steps:
    - uses: actions/checkout@v4
      with:
        fetch-depth: 0

    - uses: actions/download-artifact@v4
      with:
        path: release-assets/
        merge-multiple: true

    - name: Generate checksums
      working-directory: release-assets
      run: |
        sha256sum *.deb *.rpm *.tar.gz 2>/dev/null > SHA256SUMS.txt || true

    - name: Determine prerelease status
      id: prerelease
      run: |
        TAG="${GITHUB_REF_NAME}"
        if [[ "$TAG" == *"-rc"* ]] || [[ "$TAG" == *"-beta"* ]] || [[ "$TAG" == *"-alpha"* ]]; then
          echo "is_prerelease=true" >> "$GITHUB_OUTPUT"
        else
          echo "is_prerelease=false" >> "$GITHUB_OUTPUT"
        fi

    - name: Create GitHub Release
      uses: softprops/action-gh-release@v2
      with:
        files: release-assets/*
        generate_release_notes: true
        draft: false
        prerelease: ${{ steps.prerelease.outputs.is_prerelease }}
        body: |
          ## Installation

          ### Debian/Ubuntu
          ```bash
          sudo dpkg -i mon-projet_${{ github.ref_name }}_amd64.deb
          ```

          ### Docker
          ```bash
          docker pull ghcr.io/${{ github.repository }}:${{ github.ref_name }}
          ```

          ### Binaire statique
          ```bash
          curl -LO https://github.com/${{ github.repository }}/releases/download/${{ github.ref_name }}/mon-projet-linux-amd64.tar.gz
          tar xzf mon-projet-linux-amd64.tar.gz
          ./mon-projet/bin/mon-application --version
          ```

          ## Vérification d'intégrité
          ```bash
          sha256sum -c SHA256SUMS.txt
          ```
```

**`merge-multiple: true`** dans `download-artifact` fusionne tous les artifacts téléchargés dans le même répertoire. Chaque job de packaging a uploadé son artifact sous un nom différent (`deb-package`, `rpm-package`, `static-binary`) — cette option les rassemble.

**`generate_release_notes: true`** demande à GitHub de lister automatiquement les pull requests fusionnées depuis le tag précédent. Combiné avec le bloc `body:` qui fournit les instructions d'installation, la release obtenue est à la fois informative (quoi de neuf) et actionnable (comment installer).

**Le statut pre-release** est déterminé par le nom du tag : `v1.2.3-rc1`, `v2.0.0-beta.1` ou `v3.0.0-alpha.2` sont marqués comme pre-release, tandis que `v1.2.3` est une release stable. Les pre-releases ne sont pas affichées comme "Latest release" sur la page du dépôt.

### Release GitLab

Sur GitLab, la release est créée via la directive native `release:` du job CI :

```yaml
create-release:
  stage: deploy
  image: registry.gitlab.com/gitlab-org/release-cli:latest
  needs:
    - job: package-deb
      artifacts: true
    - job: build-static
      artifacts: true
  script:
    - |
      VERSION="${CI_COMMIT_TAG#v}"
      sha256sum *.deb *.tar.gz > SHA256SUMS.txt 2>/dev/null || true
  release:
    tag_name: $CI_COMMIT_TAG
    name: "Release ${CI_COMMIT_TAG}"
    description: |
      ## Installation

      ### Debian/Ubuntu
      ```bash
      sudo dpkg -i mon-projet_${CI_COMMIT_TAG#v}_amd64.deb
      ```

      ### Docker
      ```bash
      docker pull ${CI_REGISTRY_IMAGE}:${CI_COMMIT_TAG}
      ```

      ## Checksums
      Voir le fichier SHA256SUMS.txt dans les assets.
    assets:
      links:
        - name: "Paquet Debian amd64"
          url: "${CI_API_V4_URL}/projects/${CI_PROJECT_ID}/packages/generic/mon-projet/${CI_COMMIT_TAG#v}/mon-projet_${CI_COMMIT_TAG#v}_amd64.deb"
          link_type: package
        - name: "Archive Linux amd64"
          url: "${CI_API_V4_URL}/projects/${CI_PROJECT_ID}/packages/generic/mon-projet/${CI_COMMIT_TAG#v}/mon-projet-linux-amd64.tar.gz"
          link_type: other
        - name: "SHA256 Checksums"
          url: "${CI_API_V4_URL}/projects/${CI_PROJECT_ID}/packages/generic/mon-projet/${CI_COMMIT_TAG#v}/SHA256SUMS.txt"
          link_type: other
  rules:
    - if: '$CI_COMMIT_TAG =~ /^v\d+\.\d+\.\d+/'
```

GitLab n'attache pas directement des fichiers aux releases comme GitHub. Les assets sont des liens vers des fichiers stockés dans le **Generic Package Registry** de GitLab. Un step préalable doit uploader les fichiers vers ce registry :

```yaml
upload-packages:
  stage: package
  script:
    - |
      VERSION="${CI_COMMIT_TAG#v}"
      BASE_URL="${CI_API_V4_URL}/projects/${CI_PROJECT_ID}/packages/generic/mon-projet/${VERSION}"

      # Upload de chaque fichier vers le Package Registry
      for file in *.deb *.tar.gz SHA256SUMS.txt; do
        [ -f "$file" ] || continue
        curl --header "JOB-TOKEN: ${CI_JOB_TOKEN}" \
             --upload-file "$file" \
             "${BASE_URL}/${file}"
      done
  rules:
    - if: '$CI_COMMIT_TAG =~ /^v\d+\.\d+\.\d+/'
```

## Reproductibilité des builds de release

Un livrable de release doit être reproductible : reconstruire exactement le même binaire à partir du même commit et des mêmes dépendances. En C++, cela nécessite de contrôler plusieurs sources de non-déterminisme.

### Sources de non-reproductibilité et remèdes

| Source | Problème | Remède |
|--------|----------|--------|
| Macros `__DATE__`, `__TIME__` | Chaque build produit un binaire différent | `-Wdate-time` (warning) ou `CCACHE_SLOPPINESS=time_macros` |
| Chemin de build absolu | Encodé dans les infos de debug | `-ffile-prefix-map=${PWD}=.` |
| Ordre des fichiers objets | Peut varier selon le filesystem | CMake + Ninja gèrent un ordre déterministe |
| Versions des dépendances | `apt-get install` peut changer de version | Figer les versions dans le Dockerfile ou utiliser Conan avec lockfiles |
| Timestamp dans les archives | `tar` enregistre la date de modification | `tar --mtime="2026-01-01"` |
| Version du compilateur | Optimisations différentes entre versions | Figer la version dans l'image Docker |

Le flag de compilation **`-ffile-prefix-map`** mérite une attention particulière. Il remplace le préfixe du chemin de build dans les informations de debug et les messages d'erreur :

```cmake
# CMakeLists.txt — pour les builds de release reproductibles
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    add_compile_options("-ffile-prefix-map=${CMAKE_SOURCE_DIR}=.")
endif()
```

Sans ce flag, le binaire contient le chemin absolu du répertoire de build (`/home/runner/work/projet/projet`), ce qui rend chaque build sur un runner différent unique même si le code source est identique.

### Lockfiles de dépendances

Les gestionnaires de paquets C++ modernes supportent les lockfiles pour figer les versions exactes des dépendances :

```bash
# Conan — générer un lockfile
conan lock create conanfile.py

# Conan — builder à partir du lockfile
conan install conanfile.py --lockfile=conan.lock
```

Le lockfile (`conan.lock`) est versionné dans le dépôt. Le pipeline de release utilise le lockfile pour installer exactement les mêmes versions de dépendances que celles validées par les tests.

## Processus de release de bout en bout

Voici le workflow complet, du développement à la publication, rassemblant tous les éléments de cette section :

```
Développeur                    Pipeline CI                    Utilisateurs
───────────                    ───────────                    ────────────
                                                              
git tag v1.2.3 ──────────────▶ Déclenche le workflow  
git push --tags                de release  
                                    │
                               ┌────▼────┐
                               │  Build  │  CMake Release
                               │  + Test │  Tests complets
                               └────┬────┘
                                    │
                          ┌─────────┼─────────┐
                          ▼         ▼         ▼
                     ┌─────────┐ ┌──────┐ ┌───────┐
                     │   DEB   │ │ RPM  │ │Docker │
                     │ package │ │  pkg │ │ image │
                     └────┬────┘ └──┬───┘ └──┬────┘
                          │         │        │
                          ▼         ▼        ▼
                     ┌──────────────────────────────┐
                     │    Upload vers registries    │
                     │  - GitHub/GitLab Releases    │
                     │  - Container Registry        │  ──────▶ docker pull
                     │  - Package Registry          │  ──────▶ apt install
                     │  + SHA256SUMS.txt            │  ──────▶ curl + verify
                     └──────────────────────────────┘
```

Le processus est entièrement automatisé : le développeur pousse un tag, et les livrables sont publiés sans intervention humaine. La seule action manuelle est la décision de créer le tag — tout le reste est déterministe et reproductible.

### Checklist de release

Avant de pousser un tag de version, vérifiez :

1. **Le CHANGELOG est à jour** — les modifications depuis la dernière version sont documentées.
2. **La version dans `CMakeLists.txt`** correspond au tag (ou est extraite dynamiquement du tag).
3. **Les tests passent sur `main`** — le dernier pipeline sur la branche principale est vert.
4. **Les dépendances sont figées** — le lockfile Conan est commité et à jour.
5. **Le tag suit le semantic versioning** — `v1.2.3` pour une release, `v1.2.3-rc1` pour une release candidate.

Cette checklist peut être partiellement automatisée via un script ou un job CI déclenché par `workflow_dispatch` qui vérifie ces conditions avant de créer le tag :

```yaml
# GitHub Actions — vérification pré-release
prepare-release:
  runs-on: ubuntu-latest
  if: github.event_name == 'workflow_dispatch'
  steps:
    - uses: actions/checkout@v4
      with:
        fetch-depth: 0

    - name: Validate release readiness
      run: |
        VERSION="${{ github.event.inputs.version }}"

        # Vérifier que le tag n'existe pas déjà
        if git rev-parse "v${VERSION}" >/dev/null 2>&1; then
          echo "ERREUR : Le tag v${VERSION} existe déjà"
          exit 1
        fi

        # Vérifier que le CHANGELOG mentionne cette version
        if ! grep -q "## \[${VERSION}\]" CHANGELOG.md; then
          echo "ERREUR : Version ${VERSION} absente du CHANGELOG.md"
          exit 1
        fi

        # Vérifier que le lockfile est commité
        if [ ! -f conan.lock ]; then
          echo "WARNING : Pas de lockfile Conan trouvé"
        fi

        echo "Prêt pour la release v${VERSION}"
```

---

> **Section suivante** : 38.6 Cross-compilation : ARM, RISC-V depuis x86_64 — Mise en place de la cross-compilation dans le pipeline CI pour produire des binaires ARM64 et RISC-V depuis des runners x86_64.

⏭️ [Cross-compilation : ARM, RISC-V depuis x86_64](/38-cicd/06-cross-compilation.md)
