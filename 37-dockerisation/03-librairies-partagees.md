🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 37.3 — Gestion des librairies partagées dans les conteneurs

> **Section** : 37.3  
> **Chapitre** : 37 — Dockerisation d'Applications C++  
> **Prérequis** : Section 37.2 (Multi-stage builds), Section 27.4 (Linkage statique vs dynamique), Section 2.5.3 (Dépendances dynamiques)

---

## Le problème central

Lorsque vous copiez un binaire C++ compilé dynamiquement d'un stage Docker à un autre, vous ne copiez que le fichier exécutable. Ses dépendances — les librairies partagées (`.so`) qu'il charge au démarrage — restent dans le stage de build. Si l'image runtime ne contient pas ces librairies, le binaire refuse de démarrer :

```
/usr/local/bin/myapp: error while loading shared libraries: 
    libprotobuf.so.32: cannot open shared object file: 
    No such file or directory
```

Ce message est le symptôme le plus fréquent d'un stage runtime incomplet. Il survient systématiquement quand une dépendance n'est disponible ni via les paquets APT installés dans le stage runtime, ni via une copie manuelle depuis le stage de build.

En section 37.2.2, nous avons résolu ce problème de manière simple : installer les paquets runtime APT correspondant aux paquets `-dev` du stage de build (`libssl3` pour `libssl-dev`, `libcurl4` pour `libcurl4-openssl-dev`). Cette approche fonctionne tant que toutes vos dépendances proviennent des dépôts APT.

Mais dans un projet C++ réaliste, certaines librairies sont compilées depuis les sources (via Conan, vcpkg, ou FetchContent), n'existent dans aucun dépôt APT de la distribution cible, ou sont des librairies internes de votre organisation. Pour celles-là, il faut identifier, copier et configurer manuellement les `.so` dans l'image runtime. C'est l'objet de cette section.

---

## Rappel : comment le dynamic linker résout les dépendances

Quand le noyau Linux charge un exécutable ELF dynamiquement lié, il délègue au **dynamic linker** (`ld-linux-x86-64.so.2` sur x86_64) la résolution des librairies partagées. Ce linker cherche chaque `.so` référencée dans le binaire selon un ordre précis :

1. **`RPATH`** — Chemin codé en dur dans le binaire au moment de la compilation (obsolète, remplacé par `RUNPATH`).
2. **`LD_LIBRARY_PATH`** — Variable d'environnement, consultée à l'exécution.
3. **`RUNPATH`** — Chemin codé dans le binaire, mais consulté après `LD_LIBRARY_PATH`.
4. **Cache `ldconfig`** — Le fichier `/etc/ld.so.cache`, construit par la commande `ldconfig` à partir des chemins listés dans `/etc/ld.so.conf` et `/etc/ld.so.conf.d/`.
5. **Chemins par défaut** — `/lib/x86_64-linux-gnu`, `/usr/lib/x86_64-linux-gnu`, `/lib`, `/usr/lib`.

Si la librairie n'est trouvée à aucune de ces étapes, le binaire échoue au démarrage avec l'erreur `cannot open shared object file`. Le binaire ne démarre même pas — aucune ligne de votre `main()` ne s'exécute.

Comprendre cet ordre de résolution est essentiel pour diagnostiquer et résoudre les problèmes de librairies manquantes dans un conteneur Docker.

---

## Étape 1 : Identifier toutes les dépendances dynamiques

### `ldd` : la commande de base

La commande `ldd` affiche l'arbre complet des dépendances dynamiques d'un binaire, y compris les dépendances transitives (les `.so` dont dépendent vos `.so`) :

```bash
# Dans le stage de build ou dans un conteneur intermédiaire
ldd /install/bin/myapp
```

Sortie typique pour un serveur HTTP C++ utilisant OpenSSL, curl, Protobuf et spdlog :

```
linux-vdso.so.1 (0x00007ffcc5bfe000)  
libprotobuf.so.32 => /usr/local/lib/libprotobuf.so.32 (0x00007f4a...)  
libspdlog.so.1.14 => /usr/local/lib/libspdlog.so.1.14 (0x00007f4a...)  
libfmt.so.10 => /usr/local/lib/libfmt.so.10 (0x00007f4a...)  
libssl.so.3 => /lib/x86_64-linux-gnu/libssl.so.3 (0x00007f4a...)  
libcrypto.so.3 => /lib/x86_64-linux-gnu/libcrypto.so.3 (0x00007f4a...)  
libcurl.so.4 => /lib/x86_64-linux-gnu/libcurl.so.4 (0x00007f4a...)  
libstdc++.so.6 => /lib/x86_64-linux-gnu/libstdc++.so.6 (0x00007f4a...)  
libm.so.6 => /lib/x86_64-linux-gnu/libm.so.6 (0x00007f4a...)  
libgcc_s.so.1 => /lib/x86_64-linux-gnu/libgcc_s.so.1 (0x00007f4a...)  
libc.so.6 => /lib/x86_64-linux-gnu/libc.so.6 (0x00007f4a...)  
libpthread.so.0 => /lib/x86_64-linux-gnu/libpthread.so.0 (0x00007f4a...)  
/lib64/ld-linux-x86-64.so.2 (0x00007f4a...)
```

### Classifier les dépendances

Cette sortie se décompose en trois catégories qu'il faut distinguer :

**Librairies système fournies par la glibc** — `libc.so.6`, `libm.so.6`, `libpthread.so.0`, `libgcc_s.so.1`, `libstdc++.so.6`, `ld-linux-x86-64.so.2`, et `linux-vdso.so.1`. Ces librairies sont présentes dans toute image Debian ou Ubuntu, y compris les variantes slim. Vous n'avez **rien à faire** pour celles-ci.

**Librairies système disponibles via APT** — `libssl.so.3`, `libcrypto.so.3`, `libcurl.so.4`. Ces librairies sont fournies par des paquets Debian standards (`libssl3`, `libcurl4`). L'installation via `apt-get install` dans le stage runtime est la méthode la plus simple et la plus fiable.

**Librairies tierces non disponibles via APT** — `libprotobuf.so.32`, `libspdlog.so.1.14`, `libfmt.so.10`. Ces librairies ont été compilées depuis les sources (par Conan, vcpkg, ou CMake FetchContent) dans le stage de build. Elles n'existent dans aucun dépôt APT ou bien la version disponible ne correspond pas. C'est pour celles-ci qu'une copie manuelle est nécessaire.

### `readelf` : une alternative plus sûre que `ldd`

Il faut savoir que `ldd` exécute le dynamic linker sur le binaire cible, ce qui signifie qu'il **exécute du code** du binaire. Sur un binaire de confiance (le vôtre), ce n'est pas un problème. Mais dans un contexte de sécurité strict ou sur un binaire provenant d'une source externe, `readelf` est une alternative plus sûre car il se contente de lire les headers ELF sans rien exécuter :

```bash
readelf -d /install/bin/myapp | grep NEEDED
```

```
 0x0000000000000001 (NEEDED)  Shared library: [libprotobuf.so.32]
 0x0000000000000001 (NEEDED)  Shared library: [libspdlog.so.1.14]
 0x0000000000000001 (NEEDED)  Shared library: [libfmt.so.10]
 0x0000000000000001 (NEEDED)  Shared library: [libssl.so.3]
 0x0000000000000001 (NEEDED)  Shared library: [libcrypto.so.3]
 0x0000000000000001 (NEEDED)  Shared library: [libcurl.so.4]
 0x0000000000000001 (NEEDED)  Shared library: [libstdc++.so.6]
 0x0000000000000001 (NEEDED)  Shared library: [libc.so.6]
```

La différence avec `ldd` est que `readelf` ne montre que les dépendances **directes** du binaire, sans résoudre les dépendances transitives. C'est souvent suffisant pour identifier ce qu'il faut copier, mais si une de vos `.so` dépend elle-même d'une autre `.so` tierce, `ldd` reste nécessaire pour détecter la chaîne complète.

---

## Étape 2 : Copier les librairies depuis le stage de build

### Copie ciblée de fichiers individuels

La méthode la plus explicite et la plus contrôlée consiste à copier chaque `.so` individuellement :

```dockerfile
# Stage RUNTIME
FROM debian:bookworm-slim AS runtime

# Librairies système via APT
RUN apt-get update && apt-get install -y --no-install-recommends \
    libssl3 libcurl4 ca-certificates \
    && rm -rf /var/lib/apt/lists/*

# Librairies tierces copiées depuis le stage build
COPY --from=build /usr/local/lib/libprotobuf.so.32 /usr/local/lib/  
COPY --from=build /usr/local/lib/libspdlog.so.1.14 /usr/local/lib/  
COPY --from=build /usr/local/lib/libfmt.so.10 /usr/local/lib/  

# Reconstruire le cache du linker
RUN ldconfig
```

L'avantage de cette approche est la traçabilité : vous savez exactement ce qui entre dans votre image de production. Chaque ajout ou suppression de dépendance est un changement explicite dans le Dockerfile, visible lors des revues de code.

L'inconvénient est la maintenance : à chaque montée de version d'une librairie (par exemple `libprotobuf.so.32` → `libprotobuf.so.34`), le Dockerfile doit être mis à jour. Sur un projet avec de nombreuses dépendances tierces, cette maintenance devient fastidieuse.

### Copie d'un répertoire entier

Si vos librairies tierces sont installées dans un répertoire dédié dans le stage de build (ce qui est le cas par défaut avec Conan ou avec un `CMAKE_INSTALL_PREFIX` personnalisé), vous pouvez copier le répertoire entier :

```dockerfile
# Si Conan ou cmake --install a placé les .so dans /install/lib
COPY --from=build /install/lib/ /usr/local/lib/

RUN ldconfig
```

C'est moins granulaire mais plus maintenable. Le risque est de copier des fichiers inutiles (archives `.a`, fichiers `.cmake`, `.pc` de pkg-config) qui auraient pu se glisser dans `/install/lib/`. Pour éviter cela, assurez-vous que votre configuration CMake n'installe que les artefacts runtime :

```cmake
# Dans CMakeLists.txt — n'installer que les .so, pas les .a ni les headers
install(TARGETS mylib
    LIBRARY DESTINATION lib    # .so → installé
    ARCHIVE DESTINATION lib    # .a  → installé (à éviter si non nécessaire)
    RUNTIME DESTINATION bin    # exécutables
)
```

### Automatisation avec un script `ldd`

Pour les projets complexes avec de nombreuses dépendances, un script dans le stage de build peut collecter automatiquement toutes les `.so` nécessaires :

```dockerfile
FROM ubuntu:24.04 AS build
# ... compilation ...

# Collecter les .so non-système dans un répertoire dédié
RUN mkdir -p /runtime-libs && \
    ldd /install/bin/myapp | \
    grep "=> /usr/local" | \
    awk '{print $3}' | \
    xargs -I{} cp -L {} /runtime-libs/

# ---

FROM debian:bookworm-slim AS runtime

RUN apt-get update && apt-get install -y --no-install-recommends \
    libssl3 libcurl4 ca-certificates \
    && rm -rf /var/lib/apt/lists/*

COPY --from=build /runtime-libs/ /usr/local/lib/  
COPY --from=build /install/bin/myapp /usr/local/bin/myapp  

RUN ldconfig
```

Le filtre `grep "=> /usr/local"` est une heuristique : les librairies installées par Conan, vcpkg ou CMake FetchContent se trouvent généralement dans `/usr/local/lib`, tandis que les librairies système sont dans `/lib/x86_64-linux-gnu` ou `/usr/lib/x86_64-linux-gnu`. Ce filtre exclut donc les librairies système (déjà présentes dans l'image de base ou installées via APT) et ne copie que les tierces.

L'option `cp -L` est importante : elle suit les liens symboliques. Les librairies partagées utilisent couramment une chaîne de symlinks (`libfoo.so` → `libfoo.so.3` → `libfoo.so.3.2.1`). Sans `-L`, vous copieriez le symlink sans le fichier réel qu'il pointe.

Attention toutefois : cette heuristique suppose que toutes les librairies tierces sont dans `/usr/local`. Si certaines sont installées ailleurs (par exemple dans `/opt` ou dans un prefix Conan spécifique), adaptez le filtre `grep`.

---

## Étape 3 : Configurer le dynamic linker dans le runtime

### `ldconfig` : la méthode standard

Après avoir copié des `.so` dans `/usr/local/lib/`, il faut que le dynamic linker sache les trouver. La commande `ldconfig` reconstruit le cache `/etc/ld.so.cache` en scannant les répertoires listés dans `/etc/ld.so.conf` :

```dockerfile
COPY --from=build /install/lib/ /usr/local/lib/  
RUN ldconfig  
```

Sur Debian et Ubuntu, `/usr/local/lib` est inclus par défaut dans `/etc/ld.so.conf.d/libc.conf`, donc `ldconfig` sans argument suffit.

Si vous copiez des librairies dans un répertoire non standard, ajoutez-le à la configuration :

```dockerfile
COPY --from=build /install/lib/ /opt/myapp/lib/  
RUN echo "/opt/myapp/lib" >> /etc/ld.so.conf.d/myapp.conf && ldconfig  
```

### `LD_LIBRARY_PATH` : rapide mais déconseillé

La variable d'environnement `LD_LIBRARY_PATH` permet d'ajouter des chemins de recherche au dynamic linker sans passer par `ldconfig` :

```dockerfile
ENV LD_LIBRARY_PATH=/usr/local/lib
```

C'est la solution la plus rapide à mettre en place, mais elle est généralement déconseillée pour plusieurs raisons. `LD_LIBRARY_PATH` est consultée avant le cache `ldconfig` et avant les chemins par défaut : elle peut masquer involontairement une librairie système par une version locale, créant des incompatibilités subtiles. Elle est aussi héritée par tous les processus enfants, ce qui peut avoir des effets de bord si votre application lance des sous-processus.

Pour un conteneur Docker simple avec un seul binaire et un contrôle total sur le contenu de l'image, `LD_LIBRARY_PATH` fonctionne en pratique. Mais `ldconfig` est la méthode propre et reste préférable.

### `RPATH` / `RUNPATH` : codé dans le binaire

Au moment de la compilation, vous pouvez encoder le chemin des librairies directement dans le binaire via le `RPATH` ou `RUNPATH` :

```cmake
# Dans CMakeLists.txt
set_target_properties(myapp PROPERTIES
    INSTALL_RPATH "/usr/local/lib"
    BUILD_WITH_INSTALL_RPATH TRUE
)
```

Ou via les flags du linker :

```dockerfile
RUN cmake -B build -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_RPATH="/usr/local/lib" \
    -DCMAKE_BUILD_WITH_INSTALL_RPATH=ON
```

Avec un `RUNPATH` codé, le binaire sait où chercher ses `.so` sans dépendre de `ldconfig` ni de `LD_LIBRARY_PATH`. C'est particulièrement utile pour les images distroless (section 37.5) où l'exécution de `ldconfig` n'est pas possible.

Vous pouvez vérifier le `RUNPATH` d'un binaire avec `readelf` :

```bash
readelf -d myapp | grep -i path
# 0x000000000000001d (RUNPATH)  Library runpath: [/usr/local/lib]
```

---

## Étape 4 : Vérification complète dans le stage runtime

La vérification ne doit pas se faire uniquement dans le stage de build — les chemins et les librairies disponibles diffèrent entre les deux stages. Ajoutez une vérification dans le stage runtime lui-même :

```dockerfile
FROM debian:bookworm-slim AS runtime

# ... installation dépendances APT ...
# ... copie du binaire et des .so ...

RUN ldconfig

# Vérification : toutes les .so sont résolues
RUN ldd /usr/local/bin/myapp && \
    ldd /usr/local/bin/myapp | grep -q "not found" && \
    { echo "ERREUR: librairies manquantes détectées" ; exit 1 ; } || true

# Vérification : le binaire démarre
RUN /usr/local/bin/myapp --version
```

Le test `grep -q "not found"` détecte si une librairie n'est pas résolue et fait échouer le build Docker immédiatement, avec un message explicite. Sans cette vérification, l'erreur ne se manifesterait qu'au démarrage du conteneur en production — un moment beaucoup moins opportun pour diagnostiquer un problème de dépendance.

---

## Le cas du linkage statique complet

Une alternative radicale à toute cette gestion de `.so` est de compiler le binaire en **linkage statique complet** : toutes les dépendances sont embarquées directement dans l'exécutable.

```dockerfile
FROM alpine:3.20 AS build

RUN apk add --no-cache g++ cmake ninja-build make \
    linux-headers openssl-dev openssl-libs-static \
    curl-dev curl-static protobuf-dev

WORKDIR /src  
COPY . .  

RUN cmake -B build -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_EXE_LINKER_FLAGS="-static" \
    -DOPENSSL_USE_STATIC_LIBS=ON \
    && cmake --build build --parallel $(nproc)

# Vérifier que le binaire est réellement statique
RUN file build/myapp | grep "statically linked"  
RUN ldd build/myapp 2>&1 | grep "not a dynamic executable"  

# ---

FROM scratch AS runtime  
COPY --from=build /src/build/myapp /myapp  

# Certificats TLS (nécessaires même en statique si HTTPS est utilisé)
COPY --from=build /etc/ssl/certs/ca-certificates.crt /etc/ssl/certs/

ENTRYPOINT ["/myapp"]
```

### Avantages du linkage statique

**Zéro dépendance runtime** — Le binaire est autonome. L'image `scratch` (vide, 0 octet) suffit. Pas de librairies manquantes, pas de `ldconfig`, pas de problèmes d'ABI.

**Portabilité absolue** — Le binaire fonctionne sur n'importe quel noyau Linux compatible, indépendamment de la distribution ou de la version de glibc/musl.

**Image minimale** — La taille de l'image correspond exactement à la taille du binaire (plus les certificats TLS si nécessaire). Pour un serveur HTTP typique, c'est de l'ordre de 15-25 MB.

### Limites du linkage statique

**Taille du binaire** — Toutes les librairies sont embarquées, ce qui gonfle l'exécutable. Un binaire dynamique de 8 MB peut devenir un binaire statique de 25 MB. Ce n'est généralement pas un problème, sauf si le binaire doit être transféré fréquemment.

**Pas de mise à jour indépendante des librairies** — Si une vulnérabilité est découverte dans OpenSSL, il faut recompiler et redéployer le binaire entier. Avec le linkage dynamique, il suffit de mettre à jour le paquet `libssl3` dans l'image. Dans un environnement CI/CD avec des builds automatisés, cet inconvénient est atténué.

**La glibc résiste au linkage statique** — La glibc utilise en interne `dlopen` pour charger des modules NSS (Name Service Switch), ce qui rend un linkage statique complet problématique. Le binaire semble statique, mais peut échouer à résoudre des noms DNS via `/etc/nsswitch.conf`. C'est pourquoi l'exemple ci-dessus utilise Alpine (musl) : musl est conçue pour un linkage statique propre.

**Licence LGPL** — La glibc est distribuée sous LGPL, qui impose des contraintes spécifiques en cas de linkage statique (obligation de permettre le re-linkage). musl est sous licence MIT, sans cette contrainte. Consultez votre service juridique si vous distribuez des binaires statiques linkés contre la glibc.

**Certaines librairies ne supportent pas le statique** — Quelques librairies tierces ne fournissent pas de version statique (`.a`), ou bien leur compilation en statique est mal supportée. C'est de plus en plus rare, mais reste un obstacle occasionnel.

---

## Stratégie hybride : statique pour les tierces, dynamique pour le système

Un compromis courant consiste à lier statiquement les librairies tierces (Protobuf, spdlog, fmt, etc.) tout en conservant un linkage dynamique pour les librairies système (glibc, OpenSSL, libcurl). Cela élimine le problème des `.so` tierces à copier manuellement, tout en conservant la mise à jour indépendante des librairies système via APT.

```cmake
# CMakeLists.txt — forcer le linkage statique des dépendances internes
find_package(Protobuf REQUIRED)  
find_package(spdlog REQUIRED)  

# Protobuf et spdlog en statique, OpenSSL et curl en dynamique
target_link_libraries(myapp PRIVATE
    protobuf::libprotobuf    # .a si disponible via Conan (par défaut)
    spdlog::spdlog           # .a si disponible
    OpenSSL::SSL             # .so (dynamique système)
    CURL::libcurl            # .so (dynamique système)
)
```

Avec Conan, le linkage statique des dépendances se configure dans le profil :

```ini
# ~/.conan2/profiles/docker
[settings]
os=Linux  
compiler=gcc  
compiler.version=14  
build_type=Release  

[options]
*:shared=False
```

L'option `*:shared=False` indique à Conan de compiler toutes les dépendances en librairies statiques (`.a`). Le binaire résultant ne dépend dynamiquement que des librairies système — celles que vous installez facilement via APT dans le stage runtime.

La sortie de `ldd` sur un tel binaire est beaucoup plus courte :

```
linux-vdso.so.1 (0x00007ffc...)  
libssl.so.3 => /lib/x86_64-linux-gnu/libssl.so.3  
libcrypto.so.3 => /lib/x86_64-linux-gnu/libcrypto.so.3  
libcurl.so.4 => /lib/x86_64-linux-gnu/libcurl.so.4  
libstdc++.so.6 => /lib/x86_64-linux-gnu/libstdc++.so.6  
libm.so.6 => /lib/x86_64-linux-gnu/libm.so.6  
libgcc_s.so.1 => /lib/x86_64-linux-gnu/libgcc_s.so.1  
libc.so.6 => /lib/x86_64-linux-gnu/libc.so.6  
/lib64/ld-linux-x86-64.so.2
```

Plus aucune librairie tierce dans `/usr/local/lib` — elles sont toutes embarquées dans le binaire. Le stage runtime n'a besoin que des paquets APT système. C'est la stratégie qui offre le meilleur rapport simplicité/contrôle pour la majorité des projets C++ dockerisés.

---

## Arbre de décision

Voici un guide synthétique pour choisir votre stratégie de gestion des librairies dans un conteneur Docker :

```
Votre binaire a-t-il des dépendances .so tierces
(compilées par Conan/vcpkg/FetchContent) ?
│
├─ NON → Toutes les .so viennent d'APT
│        → Installer les paquets runtime dans le stage runtime
│        → C'est le cas le plus simple (cf. §37.2.2)
│
└─ OUI → Combien de dépendances tierces ?
         │
         ├─ Peu (1-3) → Copie manuelle ciblée des .so
         │              → Maintenable, traçable
         │
         ├─ Nombreuses → Deux choix :
         │   │
         │   ├─ Option A : Stratégie hybride
         │   │  Conan shared=False pour les tierces
         │   │  Linkage dynamique système (glibc, SSL, curl)
         │   │  → Élimine les .so tierces du runtime
         │   │
         │   └─ Option B : Copie automatisée
         │      Script ldd + grep dans le stage build
         │      COPY --from=build /runtime-libs/ → runtime
         │      → Plus fragile, mais pas de recompilation
         │
         └─ Toutes → Linkage statique complet
                     Alpine + musl + -static
                     Image scratch
                     → Zéro dépendance, image minimale
```

---

## Diagnostic des erreurs courantes

### `cannot open shared object file`

```
myapp: error while loading shared libraries: libfoo.so.5: 
    cannot open shared object file: No such file or directory
```

La librairie `libfoo.so.5` est absente de l'image runtime. Solutions par ordre de préférence :

1. Installer le paquet APT correspondant (`apt-file search libfoo.so.5`).
2. Copier la `.so` depuis le stage de build (`COPY --from=build`).
3. Passer la dépendance en linkage statique (Conan `shared=False`).

### `version GLIBCXX_3.4.32 not found`

```
myapp: /lib/x86_64-linux-gnu/libstdc++.so.6: version 
    `GLIBCXX_3.4.32' not found (required by myapp)
```

Le binaire a été compilé avec une version de GCC plus récente que celle fournie par l'image runtime. La `libstdc++.so.6` de l'image runtime ne contient pas les symboles nécessaires. Solutions :

1. Utiliser la même distribution (et donc la même version de `libstdc++`) dans les deux stages.
2. Copier la `libstdc++.so.6` du stage de build vers le stage runtime.
3. Compiler avec `-static-libstdc++` pour embarquer la librairie standard C++ dans le binaire.

### `GLIBC_2.38 not found`

```
myapp: /lib/x86_64-linux-gnu/libc.so.6: version 
    `GLIBC_2.38' not found (required by myapp)
```

Même problème mais avec la glibc elle-même : le binaire a été compilé sur un système avec une glibc plus récente que celle de l'image runtime. C'est un problème plus sérieux car la glibc n'est pas facilement copiable entre images — elle est intimement liée au dynamic linker et au noyau.

La règle absolue est : **compilez toujours sur une glibc de version inférieure ou égale à celle du runtime**. En pratique, utiliser la même distribution Debian/Ubuntu dans les deux stages élimine ce problème. Si vous devez compiler sur un système récent et déployer sur un système plus ancien, envisagez le linkage statique ou l'utilisation d'une image de build basée sur la même distribution que le runtime.

### Librairie trouvée dans le mauvais chemin

```
ldd myapp
    libfoo.so.5 => /usr/local/lib/libfoo.so.5
```

mais à l'exécution :

```
cannot open shared object file: libfoo.so.5
```

Cela signifie que le cache `ldconfig` n'a pas été reconstruit après la copie des `.so`. Ajoutez `RUN ldconfig` après les instructions `COPY` dans le stage runtime.

---


⏭️ [Best practices : Sécurité et reproductibilité](/37-dockerisation/04-best-practices.md)
