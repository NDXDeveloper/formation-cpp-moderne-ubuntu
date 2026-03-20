🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 39.1 — Création de paquets DEB (Debian/Ubuntu)

## Le format natif de l'écosystème Debian

---

## Introduction

Le format `.deb` est le format de packaging natif de Debian et de toutes les distributions qui en dérivent — Ubuntu, Linux Mint, Pop!_OS, Raspberry Pi OS, et bien d'autres. Quand vous tapez `sudo apt install htop`, c'est un fichier `.deb` qui est téléchargé depuis un dépôt, vérifié, décompressé, et installé sur votre système. Quand vous le désinstallez avec `apt remove`, c'est le système DEB qui sait quels fichiers supprimer, quels scripts exécuter, et quelles dépendances sont devenues orphelines.

Pour un développeur C++ qui déploie sur des serveurs Ubuntu — ce qui est le cas de figure principal de cette formation — savoir créer un paquet `.deb` est une compétence directement opérationnelle. Plutôt que de distribuer un binaire nu accompagné d'un `README` expliquant où le copier et quelles librairies installer, vous fournissez un artefact que l'administrateur système installe en une commande, avec résolution automatique des dépendances et désinstallation propre.

---

## Anatomie rapide du format

Un fichier `.deb` est en réalité une archive `ar` (le format d'archive Unix historique) qui contient trois éléments :

- **`debian-binary`** — Un petit fichier texte indiquant la version du format (actuellement `2.0`).  
- **`control.tar.zst`** — Une archive compressée contenant les métadonnées du paquet : nom, version, dépendances, description, ainsi que les scripts d'installation et de désinstallation. C'est le cerveau du paquet.  
- **`data.tar.zst`** — Une archive compressée contenant les fichiers à installer, organisés selon l'arborescence du système de fichiers cible (`usr/bin/`, `usr/lib/`, `etc/`, etc.). C'est le corps du paquet.

Vous pouvez vérifier cela sur n'importe quel `.deb` présent dans le cache APT de votre machine :

```bash
$ ar t /var/cache/apt/archives/htop_3.3.0-4_amd64.deb
debian-binary  
control.tar.zst  
data.tar.zst  
```

La compression a évolué au fil du temps : les anciens paquets utilisaient `gzip` (`.tar.gz`), puis `xz` (`.tar.xz`). Depuis Debian 12 et Ubuntu 23.04, `zstd` (`.tar.zst`) est le défaut, offrant un meilleur compromis entre ratio de compression et vitesse de décompression.

---

## Deux approches pour créer un paquet DEB

Il existe deux philosophies pour construire un `.deb`, chacune adaptée à un contexte différent.

### L'approche officielle Debian : debhelper et dpkg-buildpackage

C'est la méthode canonique utilisée par les mainteneurs de paquets Debian et Ubuntu. Elle repose sur un répertoire `debian/` à la racine du projet source, contenant un ensemble de fichiers standardisés (`rules`, `control`, `changelog`, `copyright`, etc.). L'outil `dpkg-buildpackage`, assisté par le framework `debhelper` (`dh`), orchestre la compilation, l'installation staging, et l'empaquetage.

Cette approche est la bonne si vous visez une inclusion dans les dépôts officiels Debian ou Ubuntu, ou si vous maintenez un paquet pour une distribution. Elle impose une structure rigoureuse, gère automatiquement de nombreux détails (stripping des binaires, calcul des dépendances de librairies partagées avec `dh_shlibdeps`, génération des symboles), et produit des paquets conformes à la Debian Policy.

En contrepartie, la courbe d'apprentissage est significative. Le système `debian/rules` est un Makefile avec des conventions spécifiques, la gestion du changelog suit un format strict, et les interactions entre `debhelper`, `dpkg-source`, et `lintian` (l'outil de vérification de conformité) demandent du temps à maîtriser.

### L'approche directe : dpkg-deb

C'est la méthode pragmatique pour les déploiements internes, les paquets privés, et les pipelines CI/CD où l'objectif est de produire rapidement un `.deb` installable sans viser les dépôts officiels. Vous construisez manuellement l'arborescence du paquet (métadonnées + fichiers), puis vous appelez `dpkg-deb --build` pour produire le `.deb`.

Cette approche offre un contrôle total et une compréhension directe de ce que contient le paquet. Elle est plus simple à automatiser dans un script de CI et ne nécessite pas d'apprendre le framework `debhelper`. C'est celle que nous détaillerons dans les sous-sections suivantes, car elle correspond au cas d'usage le plus fréquent pour un développeur C++ en contexte DevOps : packager une application interne pour la déployer sur un parc de serveurs Ubuntu.

---

## Prérequis : les outils nécessaires

Avant de commencer, assurez-vous que les outils de base sont installés sur votre machine Ubuntu :

```bash
sudo apt update  
sudo apt install dpkg-dev build-essential fakeroot lintian  
```

Voici le rôle de chaque paquet dans le processus :

- **`dpkg-dev`** fournit `dpkg-deb` (construction du `.deb`), `dpkg-shlibdeps` (détection des dépendances de librairies partagées), et `dpkg-gencontrol`.  
- **`build-essential`** installe GCC, `make`, et les headers standard — normalement déjà présent si vous suivez cette formation depuis le début.  
- **`fakeroot`** permet de construire un paquet avec les bonnes permissions (fichiers appartenant à `root:root`) sans avoir besoin de privilèges root réels. C'est indispensable pour un packaging propre en CI.  
- **`lintian`** est l'outil de vérification de conformité Debian. Il analyse votre `.deb` et signale les erreurs et avertissements (dépendances manquantes, permissions incorrectes, fichiers mal placés). Même pour un paquet interne, passer `lintian` est une bonne habitude.

---

## Le lien avec CMake : install() comme fondation

Le point de départ du packaging est la commande `install()` de votre `CMakeLists.txt`, vue au chapitre 26. Cette commande définit quels fichiers doivent être installés et où. Par exemple :

```cmake
# Binaire principal → /usr/bin/
install(TARGETS mon_outil RUNTIME DESTINATION bin)

# Librairie partagée → /usr/lib/
install(TARGETS ma_lib LIBRARY DESTINATION lib)

# Headers publics → /usr/include/mon_projet/
install(DIRECTORY include/mon_projet
        DESTINATION include)

# Fichier de configuration → /etc/mon_projet/
install(FILES config/mon_projet.conf
        DESTINATION /etc/mon_projet)
```

La variable `DESTDIR` permet de rediriger l'installation vers un répertoire temporaire plutôt que vers le système réel. C'est le mécanisme clé du packaging :

```bash
cmake --build build  
DESTDIR=/tmp/mon-paquet cmake --install build --prefix /usr  
```

Après cette commande, `/tmp/mon-paquet` contient l'arborescence complète des fichiers telle qu'elle apparaîtra sur le système cible : `/tmp/mon-paquet/usr/bin/mon_outil`, `/tmp/mon-paquet/usr/lib/libma_lib.so`, etc. Il ne reste plus qu'à ajouter les métadonnées (`control`) et à empaqueter le tout.

Cette séparation nette entre le build system (qui sait *quoi* installer *où*) et le packaging (qui sait *comment* l'empaqueter et le distribuer) est une bonne pratique fondamentale. Elle permet d'utiliser le même `CMakeLists.txt` pour produire un `.deb`, un `.rpm`, un AppImage, ou un conteneur Docker.

---

## CPack : l'alternative intégrée à CMake

CMake inclut un module appelé **CPack** qui peut générer directement des paquets DEB (et RPM, et d'autres formats) à partir des directives `install()`. Un exemple minimal :

```cmake
set(CPACK_GENERATOR "DEB")  
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Votre Nom <vous@exemple.com>")  
set(CPACK_DEBIAN_PACKAGE_DEPENDS "libssl3 (>= 3.0), libstdc++6")  
set(CPACK_PACKAGE_VERSION "1.2.0")  
include(CPack)  
```

Puis en ligne de commande :

```bash
cmake --build build  
cd build && cpack  
```

CPack est pratique pour les cas simples et pour les projets qui veulent rester 100% CMake. Cependant, il offre un contrôle limité sur les scripts d'installation, la gestion fine des dépendances, et les conventions Debian avancées. Pour cette raison, les projets professionnels lui préfèrent souvent l'approche `dpkg-deb` manuelle ou un outil spécialisé comme `nfpm` (un packager multi-format écrit en Go, populaire dans les pipelines CI modernes).

Nous présenterons les deux approches : `dpkg-deb` pour comprendre le mécanisme en profondeur, et une mention de CPack pour les cas où la simplicité prime.

---

## Plan de la section

Les sous-sections suivantes vous guident pas à pas dans la création d'un paquet DEB pour une application C++ :

**Section 39.1.1 — Structure d'un paquet DEB** détaille l'arborescence interne d'un `.deb` : le répertoire `DEBIAN/` avec ses fichiers de métadonnées, et l'arborescence de données qui reflète le système de fichiers cible. Vous apprendrez à construire cette structure à la main.

**Section 39.1.2 — Scripts de contrôle** couvre les scripts `preinst`, `postinst`, `prerm` et `postrm` qui s'exécutent aux différentes étapes de l'installation et de la désinstallation. Vous verrez comment les utiliser pour créer un utilisateur système, activer un service systemd, ou nettoyer des fichiers de cache.

**Section 39.1.3 — Construction avec dpkg-deb** met tout ensemble : à partir d'un projet C++ compilé avec CMake, vous produirez un `.deb` fonctionnel, le vérifierez avec `lintian`, l'installerez, le testerez, et le désinstallerez proprement.

---

## Conventions de nommage

Avant de plonger dans les détails, un mot sur le nommage. Les paquets Debian suivent une convention stricte :

```
<nom>_<version>-<révision>_<architecture>.deb
```

Par exemple : `mon-outil_1.2.0-1_amd64.deb`. Chaque composant a un rôle précis :

- **`nom`** est le nom du paquet, en minuscules, avec des tirets comme séparateurs (pas d'underscores ni de majuscules). Il doit être unique dans le dépôt cible.  
- **`version`** suit généralement le Semantic Versioning (`MAJOR.MINOR.PATCH`), cohérent avec votre tag Git et votre `CMakeLists.txt`.  
- **`révision`** (`-1`, `-2`, etc.) identifie la révision du packaging lui-même, indépendamment de la version du logiciel. Si vous corrigez un bug dans un script `postinst` sans changer le code source, vous incrémentez la révision.  
- **`architecture`** est l'architecture cible : `amd64` pour x86_64, `arm64` pour AArch64, `i386` pour x86 32 bits, ou `all` pour les paquets indépendants de l'architecture (scripts, documentation).

Respecter cette convention n'est pas qu'une question d'esthétique : `dpkg` et `apt` s'appuient sur le nom du fichier pour identifier et comparer les versions lors des mises à jour.

⏭️ [Structure d'un paquet DEB](/39-packaging/01.1-structure-deb.md)
