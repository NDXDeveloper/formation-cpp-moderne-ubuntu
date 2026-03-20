🔝 Retour au [Sommaire](/SOMMAIRE.md)

# Chapitre 39 — Packaging et Distribution

## Du binaire compilé au paquet installable : distribuer du C++ proprement

---

## Pourquoi ce chapitre ?

Vous avez écrit votre application C++. Elle compile, elle passe les tests, elle tourne dans un conteneur Docker. Mais comment la mettre entre les mains de vos utilisateurs — qu'il s'agisse d'une équipe interne, de serveurs de production, ou d'une communauté open source ?

La compilation produit un binaire (ou un ensemble de binaires et de librairies). Le **packaging** transforme ce résultat brut en un artefact installable, versionné, désinstallable proprement, et intégrable dans les systèmes de gestion de paquets que les administrateurs système connaissent déjà. Sans cette étape, distribuer du C++ revient à demander à chaque utilisateur de cloner le dépôt, installer les dépendances de build, et compiler lui-même — un processus fragile, lent, et hostile.

Ce chapitre couvre les formats de packaging les plus courants dans l'écosystème Linux, du plus spécifique à une distribution au plus universel.

---

## Le problème spécifique du C++

La distribution de logiciels compilés en C++ pose des défis que les langages interprétés ou à runtime universel n'ont pas, ou pas avec la même acuité.

### Dépendances binaires et compatibilité ABI

Un binaire C++ est lié — statiquement ou dynamiquement — à des librairies système. Quand la liaison est dynamique (`.so`), le binaire attend une version précise de chaque librairie à l'exécution. Si l'utilisateur a une version différente de `libstdc++`, de `libssl`, ou de n'importe quelle autre dépendance, le programme ne démarre pas — ou pire, il démarre et se comporte de manière imprévisible.

Le packaging résout ce problème en déclarant explicitement les dépendances et leurs contraintes de version. Le gestionnaire de paquets se charge de les résoudre et de les installer.

### Pas de runtime universel

Python a `pip` et un interpréteur commun. Java a le JRE. Go et Rust produisent des binaires statiques par défaut. Le C++ n'a rien de tout cela : chaque binaire porte les traces de son environnement de compilation (version du compilateur, libc, flags d'optimisation, standard C++ utilisé). Distribuer du C++ demande donc une attention particulière à la **reproductibilité** de l'environnement cible.

### Fichiers multiples à installer

Une application C++ non triviale ne se résume pas à un seul exécutable. Un paquet doit souvent inclure et positionner correctement des fichiers de configuration (dans `/etc/`), des librairies partagées (dans `/usr/lib/`), des headers pour les développeurs (dans `/usr/include/`), des pages man ou de la documentation (dans `/usr/share/`), des fichiers de service systemd (dans `/lib/systemd/system/`), et éventuellement des scripts de post-installation. Le packaging organise et automatise tout cela.

---

## Les formats de packaging Linux

L'écosystème Linux offre plusieurs formats, chacun avec son périmètre et ses compromis.

### Paquets natifs des distributions

Les deux grandes familles sont **DEB** (Debian, Ubuntu, Linux Mint, et dérivés) et **RPM** (Red Hat, Fedora, CentOS, Rocky Linux, openSUSE, et dérivés). Ces formats sont profondément intégrés à leurs distributions respectives : ils gèrent les dépendances via les dépôts officiels, supportent les scripts d'installation et de désinstallation, et s'appuient sur des outils matures (`dpkg`/`apt` pour DEB, `rpm`/`dnf` pour RPM).

Leur principal inconvénient est justement cette spécificité : un paquet `.deb` ne s'installe pas sur Fedora, et un `.rpm` ne fonctionne pas sur Ubuntu. Si votre cible est une distribution précise — ce qui est souvent le cas en environnement serveur — c'est le choix naturel. Si vous devez couvrir plusieurs distributions, il faut maintenir plusieurs pipelines de packaging.

### Formats universels

**AppImage**, **Snap** et **Flatpak** répondent au problème de la fragmentation en embarquant les dépendances avec l'application. Un AppImage est un fichier unique et autonome, exécutable sans installation. Snap (porté par Canonical) et Flatpak (porté par Red Hat et la communauté GNOME) ajoutent du sandboxing, des mises à jour automatiques, et une intégration avec des magasins d'applications.

Ces formats conviennent bien aux applications desktop et aux outils CLI distribués à une base d'utilisateurs hétérogène. Ils sont moins courants pour les déploiements serveur, où les paquets natifs ou les conteneurs Docker restent la norme.

### Conteneurs comme alternative au packaging

Docker, couvert au chapitre 37, constitue une forme de packaging à part entière : le conteneur embarque le binaire, ses dépendances, et l'environnement d'exécution complet. Pour les déploiements cloud et les architectures microservices, c'est souvent le format de distribution privilégié — le packaging DEB ou RPM intervient alors comme étape intermédiaire *à l'intérieur* du Dockerfile, ou bien il est remplacé par une simple copie du binaire dans une image distroless.

---

## Workflow typique : de la compilation au paquet

Un pipeline de packaging s'inscrit naturellement dans la chaîne CI/CD vue au chapitre 38. Le flux habituel suit ces étapes dans l'ordre :

1. **Compilation** — CMake + Ninja produisent les binaires et librairies, idéalement dans un environnement contrôlé (conteneur de build).
2. **Installation staging** — `cmake --install` place les fichiers dans un répertoire temporaire avec l'arborescence finale (`DESTDIR=/tmp/staging cmake --install build`).
3. **Création du paquet** — Un outil de packaging (`dpkg-deb`, `rpmbuild`, `appimagetool`) transforme cette arborescence en paquet distribuable.
4. **Tests du paquet** — Installation dans un environnement vierge, vérification que le programme se lance, que les dépendances sont correctement déclarées, que la désinstallation est propre.
5. **Publication** — Upload vers un dépôt APT/DNF privé, un registry d'artifacts (Artifactory, Nexus), ou un canal de distribution (Snapcraft, GitHub Releases).

L'intégration avec CMake est un point clé. La commande `install()` dans votre `CMakeLists.txt` définit **où** chaque fichier doit être installé. Cette information sert ensuite à tous les formats de packaging — c'est le contrat entre votre build system et votre pipeline de distribution.

---

## Ce que couvre ce chapitre

Les sections suivantes détaillent les formats les plus pertinents pour un développeur C++ travaillant sur Ubuntu et dans un contexte DevOps.

**Section 39.1 — Création de paquets DEB (Debian/Ubuntu)** traite du format natif d'Ubuntu : la structure interne d'un `.deb`, les scripts de contrôle, et la construction avec `dpkg-deb`. C'est le format incontournable si vous déployez sur des serveurs Debian/Ubuntu.

**Section 39.2 — Création de paquets RPM (RedHat/CentOS)** couvre l'équivalent pour l'écosystème Red Hat, avec `rpmbuild` et les fichiers `.spec`. Même si cette formation est centrée sur Ubuntu, connaître RPM est essentiel dans un contexte professionnel où les environnements cibles sont variés.

**Section 39.3 — AppImage et distribution universelle** présente le format AppImage pour produire un binaire autonome exécutable sur n'importe quelle distribution Linux récente, sans installation. Nous aborderons aussi brièvement Snap et Flatpak comme alternatives.

**Section 39.4 — Distribution via gestionnaires de paquets (apt, snap)** ferme la boucle en montrant comment publier vos paquets dans des dépôts accessibles à `apt` ou `snap`, pour que vos utilisateurs puissent installer et mettre à jour votre logiciel avec une simple commande.

---

## Prérequis

Ce chapitre s'appuie sur plusieurs notions vues précédemment dans la formation :

- **Chapitre 26 (CMake)** — La commande `install()` et la structure de projet CMake servent de base à tout le packaging.  
- **Chapitre 27 (Gestion des dépendances)** — La distinction entre linkage statique et dynamique (section 27.4) influence directement les dépendances déclarées dans un paquet.  
- **Chapitre 37 (Dockerisation)** — Les multi-stage builds et les images distroless sont complémentaires au packaging traditionnel.  
- **Chapitre 38 (CI/CD)** — L'automatisation du packaging s'intègre dans les pipelines GitLab CI ou GitHub Actions.

Une familiarité avec l'administration Linux de base (arborescence `/usr`, `apt`, permissions) est également supposée.

⏭️ [Création de paquets DEB (Debian/Ubuntu)](/39-packaging/01-paquets-deb.md)
