🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 37.1 — Création d'images Docker pour C++ (Ubuntu vs Alpine)

> **Section** : 37.1  
> **Chapitre** : 37 — Dockerisation d'Applications C++  
> **Prérequis** : Chapitre 2 (Toolchain Ubuntu), Section 27.4 (Linkage statique vs dynamique)

---

## Le choix fondamental : quelle image de base ?

Le premier `FROM` de votre Dockerfile est l'une des décisions les plus structurantes de votre stratégie de conteneurisation. Ce choix détermine la librairie C standard disponible, les outils de packaging, la taille de l'image résultante, et — plus subtilement — l'ensemble des bugs que vous risquez de rencontrer en production.

Pour le C++ compilé, deux familles d'images dominent largement l'écosystème :

- **Ubuntu / Debian** — basées sur la **glibc** (GNU C Library), l'implémentation standard de la librairie C sur Linux.  
- **Alpine Linux** — basée sur **musl**, une implémentation alternative de la librairie C, conçue pour être légère et simple.

Ce choix, anodin en apparence, a des répercussions profondes sur la compilation, le comportement à l'exécution, et la compatibilité de vos binaires.

---

## Ubuntu et Debian : la valeur sûre

### Pourquoi c'est le choix par défaut

Ubuntu et Debian utilisent la glibc, qui est la librairie C contre laquelle l'écrasante majorité des logiciels C++ sont développés et testés. Quand un mainteneur de librairie open-source teste son code sur Linux, il le fait généralement sur une distribution glibc. Quand les compilateurs GCC et Clang sont packagés, c'est d'abord pour des environnements glibc. Les librairies tierces installées via Conan ou vcpkg sont, par défaut, compilées contre la glibc.

Ce n'est pas un détail : cela signifie que choisir Ubuntu ou Debian, c'est choisir **le chemin de moindre friction**.

### Images de base recommandées

Pour le **stage de build** (compilation), les images courantes sont :

```dockerfile
# Ubuntu LTS — écosystème riche, APT bien maintenu
FROM ubuntu:24.04 AS build

# Debian stable — plus légère qu'Ubuntu, même glibc
FROM debian:bookworm AS build

# GCC officielle — toolchain préinstallée
FROM gcc:15 AS build
```

Pour le **stage de runtime** (exécution), on utilise des variantes allégées :

```dockerfile
# Debian slim — ~75 MB, pas de docs ni de cache apt
FROM debian:bookworm-slim AS runtime

# Ubuntu minimal — comparable
FROM ubuntu:24.04 AS runtime
```

### Installation de la toolchain sur Ubuntu

Voici l'installation complète d'un environnement de build C++ dans un conteneur Ubuntu :

```dockerfile
FROM ubuntu:24.04 AS build

# Éviter les prompts interactifs pendant l'installation
ENV DEBIAN_FRONTEND=noninteractive

# Installation de la toolchain en un seul layer
RUN apt-get update && apt-get install -y --no-install-recommends \
    g++ \
    cmake \
    ninja-build \
    pkg-config \
    libssl-dev \
    && rm -rf /var/lib/apt/lists/*
```

Quelques points importants sur ce `Dockerfile` :

**`--no-install-recommends`** — Cette option empêche APT d'installer les paquets "recommandés" qui ne sont pas strictement nécessaires. Sur un serveur de build, vous n'avez pas besoin de la documentation man, d'éditeurs de texte, ou d'utilitaires interactifs. Cette seule option peut économiser plusieurs centaines de mégaoctets.

**`rm -rf /var/lib/apt/lists/*`** — Le cache APT est inutile une fois les paquets installés. Le supprimer dans la même instruction `RUN` (et donc le même layer) évite qu'il soit capturé dans l'image.

**Un seul `RUN`** — Chaque instruction `RUN` crée un layer dans l'image Docker. Consolider l'installation en une seule commande chaînée avec `&&` produit un seul layer, ce qui réduit la taille finale et simplifie le cache.

### Installation de la toolchain sur Debian

L'équivalent Debian est quasiment identique, la différence étant les noms de paquets disponibles :

```dockerfile
FROM debian:bookworm AS build

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
    g++ \
    cmake \
    ninja-build \
    pkg-config \
    libssl-dev \
    && rm -rf /var/lib/apt/lists/*
```

### Avantages de l'approche Ubuntu/Debian

**Compatibilité maximale** — La quasi-totalité des librairies C++ sont testées contre la glibc. Les binaires compilés dans un conteneur Ubuntu s'exécutent sans surprise dans un conteneur Ubuntu ou Debian de même architecture.

**Écosystème APT riche** — Les librairies de développement sont disponibles directement via `apt-get install`. Pour des dépendances courantes comme OpenSSL, zlib, libcurl ou Boost, c'est souvent le moyen le plus rapide et fiable de les obtenir.

**Debugging facilité** — En cas de problème en production, vous pouvez `docker exec` dans le conteneur et disposer d'outils standards (`ldd`, `strace`, `gdb` si installé). Avec un runtime Debian slim, l'ajout temporaire d'outils de debug est trivial via APT.

**Support GCC et Clang natif** — Les deux compilateurs sont maintenus en tant que paquets officiels. La gestion des versions via `update-alternatives` (section 2.1.1) fonctionne de manière identique dans le conteneur.

### Inconvénients

**Taille des images** — Une image `ubuntu:24.04` pèse environ 78 MB compressée. Avec la toolchain installée, le stage de build atteint facilement 800 MB à 1.5 GB. Le stage de runtime en `debian:bookworm-slim` descend autour de 75-80 MB avant l'ajout du binaire et des `.so`.

**Surface d'attaque** — Même une image `slim` embarque un shell, des utilitaires système, et un gestionnaire de paquets. C'est autant de composants potentiellement vulnérables.

---

## Alpine Linux : la légèreté au prix de la vigilance

### Ce qu'Alpine change fondamentalement

Alpine Linux repose sur **musl** au lieu de la glibc, et sur **BusyBox** au lieu des coreutils GNU. L'image de base `alpine:3.20` pèse environ **7 MB** compressée — soit plus de dix fois moins qu'Ubuntu. C'est cette légèreté extrême qui fait son attrait.

Mais musl n'est pas un remplacement transparent de la glibc. C'est une implémentation indépendante de la norme POSIX qui fait des choix différents sur de nombreux points.

### Installation de la toolchain sur Alpine

```dockerfile
FROM alpine:3.20 AS build

RUN apk add --no-cache \
    g++ \
    cmake \
    ninja-build \
    make \
    pkgconf \
    linux-headers \
    openssl-dev
```

La syntaxe est plus concise : `apk add --no-cache` est l'équivalent d'un `apt-get install` suivi du nettoyage du cache, en une seule opération.

### Les différences musl vs glibc qui impactent le C++

C'est ici que le choix Alpine devient délicat pour le C++. Les différences entre musl et glibc ne sont pas seulement théoriques — elles provoquent des comportements observables.

#### Taille de stack par défaut des threads

La glibc alloue par défaut **8 MB** de stack par thread. musl alloue **128 KB** (ou 80 KB sur certaines versions). Pour une application C++ multi-threadée qui utilise de la récursion profonde ou des allocations importantes sur la stack, cette différence peut provoquer des **stack overflows silencieux** qui n'existent pas sur glibc.

```cpp
// Ce code peut crasher sur musl mais fonctionner sur glibc
void recursive_function(int depth) {
    char buffer[4096]; // 4 KB sur la stack à chaque appel
    if (depth > 0)
        recursive_function(depth - 1);
}

// depth=100 → ~400 KB de stack, dépasse le défaut musl
```

La solution est de configurer explicitement la taille de la stack lors de la création des threads, via `pthread_attr_setstacksize` ou les attributs de `std::jthread` :

```cpp
pthread_attr_t attr;  
pthread_attr_init(&attr);  
pthread_attr_setstacksize(&attr, 8 * 1024 * 1024); // 8 MB comme glibc  
```

#### Résolution DNS

L'implémentation DNS de musl ne supporte pas certaines fonctionnalités avancées de la glibc, notamment :

- Le fichier `/etc/resolv.conf` avec l'option `search` peut se comporter différemment.  
- La résolution DNS est **bloquante et sérialisée** dans musl (pas de requêtes parallèles A et AAAA).  
- Les noms de service dans `/etc/services` ont une couverture réduite.

Pour un serveur C++ qui fait beaucoup de résolutions DNS, cela peut entraîner des **latences visibles** par rapport à un déploiement glibc.

#### Gestion des locales

musl a un support minimaliste des locales. Si votre application C++ utilise `std::locale`, `std::setlocale`, ou les facets de localisation de la STL (formatage de nombres, dates, tri selon la locale), le comportement peut différer significativement. En pratique, musl se comporte comme si seule la locale `C`/`POSIX` était disponible.

```cpp
// Sur glibc : formatage selon la locale système
// Sur musl : formatage toujours en locale C
std::cout.imbue(std::locale("fr_FR.UTF-8")); // peut échouer sur musl
```

#### Compatibilité des librairies précompilées

Un binaire compilé contre la glibc ne peut **pas** s'exécuter sur un système musl (et inversement) sans recompilation. Cela signifie :

- Les librairies précompilées distribuées sous forme de `.so` pour "Linux x86_64" supposent généralement la glibc. Elles ne fonctionneront pas sur Alpine.  
- Les paquets Conan ou vcpkg doivent être compilés spécifiquement pour musl, ce qui n'est pas toujours supporté par les recettes existantes.  
- Certaines librairies tierces ne compilent tout simplement pas contre musl sans patches.

### Quand Alpine est un bon choix

Malgré ces contraintes, Alpine reste pertinent dans plusieurs scénarios :

**Binaire statique pur** — Si vous compilez votre application C++ avec un linkage entièrement statique (`-static`), le binaire résultant est autonome et ne dépend ni de musl ni de la glibc. Dans ce cas, Alpine offre un environnement de build léger et l'image runtime peut être `scratch` (vide).

```dockerfile
FROM alpine:3.20 AS build

RUN apk add --no-cache g++ cmake ninja-build make linux-headers \
    openssl-dev openssl-libs-static

WORKDIR /src  
COPY . .  

RUN cmake -B build -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_EXE_LINKER_FLAGS="-static" \
    && cmake --build build

# Image runtime vide : uniquement le binaire
FROM scratch AS runtime  
COPY --from=build /src/build/myapp /myapp  
ENTRYPOINT ["/myapp"]  
```

**Microservices simples** — Pour un petit service sans dépendances complexes, sans utilisation avancée des locales ni de threading intensif, Alpine fonctionne très bien et produit des images minuscules.

**Environnements contraints en taille** — Dans des contextes edge/IoT ou des clusters Kubernetes avec des budgets réseau serrés, les quelques dizaines de mégaoctets économisés par image peuvent se cumuler de façon significative lorsque des centaines de pods sont déployés.

### Quand éviter Alpine

**Applications multi-threadées complexes** — Le défaut de stack réduit et les différences de comportement de pthreads introduisent des risques de bugs subtils et difficiles à diagnostiquer.

**Dépendances tierces nombreuses** — Si votre projet dépend de librairies qui ne sont pas testées contre musl, vous allez passer un temps non négligeable à résoudre des problèmes de compilation ou de runtime.

**Applications nécessitant des locales** — Tout ce qui touche à l'internationalisation ou au formatage localisé sera probablement cassé ou dégradé.

**Équipe non familière avec musl** — Les bugs liés aux différences musl/glibc sont notoirement difficiles à diagnostiquer. Si votre équipe n'a pas d'expérience avec musl, le temps perdu en debugging peut largement annuler le gain en taille d'image.

---

## Comparaison chiffrée

Voici les tailles d'images mesurées pour un même projet C++ minimal (un serveur HTTP basé sur cpp-httplib, compilé avec GCC 14, CMake, Ninja) en mars 2026 :

| Configuration | Image de build | Image runtime | Binaire |
|---------------|---------------|---------------|---------|
| Ubuntu 24.04 + Debian slim runtime | ~1.2 GB | ~95 MB | ~8 MB (dynamique) |
| Debian bookworm + Debian slim runtime | ~1.0 GB | ~90 MB | ~8 MB (dynamique) |
| Alpine 3.20 + Alpine runtime | ~650 MB | ~18 MB | ~7 MB (dynamique/musl) |
| Alpine 3.20 + scratch (statique) | ~700 MB | ~12 MB | ~12 MB (statique) |
| Ubuntu 24.04 + distroless (§37.5) | ~1.2 GB | ~35 MB | ~8 MB (dynamique) |

Quelques observations :

- L'image de build est toujours volumineuse — c'est normal et attendu. Elle n'est jamais déployée en production ; seule l'image runtime compte.  
- La différence entre Alpine runtime (~18 MB) et Debian slim runtime (~90 MB) est significative, mais **les deux sont raisonnables** pour un déploiement Kubernetes.  
- Le binaire statique est plus gros que le dynamique (il embarque la librairie standard et les dépendances), mais il élimine toute dépendance au runtime.  
- L'approche distroless (section 37.5) offre un compromis intéressant : plus légère que Debian slim, compatible glibc, et sans shell.

---

## L'image GCC officielle : un raccourci pour le build

Docker Hub propose des images officielles GCC qui embarquent la toolchain complète :

```dockerfile
FROM gcc:15 AS build

# CMake et Ninja ne sont pas inclus par défaut
RUN apt-get update && apt-get install -y --no-install-recommends \
    cmake ninja-build \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /src  
COPY . .  

RUN cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release \
    && cmake --build build
```

Ces images sont basées sur Debian et incluent GCC préinstallé à la version spécifiée. L'avantage est la rapidité de mise en place : pas besoin de configurer `update-alternatives` ni de se soucier de la version du compilateur. L'inconvénient est que ces images sont volumineuses (souvent plus de 1.5 GB) et que le contrôle fin de l'environnement est plus limité.

Pour des builds CI/CD où la taille de l'image de build n'est pas critique (le cache Docker la télécharge une seule fois), c'est un choix pragmatique.

---

## Structure de base d'un Dockerfile C++ complet

Pour conclure cette section, voici un template complet qui servira de fondation pour les sections suivantes. Il illustre l'approche Ubuntu/Debian, qui sera notre référence par défaut :

```dockerfile
# ============================================================
# Stage 1 : BUILD
# ============================================================
FROM ubuntu:24.04 AS build

ENV DEBIAN_FRONTEND=noninteractive

# Toolchain C++ et dépendances de build
RUN apt-get update && apt-get install -y --no-install-recommends \
    g++ \
    cmake \
    ninja-build \
    pkg-config \
    libssl-dev \
    libcurl4-openssl-dev \
    && rm -rf /var/lib/apt/lists/*

# Copie des sources
WORKDIR /src  
COPY CMakeLists.txt .  
COPY src/ src/  
COPY include/ include/  

# Configuration et compilation
RUN cmake -B build -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_CXX_COMPILER=g++

RUN cmake --build build --parallel $(nproc)

# Vérification rapide : le binaire existe et s'exécute
RUN ./build/myapp --version

# ============================================================
# Stage 2 : RUNTIME
# ============================================================
FROM debian:bookworm-slim AS runtime

# Dépendances runtime uniquement (pas les -dev)
RUN apt-get update && apt-get install -y --no-install-recommends \
    libssl3 \
    libcurl4 \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

# Utilisateur non-root (sécurité — détaillé en §37.4)
RUN useradd --create-home --shell /bin/false appuser  
USER appuser  

# Copie du binaire depuis le stage de build
COPY --from=build /src/build/myapp /usr/local/bin/myapp

ENTRYPOINT ["myapp"]
```

Ce template sera enrichi progressivement dans les sections suivantes : la section 37.2 détaillera les stratégies de multi-stage build avancées, la section 37.3 traitera de la copie fine des librairies partagées, et les sections 37.4 et 37.5 aborderont respectivement les bonnes pratiques de sécurité et les images distroless.

---

## Recommandation de la formation

**Commencez avec Ubuntu ou Debian.** C'est le choix par défaut recommandé pour la grande majorité des projets C++ conteneurisés. La compatibilité glibc élimine une classe entière de bugs, l'écosystème APT couvre la plupart des dépendances, et la différence de taille avec Alpine au niveau de l'image runtime (90 MB vs 18 MB) est rarement un facteur décisif.

Réservez Alpine aux cas où vous visez un linkage statique complet, où la taille de l'image est une contrainte forte et mesurée, ou lorsque votre équipe a une expérience établie avec musl.

Si vous cherchez un compromis entre légèreté et compatibilité glibc, les images **distroless** (section 37.5) sont souvent une meilleure réponse qu'Alpine.

---


⏭️ [Multi-stage builds : Optimisation de la taille](/37-dockerisation/02-multi-stage-builds.md)
