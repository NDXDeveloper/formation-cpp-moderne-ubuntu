🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 39.2 — Création de paquets RPM (RedHat/CentOS)

## L'autre grande famille de paquets Linux

---

## Introduction

Si le format DEB domine l'écosystème Debian et Ubuntu, le format **RPM** (Red Hat Package Manager) règne sur l'autre moitié du monde Linux : Red Hat Enterprise Linux (RHEL), Fedora, CentOS Stream, Rocky Linux, AlmaLinux, openSUSE, et Amazon Linux. Dans un contexte professionnel, il est rare de ne cibler qu'une seule famille de distributions. Les serveurs de production d'une même entreprise mélangent souvent Ubuntu et RHEL, les images cloud varient selon les équipes, et un outil CLI interne doit parfois couvrir les deux écosystèmes.

Cette section ne vise pas à faire de vous un mainteneur de paquets RPM — ce serait un chapitre entier en soi. L'objectif est de vous donner une compréhension solide du format, de ses différences structurelles avec DEB, et un workflow complet pour produire un `.rpm` fonctionnel à partir du même projet C++ que celui packagé en DEB dans la section 39.1.

---

## DEB vs RPM : différences fondamentales

Si vous venez de lire les sections 39.1.x sur les paquets DEB, vous avez un modèle mental en place. Le format RPM repose sur les mêmes principes — métadonnées, dépendances, scripts de cycle de vie, arborescence de fichiers — mais avec une implémentation et une philosophie sensiblement différentes.

### Le fichier spec : tout en un seul endroit

La différence la plus structurante est l'approche centralisée du RPM. Là où un paquet DEB distribue ses métadonnées dans plusieurs fichiers (`control`, `conffiles`, `postinst`, `postrm`, etc.) au sein du répertoire `DEBIAN/`, un paquet RPM concentre **toute** la description — métadonnées, dépendances, instructions de compilation, scripts de cycle de vie, et liste des fichiers — dans un unique fichier `.spec`. Ce fichier est à la fois la recette de construction et la documentation du paquet.

### Base de données et transactions

RPM maintient une base de données locale (historiquement BerkeleyDB, remplacée par SQLite depuis Fedora 33 et RHEL 9) qui enregistre chaque fichier installé par chaque paquet, avec ses permissions, son checksum, et ses métadonnées. Cette base permet à RPM de vérifier l'intégrité des fichiers installés (`rpm -V`), de détecter les fichiers modifiés, et de résoudre les conflits de manière transactionnelle.

### Gestionnaires de paquets de haut niveau

`rpm` est l'outil bas niveau (équivalent de `dpkg`). Les gestionnaires de haut niveau qui résolvent les dépendances et gèrent les dépôts sont **dnf** (Fedora, RHEL 9+, CentOS Stream) et **yum** (RHEL 7/8, ancienne génération). Le rapport `dnf`/`rpm` est l'exact analogue de `apt`/`dpkg` dans le monde Debian.

### Tableau de correspondance rapide

| Concept | Debian/Ubuntu | Red Hat/Fedora |
|---|---|---|
| Format de paquet | `.deb` | `.rpm` |
| Outil bas niveau | `dpkg` | `rpm` |
| Outil haut niveau | `apt` | `dnf` (ou `yum`) |
| Fichier de métadonnées | `DEBIAN/control` | `fichier.spec` |
| Scripts de cycle de vie | `postinst`, `prerm`, etc. | Sections `%pre`, `%post`, etc. dans le `.spec` |
| Vérification de conformité | `lintian` | `rpmlint` |
| Construction | `dpkg-deb --build` | `rpmbuild` |
| Protection des configs | `conffiles` | Directive `%config(noreplace)` |
| Dépendances auto | `dpkg-shlibdeps` | `rpmbuild` AutoReq (automatique) |

---

## Prérequis : construire un RPM depuis Ubuntu

La formation étant centrée sur Ubuntu, deux approches s'offrent à vous pour construire des paquets RPM.

### Approche recommandée : conteneur Docker

L'approche la plus fiable et la plus proche de la réalité CI est de construire le RPM dans un conteneur Fedora ou Rocky Linux :

```bash
docker run --rm -v "$(pwd):/src" -w /src rockylinux:9 bash -c '
    dnf install -y rpm-build rpmdevtools gcc-c++ cmake ninja-build \
                   openssl-devel rpmlint
    rpmdev-setuptree
    # ... construction du RPM
'
```

C'est l'approche que nous utiliserons dans les exemples. Elle garantit que le RPM est construit avec les librairies et les outils de la distribution cible — condition essentielle pour que les dépendances automatiques soient correctes.

### Approche alternative : outils cross-packaging

Il est techniquement possible de construire un RPM sur Ubuntu en installant `rpm` et `rpmbuild` :

```bash
sudo apt install rpm rpmlint
```

Cela fonctionne pour produire l'archive RPM, mais les dépendances automatiques seront calculées par rapport aux librairies Ubuntu, pas celles de la distribution cible. Le paquet résultant peut déclarer des dépendances inexistantes sur RHEL. Cette approche convient pour tester la mécanique du packaging, pas pour produire des paquets de production.

---

## L'arborescence de build RPM

L'outil `rpmbuild` attend une arborescence standardisée. La commande `rpmdev-setuptree` la crée dans `~/rpmbuild/` :

```bash
$ rpmdev-setuptree
$ tree ~/rpmbuild
/home/user/rpmbuild/
├── BUILD/          # Répertoire de compilation (utilisé par rpmbuild)
├── RPMS/           # Paquets RPM binaires générés
├── SOURCES/        # Archives sources (.tar.gz) et patches
├── SPECS/          # Fichiers .spec
└── SRPMS/          # Paquets RPM source (SRPM)
```

Chaque répertoire a un rôle précis. **`SOURCES/`** contient l'archive tar du code source. **`SPECS/`** contient le fichier `.spec` qui décrit toute la recette. **`BUILD/`** est le répertoire de travail où `rpmbuild` décompresse les sources et lance la compilation. **`RPMS/`** reçoit le paquet binaire final. **`SRPMS/`** reçoit le paquet source (un `.src.rpm` qui contient le `.spec` et l'archive source — l'équivalent d'un paquet source Debian).

---

## Le fichier .spec : anatomie complète

Le fichier `.spec` est le cœur du packaging RPM. Reprenons le projet `syswatch` de la section 39.1 et écrivons son fichier spec :

```spec
# ============================================================
# syswatch.spec — Paquet RPM pour syswatch
# ============================================================

Name:           syswatch  
Version:        1.2.0  
Release:        1%{?dist}  
Summary:        Outil de monitoring systeme haute performance  

License:        Proprietary  
URL:            https://github.com/exemple/syswatch  
Source0:        %{name}-%{version}.tar.gz  

BuildRequires:  gcc-c++ >= 12  
BuildRequires:  cmake >= 3.25  
BuildRequires:  ninja-build  
BuildRequires:  openssl-devel >= 3.0  

Requires:       openssl-libs >= 3.0

%description
Syswatch collecte des metriques CPU, memoire et disque  
en temps reel et les expose via un endpoint Prometheus.  

Concu pour les environnements de production Linux.

# ============================================================
# Preparation : decompression des sources
# ============================================================
%prep
%autosetup -n %{name}-%{version}

# ============================================================
# Compilation
# ============================================================
%build
%cmake -G Ninja -DCMAKE_BUILD_TYPE=Release
%cmake_build

# ============================================================
# Installation dans le buildroot
# ============================================================
%install
%cmake_install

# Fichiers supplementaires non geres par CMake install()
install -Dm 0644 config/syswatch.yaml \
    %{buildroot}/etc/syswatch/syswatch.yaml

install -Dm 0644 systemd/syswatch.service \
    %{buildroot}%{_unitdir}/syswatch.service

# Repertoires vides a creer
install -d -m 0750 %{buildroot}/var/lib/syswatch  
install -d -m 0750 %{buildroot}/var/log/syswatch  

# ============================================================
# Scripts de cycle de vie
# ============================================================
%pre
# Avant installation : creer l'utilisateur systeme
getent group syswatch > /dev/null 2>&1 || groupadd -r syswatch  
getent passwd syswatch > /dev/null 2>&1 || \  
    useradd -r -g syswatch -d /nonexistent -s /sbin/nologin \
            -c "Syswatch monitoring agent" syswatch

%post
# Apres installation : activer et demarrer le service
%systemd_post syswatch.service

%preun
# Avant desinstallation : arreter et desactiver le service
%systemd_preun syswatch.service

%postun
# Apres desinstallation : recharger systemd
%systemd_postun_with_restart syswatch.service

# ============================================================
# Liste des fichiers installes
# ============================================================
%files
%license LICENSE
%doc README.md

%{_bindir}/syswatch
%{_unitdir}/syswatch.service

%dir /etc/syswatch
%config(noreplace) /etc/syswatch/syswatch.yaml

%attr(0750, syswatch, syswatch) %dir /var/lib/syswatch
%attr(0750, syswatch, syswatch) %dir /var/log/syswatch

# ============================================================
# Changelog
# ============================================================
%changelog
* Tue Mar 17 2026 Equipe SRE <sre@exemple.com> - 1.2.0-1
- Version initiale du paquet RPM
- Support Prometheus endpoint
- Configuration hot-reload via SIGHUP
```

Ce fichier est dense. Détaillons chaque section.

---

## En-tête : les métadonnées

```spec
Name:           syswatch  
Version:        1.2.0  
Release:        1%{?dist}  
```

**`Name`**, **`Version`** et **`Release`** forment le triplet d'identification du paquet — l'équivalent du champ `Package` et `Version` du `control` DEB. La macro `%{?dist}` est un suffixe de distribution ajouté automatiquement par le système de build : `.el9` pour RHEL 9, `.fc39` pour Fedora 39. Le paquet résultant s'appellera `syswatch-1.2.0-1.el9.x86_64.rpm`.

**`BuildRequires`** liste les paquets nécessaires à la **compilation** — l'équivalent des `Build-Depends` d'un paquet source Debian. Sur RHEL/Fedora, les paquets de développement sont suffixés `-devel` (et non `-dev` comme sur Debian).

**`Requires`** liste les dépendances à l'exécution. Contrairement au format DEB où `dpkg-shlibdeps` doit être invoqué manuellement, RPM détecte automatiquement les dépendances de librairies partagées lors du build (via le mécanisme AutoReq). Il n'est donc souvent nécessaire de déclarer explicitement que les dépendances non-librairies (un interpréteur, un outil externe, etc.). Les dépendances sur les `.so` sont ajoutées automatiquement.

---

## Sections de build : %prep, %build, %install

```spec
%prep
%autosetup -n %{name}-%{version}
```

**`%prep`** prépare les sources. La macro `%autosetup` décompresse automatiquement l'archive `Source0` et applique les éventuels patches. L'option `-n` spécifie le nom du répertoire créé par l'extraction.

```spec
%build
%cmake -G Ninja -DCMAKE_BUILD_TYPE=Release
%cmake_build
```

**`%build`** lance la compilation. Les macros `%cmake` et `%cmake_build` sont fournies par le paquet `cmake-rpm-macros` et encapsulent les appels CMake avec les flags standards de la distribution (répertoires d'installation, flags de compilation, etc.). Elles sont l'équivalent d'appeler `cmake -B` et `cmake --build` avec les bonnes options pour Fedora ou RHEL.

```spec
%install
%cmake_install
```

**`%install`** place les fichiers dans le **buildroot** — un répertoire temporaire analogue au `DESTDIR` utilisé en 39.1.3. La variable `%{buildroot}` pointe vers ce répertoire. `rpmbuild` l'utilise ensuite pour créer l'archive de données du RPM.

La commande `install -Dm` qui suit illustre un point important : les fichiers qui ne sont pas gérés par `cmake --install` (ici la configuration YAML et le fichier unit systemd) doivent être installés manuellement dans le buildroot. L'option `-D` crée les répertoires parents si nécessaires, et `-m` fixe les permissions.

---

## Scripts de cycle de vie

Les scripts RPM remplissent le même rôle que les scripts DEB (`preinst`, `postinst`, `prerm`, `postrm`) mais avec des noms et une syntaxe différents.

| Script DEB | Section RPM | Moment d'exécution |
|---|---|---|
| `preinst` | `%pre` | Avant installation des fichiers |
| `postinst` | `%post` | Après installation des fichiers |
| `prerm` | `%preun` | Avant désinstallation des fichiers |
| `postrm` | `%postun` | Après désinstallation des fichiers |

### Création de l'utilisateur système

```spec
%pre
getent group syswatch > /dev/null 2>&1 || groupadd -r syswatch  
getent passwd syswatch > /dev/null 2>&1 || \  
    useradd -r -g syswatch -d /nonexistent -s /sbin/nologin \
            -c "Syswatch monitoring agent" syswatch
```

Remarquez que sur RHEL/Fedora, les commandes de gestion d'utilisateurs sont `useradd`/`groupadd` (avec l'option `-r` pour créer un compte système), là où Debian utilise `adduser --system`. Les deux produisent le même résultat — un utilisateur système sans shell interactif — mais la syntaxe diffère.

Contrairement au DEB où nous créions l'utilisateur dans le `postinst` (après l'installation des fichiers), la convention RPM est de le faire dans le `%pre` (avant). La raison : RPM peut attribuer les permissions des fichiers installés à cet utilisateur directement via la directive `%attr` de la section `%files`. Si l'utilisateur n'existe pas au moment de l'installation des fichiers, RPM émet un warning.

### Macros systemd

```spec
%post
%systemd_post syswatch.service

%preun
%systemd_preun syswatch.service

%postun
%systemd_postun_with_restart syswatch.service
```

Les macros `%systemd_post`, `%systemd_preun` et `%systemd_postun_with_restart` sont fournies par le paquet `systemd-rpm-macros`. Elles encapsulent les appels `systemctl daemon-reload`, `systemctl enable`, `systemctl preset`, `systemctl disable` et `systemctl stop` avec la gestion correcte des mises à jour et des premières installations. Leur utilisation est fortement recommandée plutôt que d'appeler `systemctl` manuellement — elles gèrent des cas limites (comme les presets de distribution) que vous oublieriez autrement.

La macro `%systemd_postun_with_restart` redémarre automatiquement le service après une mise à jour du paquet — le comportement souhaité pour la plupart des services. Si vous préférez ne pas redémarrer automatiquement (pour laisser l'administrateur contrôler le moment), utilisez `%systemd_postun` à la place.

---

## La section %files : déclaration explicite

C'est la différence la plus contraignante avec DEB. Dans un paquet DEB, tous les fichiers présents dans l'arborescence de données sont automatiquement inclus. En RPM, **chaque fichier doit être explicitement listé** dans la section `%files`. Un fichier présent dans le buildroot mais absent de `%files` provoque une erreur de build (le check `Unpackaged files`).

```spec
%files
%license LICENSE
%doc README.md

%{_bindir}/syswatch
%{_unitdir}/syswatch.service

%dir /etc/syswatch
%config(noreplace) /etc/syswatch/syswatch.yaml

%attr(0750, syswatch, syswatch) %dir /var/lib/syswatch
%attr(0750, syswatch, syswatch) %dir /var/log/syswatch
```

Plusieurs directives méritent explication.

**`%{_bindir}`**, **`%{_unitdir}`** sont des macros RPM qui pointent vers les chemins standards de la distribution (`/usr/bin`, `/usr/lib/systemd/system`). Utilisez-les systématiquement plutôt que des chemins en dur — elles garantissent la compatibilité avec les différentes versions de Fedora et RHEL.

**`%license`** et **`%doc`** marquent les fichiers de licence et de documentation. RPM les installe automatiquement dans `/usr/share/licenses/` et `/usr/share/doc/` respectivement.

**`%dir`** déclare un répertoire appartenant au paquet. Sans cette directive, seuls les fichiers contenus dans le répertoire seraient enregistrés, pas le répertoire lui-même — ce qui causerait un répertoire orphelin après désinstallation.

**`%config(noreplace)`** est l'équivalent RPM du `conffiles` DEB. Lors d'une mise à jour, si l'utilisateur a modifié le fichier, RPM installe la nouvelle version sous le nom `syswatch.yaml.rpmnew` et conserve la version de l'utilisateur. Sans `noreplace`, RPM remplacerait le fichier et sauvegarderait l'ancienne version en `.rpmsave`. La sémantique `noreplace` est presque toujours ce que vous voulez pour les fichiers de configuration.

**`%attr(0750, syswatch, syswatch)`** fixe les permissions et le propriétaire d'un fichier ou répertoire directement dans le paquet. C'est plus propre que de faire un `chown` dans le `%post` — les permissions sont appliquées au moment de l'installation par RPM lui-même.

---

## Construction du paquet

### Préparation de l'archive source

`rpmbuild` attend une archive tar dans `SOURCES/` :

```bash
# Depuis la racine du projet
tar czf ~/rpmbuild/SOURCES/syswatch-1.2.0.tar.gz \
    --transform "s,^,syswatch-1.2.0/," \
    src/ include/ config/ systemd/ CMakeLists.txt LICENSE README.md
```

L'option `--transform` ajoute un préfixe `syswatch-1.2.0/` à tous les chemins dans l'archive — c'est la convention attendue par `%autosetup`.

### Copie du fichier spec

```bash
cp syswatch.spec ~/rpmbuild/SPECS/
```

### Lancement du build

```bash
rpmbuild -ba ~/rpmbuild/SPECS/syswatch.spec
```

L'option `-ba` signifie "build all" : compiler les sources, produire le RPM binaire *et* le SRPM. Alternatives utiles : `-bb` (binaire uniquement, pas de SRPM) et `-bs` (SRPM uniquement, pas de compilation).

Si le build réussit, le RPM est dans `~/rpmbuild/RPMS/x86_64/` :

```bash
$ ls ~/rpmbuild/RPMS/x86_64/
syswatch-1.2.0-1.el9.x86_64.rpm
```

---

## Vérification et test

### rpmlint

L'équivalent de `lintian` pour RPM est **`rpmlint`** :

```bash
$ rpmlint ~/rpmbuild/RPMS/x86_64/syswatch-1.2.0-1.el9.x86_64.rpm
syswatch.x86_64: W: no-manual-page-for-binary syswatch  
syswatch.x86_64: W: spelling-error %description -l en_US systeme -> system  
0 errors, 2 warnings.
```

Comme `lintian`, `rpmlint` signale les écarts par rapport aux bonnes pratiques. Zéro erreur est l'objectif.

### Inspection du contenu

```bash
# Métadonnées
$ rpm -qpi ~/rpmbuild/RPMS/x86_64/syswatch-1.2.0-1.el9.x86_64.rpm
Name        : syswatch  
Version     : 1.2.0  
Release     : 1.el9  
Architecture: x86_64  
...

# Liste des fichiers
$ rpm -qpl ~/rpmbuild/RPMS/x86_64/syswatch-1.2.0-1.el9.x86_64.rpm
/etc/syswatch/syswatch.yaml
/usr/bin/syswatch
/usr/lib/systemd/system/syswatch.service
/usr/share/doc/syswatch/README.md
/usr/share/licenses/syswatch/LICENSE
/var/lib/syswatch
/var/log/syswatch

# Dépendances automatiques
$ rpm -qpR ~/rpmbuild/RPMS/x86_64/syswatch-1.2.0-1.el9.x86_64.rpm
libc.so.6()(64bit)  
libc.so.6(GLIBC_2.34)(64bit)  
libcrypto.so.3()(64bit)  
libssl.so.3()(64bit)  
libstdc++.so.6()(64bit)  
openssl-libs >= 3.0  
...
```

La commande `rpm -qpR` est particulièrement révélatrice : elle montre à la fois les dépendances automatiques (les `.so`) et les dépendances explicites déclarées dans le champ `Requires`. C'est le bon moyen de vérifier que l'AutoReq a bien fait son travail.

### Installation et test

```bash
# Installation
$ sudo dnf install ./syswatch-1.2.0-1.el9.x86_64.rpm

# Vérification
$ systemctl status syswatch.service
$ rpm -V syswatch      # Vérifie l'intégrité des fichiers installés

# Désinstallation
$ sudo dnf remove syswatch
```

La commande `rpm -V` (verify) est une fonctionnalité propre à RPM sans équivalent direct dans le monde DEB. Elle compare chaque fichier installé avec les métadonnées enregistrées (checksum, taille, permissions, propriétaire) et signale toute divergence. C'est un outil de diagnostic précieux en production.

---

## Construire DEB et RPM depuis la même CI

En pratique, un pipeline CI professionnel produit les deux formats en parallèle. L'architecture type repose sur des matrix builds (voir section 38.7) avec un conteneur par distribution cible :

```yaml
# .github/workflows/packaging.yml (extrait)
jobs:
  package:
    strategy:
      matrix:
        include:
          - distro: ubuntu:24.04
            format: deb
            build_script: ./packaging/build-deb.sh
          - distro: rockylinux:9
            format: rpm
            build_script: ./packaging/build-rpm.sh
    
    runs-on: ubuntu-latest
    container: ${{ matrix.distro }}
    
    steps:
      - uses: actions/checkout@v4
      - run: ${{ matrix.build_script }}
      - uses: actions/upload-artifact@v4
        with:
          name: package-${{ matrix.format }}
          path: "dist/*.${{ matrix.format }}"
```

L'outil **nfpm** mentionné en section 39.1.3 simplifie cette approche en générant les deux formats depuis un même fichier de configuration YAML, sans maintenir deux scripts de build séparés. Pour les projets qui ciblent systématiquement DEB et RPM, c'est souvent le meilleur compromis entre contrôle et maintenabilité.

---

## Différences de comportement à connaître

Quelques divergences subtiles entre DEB et RPM méritent d'être gardées en tête lorsque vous maintenez les deux formats.

### Gestion des fichiers de configuration lors des mises à jour

Avec DEB et `conffiles`, `dpkg` demande interactivement à l'utilisateur quoi faire en cas de conflit (garder l'ancien, installer le nouveau, voir le diff). Avec RPM et `%config(noreplace)`, il n'y a pas de question interactive : RPM installe silencieusement le nouveau fichier sous l'extension `.rpmnew` et conserve la version de l'utilisateur. L'administrateur doit ensuite vérifier manuellement s'il y a un `.rpmnew` à intégrer. Ce comportement est plus adapté aux déploiements automatisés (pas de prompt interactif qui bloque la CI), mais demande une procédure post-mise à jour documentée.

### Dépendances automatiques

RPM calcule automatiquement les dépendances de librairies partagées pendant le build — il analyse les binaires dans le buildroot et injecte les `Requires` correspondants sans intervention. Côté DEB, cette étape nécessite un appel explicite à `dpkg-shlibdeps`. L'automatisme RPM est plus confortable, mais il peut aussi être source de surprises s'il détecte une dépendance indésirable. Les macros `%{?__requires_exclude}` permettent de filtrer les dépendances automatiques.

### Stripping

`rpmbuild` strippe automatiquement les binaires par défaut (via la macro `%__strip`). Il n'est pas nécessaire d'appeler `strip` manuellement comme pour un paquet DEB. Si vous voulez conserver les symboles de débogage, RPM peut générer automatiquement un sous-paquet `-debuginfo` contenant les fichiers `.debug` séparés — activé par défaut sur les distributions récentes.

### Gestion des utilisateurs système

La pratique recommandée sur Fedora et RHEL récentes est d'utiliser **sysusers.d** plutôt que `useradd` dans le script `%pre`. Un fichier `/usr/lib/sysusers.d/syswatch.conf` déclare l'utilisateur de manière déclarative :

```
u syswatch - "Syswatch monitoring agent" /nonexistent /sbin/nologin
```

La macro `%sysusers_create_compat` dans le `%pre` crée ensuite l'utilisateur. Cette approche est plus propre et plus portable, mais elle nécessite `systemd-sysusers` — disponible sur RHEL 9+ et Fedora.

---

## Résumé

Le format RPM repose sur les mêmes principes que DEB — métadonnées, dépendances, scripts de cycle de vie, arborescence de fichiers — mais avec une approche centralisée dans un fichier `.spec` unique. Les macros RPM (`%cmake`, `%systemd_post`, `%config(noreplace)`, `%attr`) automatisent les tâches courantes et les conventions de la distribution. La détection automatique des dépendances de librairies partagées est un avantage notable par rapport au workflow DEB.

Pour un développeur C++ travaillant principalement sur Ubuntu, la compétence RPM se résume à savoir écrire un `.spec` correct, comprendre les macros essentielles, et intégrer le build RPM dans un pipeline CI conteneurisé. L'outil `nfpm` offre un raccourci pragmatique pour les équipes qui veulent maintenir un seul fichier de configuration pour les deux formats.

La section suivante (39.3) explore une approche radicalement différente : les formats universels qui éliminent la distinction DEB/RPM en embarquant toutes les dépendances avec l'application.

⏭️ [AppImage et distribution universelle](/39-packaging/03-appimage.md)
