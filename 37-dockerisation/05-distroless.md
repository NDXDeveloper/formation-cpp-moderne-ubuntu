🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 37.5 — Distroless images : Conteneurs minimaux ⭐

> **Section** : 37.5  
> **Chapitre** : 37 — Dockerisation d'Applications C++  
> **Prérequis** : Sections 37.1 à 37.4, Section 27.4 (Linkage statique vs dynamique)

---

## Le concept : une image sans distribution

Tout au long de ce chapitre, nous avons progressivement réduit le contenu de l'image runtime : passage du single-stage au multi-stage, élimination de la toolchain, remplacement des paquets `-dev` par leurs équivalents runtime, suppression du cache APT. Le résultat — une image Debian slim d'environ 90 MB — est déjà un progrès considérable par rapport aux 720 MB initiaux.

Les images distroless poussent cette logique à son terme. Initiées par Google et publiées sous le projet `gcr.io/distroless`, elles ne contiennent **aucun des composants habituels d'une distribution Linux** : pas de shell (`/bin/sh`, `/bin/bash`), pas de gestionnaire de paquets (`apt`, `apk`), pas d'utilitaires système (`ls`, `cat`, `curl`, `wget`), pas d'éditeur de texte, pas de cron, pas de documentation. L'image contient uniquement les fichiers strictement nécessaires à l'exécution d'un binaire : la librairie C, le dynamic linker, les certificats TLS racine, et les données de fuseaux horaires.

Le résultat est une image de **25-35 MB** qui offre la compatibilité glibc d'une image Debian avec une surface d'attaque proche d'une image `scratch`. C'est le compromis idéal pour les binaires C++ compilés dynamiquement.

---

## Pourquoi les images distroless sont particulièrement adaptées au C++

Pour comprendre l'intérêt spécifique des images distroless pour le C++, comparons avec d'autres langages.

Un conteneur Python distroless doit embarquer l'interpréteur Python, ses modules standard, et toutes les dépendances pip. Le gain de taille par rapport à une image slim est modeste car l'interpréteur lui-même est volumineux.

Un conteneur Go n'a souvent besoin d'aucune librairie dynamique — le binaire statique va directement dans `scratch`. L'image distroless n'apporte rien de plus que `scratch`.

Le C++ occupe une position intermédiaire que les images distroless servent parfaitement. Un binaire C++ typique, compilé dynamiquement, dépend de la glibc, de `libstdc++`, et de quelques librairies système (OpenSSL, zlib). Ces librairies sont incluses dans l'image distroless `cc-debian12`. Le binaire ne nécessite ni shell ni utilitaires pour s'exécuter. L'image distroless fournit exactement ce dont le binaire a besoin — pas un fichier de plus.

---

## Les variantes distroless pour le C++

Google publie plusieurs variantes distroless, chacune ciblant un cas d'usage. Pour le C++, deux variantes sont pertinentes.

### `gcr.io/distroless/cc-debian12`

C'est la variante de référence pour le C++ compilé dynamiquement. Elle contient :

- La **glibc** (libc, libm, libpthread, libdl, librt)  
- La **libstdc++** (librairie standard C++)  
- La **libgcc_s** (support runtime GCC)  
- Le **dynamic linker** (`ld-linux-x86-64.so.2`)  
- Les **certificats TLS racine** (`/etc/ssl/certs/ca-certificates.crt`)  
- Les **données de fuseaux horaires** (`/usr/share/zoneinfo/`)  
- Un **utilisateur non-root** préconfiguré (`nonroot`, UID 65534)

Elle ne contient **pas** : OpenSSL, libcurl, zlib, ni aucune autre librairie au-delà de celles listées ci-dessus. Si votre binaire dépend de ces librairies, vous devrez les copier depuis le stage de build (section 37.3).

```dockerfile
FROM gcr.io/distroless/cc-debian12 AS runtime
```

Taille compressée : environ **30 MB**.

### `gcr.io/distroless/static-debian12`

Cette variante est encore plus minimale : elle ne contient ni la glibc, ni `libstdc++`, ni le dynamic linker. Elle est destinée exclusivement aux **binaires statiques** qui n'ont aucune dépendance dynamique.

```dockerfile
FROM gcr.io/distroless/static-debian12 AS runtime
```

Taille compressée : environ **5 MB**. Son contenu se limite aux certificats TLS, aux données de fuseaux horaires, au fichier `/etc/passwd` (utilisateur `nonroot`), et à quelques fichiers de configuration réseau minimaux.

Pour un binaire C++ entièrement statique (compilé avec `-static` sur Alpine/musl, voir section 37.3), `static-debian12` est fonctionnellement équivalent à `scratch` mais apporte en plus les certificats TLS et l'utilisateur non-root — deux éléments qu'il faudrait autrement copier manuellement.

### Quelle variante choisir

```
Votre binaire est-il 100% statique ?
│
├─ OUI → gcr.io/distroless/static-debian12  (~5 MB)
│        ou scratch + copie manuelle des certificats
│
└─ NON → Dépend-il uniquement de glibc + libstdc++ ?
         │
         ├─ OUI → gcr.io/distroless/cc-debian12  (~30 MB)
         │        Rien d'autre à faire
         │
         └─ NON → gcr.io/distroless/cc-debian12  (~30 MB)
                  + COPY --from=build des .so manquantes
                  (OpenSSL, libcurl, etc.)
```

---

## Dockerfile complet : C++ avec distroless

Voici un Dockerfile de production utilisant l'image distroless `cc-debian12` comme runtime pour un serveur HTTP C++ dépendant d'OpenSSL et de libcurl :

```dockerfile
# ============================================================
# Stage BUILD
# ============================================================
FROM ubuntu:24.04 AS build

ENV DEBIAN_FRONTEND=noninteractive  
ENV CXX=g++  
ENV CC=gcc  

RUN apt-get update && apt-get install -y --no-install-recommends \
    g++ cmake ninja-build pkg-config \
    libssl-dev libcurl4-openssl-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /src  
COPY CMakeLists.txt cmake/ ./  
COPY src/ src/  
COPY include/ include/  

RUN cmake -B build -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=/install \
    -DCMAKE_INSTALL_RPATH="/usr/local/lib" \
    -DCMAKE_BUILD_WITH_INSTALL_RPATH=ON \
    && cmake --build build --parallel $(nproc) \
    && cmake --install build

# Collecter les .so tierces (non fournies par distroless)
RUN mkdir -p /runtime-libs && \
    for lib in \
        /usr/lib/x86_64-linux-gnu/libssl.so.3 \
        /usr/lib/x86_64-linux-gnu/libcrypto.so.3 \
        /usr/lib/x86_64-linux-gnu/libcurl.so.4 \
        /usr/lib/x86_64-linux-gnu/libnghttp2.so.14 \
        /usr/lib/x86_64-linux-gnu/librtmp.so.1 \
        /usr/lib/x86_64-linux-gnu/libssh2.so.1 \
        /usr/lib/x86_64-linux-gnu/libpsl.so.5 \
        /usr/lib/x86_64-linux-gnu/libldap-2.5.so.0 \
        /usr/lib/x86_64-linux-gnu/liblber-2.5.so.0 \
        /usr/lib/x86_64-linux-gnu/libzstd.so.1 \
        /usr/lib/x86_64-linux-gnu/libbrotlidec.so.1 \
        /usr/lib/x86_64-linux-gnu/libbrotlicommon.so.1 \
        /usr/lib/x86_64-linux-gnu/libz.so.1 \
    ; do \
        [ -f "$lib" ] && cp -L "$lib" /runtime-libs/ ; \
    done

# Vérifier que le binaire fonctionne
RUN ldd /install/bin/myapp

# ============================================================
# Stage RUNTIME (distroless)
# ============================================================
FROM gcr.io/distroless/cc-debian12 AS runtime

# Copie des librairies tierces
COPY --from=build /runtime-libs/ /usr/local/lib/

# Copie du binaire
COPY --from=build /install/bin/myapp /usr/local/bin/myapp

# Copie de la configuration (si applicable)
COPY --from=build /install/etc/myapp/ /etc/myapp/

# Distroless fournit un utilisateur 'nonroot' (UID 65534)
USER nonroot

EXPOSE 8080

ENTRYPOINT ["myapp"]  
CMD ["--config", "/etc/myapp/config.yaml"]  
```

### Particularités de ce Dockerfile

**`CMAKE_INSTALL_RPATH`** — Le `RUNPATH` est codé directement dans le binaire pour pointer vers `/usr/local/lib`. C'est indispensable avec distroless car `ldconfig` n'est pas disponible pour reconstruire le cache du dynamic linker. Sans ce `RUNPATH`, le binaire ne trouverait pas les `.so` copiées dans `/usr/local/lib/`.

**Copie explicite des `.so` transitives** — La librairie `libcurl.so.4` dépend elle-même d'une dizaine de librairies (nghttp2, SSH2, Brotli, zstd, etc.). Avec Debian slim et APT, le paquet `libcurl4` installe automatiquement toutes ces dépendances transitives. Avec distroless, il n'y a pas de gestionnaire de paquets : chaque `.so` doit être copiée explicitement. C'est la contrepartie de la minimalité.

**`USER nonroot`** — Les images distroless fournissent un utilisateur `nonroot` préconfiguré avec l'UID 65534. Il n'est pas nécessaire de créer un utilisateur manuellement — et de toute façon, `useradd` n'est pas disponible dans l'image.

**Pas de `RUN ldconfig`** — L'image distroless ne contient pas la commande `ldconfig`. Le dynamic linker s'appuie sur le `RUNPATH` du binaire pour localiser les librairies. Si vous n'avez pas configuré le `RUNPATH` au moment de la compilation, l'alternative est d'utiliser la variable d'environnement `LD_LIBRARY_PATH` :

```dockerfile
ENV LD_LIBRARY_PATH=/usr/local/lib
```

---

## Simplifier : la stratégie hybride avec distroless

Le Dockerfile ci-dessus est fonctionnel mais verbeux — la liste de `.so` à copier est longue et fragile. Chaque mise à jour de libcurl peut ajouter ou supprimer une dépendance transitive, cassant silencieusement le build.

La stratégie hybride présentée en section 37.3 élimine ce problème : en liant statiquement les dépendances tierces (Conan `shared=False`) et dynamiquement la glibc et `libstdc++`, le binaire ne dépend que des librairies déjà présentes dans `cc-debian12`.

```dockerfile
# ============================================================
# Stage BUILD (stratégie hybride)
# ============================================================
FROM ubuntu:24.04 AS build

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
    g++ cmake ninja-build pkg-config \
    python3 python3-pip \
    && rm -rf /var/lib/apt/lists/*

RUN pip3 install --break-system-packages conan \
    && conan profile detect

WORKDIR /src

# Conan : linkage statique pour toutes les dépendances
COPY conanfile.py conan.lock ./  
RUN conan install . --lockfile=conan.lock --build=missing \  
    -s build_type=Release \
    -o "*:shared=False" \
    -of build/conan

COPY CMakeLists.txt cmake/ ./  
COPY src/ src/  
COPY include/ include/  

RUN cmake -B build -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_TOOLCHAIN_FILE=build/conan/conan_toolchain.cmake \
    -DCMAKE_INSTALL_PREFIX=/install \
    && cmake --build build --parallel $(nproc) \
    && cmake --install build

# Vérification : seules les .so système apparaissent
RUN ldd /install/bin/myapp

# ============================================================
# Stage RUNTIME (distroless, simplifié)
# ============================================================
FROM gcr.io/distroless/cc-debian12 AS runtime

COPY --from=build /install/bin/myapp /usr/local/bin/myapp

USER nonroot

EXPOSE 8080  
ENTRYPOINT ["myapp"]  
```

La sortie de `ldd` du binaire hybride :

```
linux-vdso.so.1 (0x00007ffc...)  
libstdc++.so.6 => /lib/x86_64-linux-gnu/libstdc++.so.6  
libm.so.6 => /lib/x86_64-linux-gnu/libm.so.6  
libgcc_s.so.1 => /lib/x86_64-linux-gnu/libgcc_s.so.1  
libc.so.6 => /lib/x86_64-linux-gnu/libc.so.6  
/lib64/ld-linux-x86-64.so.2
```

Chacune de ces librairies est fournie par `cc-debian12`. Le stage runtime se réduit à un seul `COPY` — plus de liste de `.so` à maintenir, plus de risque de dépendance transitive oubliée.

L'image résultante pèse environ **35 MB** : 30 MB pour la base distroless, plus le binaire. C'est un tiers de l'image Debian slim équivalente, avec une surface d'attaque incomparablement réduite.

---

## Debugging d'un conteneur distroless

L'absence de shell est le principal obstacle au diagnostic d'un problème dans un conteneur distroless en production. Il est impossible de faire un `docker exec -it container /bin/sh` — le shell n'existe pas.

### Les variantes `:debug`

Google publie des variantes debug de chaque image distroless, qui incluent un shell BusyBox minimal :

```dockerfile
# En développement ou staging uniquement
FROM gcr.io/distroless/cc-debian12:debug AS runtime-debug
```

Le shell BusyBox fournit des commandes basiques (`ls`, `cat`, `sh`, `env`, `ps`). Ces variantes sont destinées aux environnements de développement et de staging — **jamais à la production**.

En CI/CD, vous pouvez construire deux tags de la même image :

```bash
# Image de production (pas de shell)
docker build --target runtime -t myapp:1.2.0 .

# Image de debug (avec shell BusyBox)
docker build --target runtime-debug -t myapp:1.2.0-debug .
```

```dockerfile
# Dans le Dockerfile
FROM gcr.io/distroless/cc-debian12 AS runtime  
COPY --from=build /install/bin/myapp /usr/local/bin/myapp  
USER nonroot  
ENTRYPOINT ["myapp"]  

FROM gcr.io/distroless/cc-debian12:debug AS runtime-debug  
COPY --from=build /install/bin/myapp /usr/local/bin/myapp  
USER nonroot  
ENTRYPOINT ["myapp"]  
```

### Diagnostic sans shell : les conteneurs éphémères Kubernetes

Depuis Kubernetes 1.25 (stable), les **conteneurs éphémères** (`ephemeral containers`) permettent d'attacher un conteneur de debug à un pod en cours d'exécution, sans modifier ni redémarrer le pod :

```bash
# Attacher un conteneur de debug au pod
kubectl debug -it mypod \
    --image=ubuntu:24.04 \
    --target=myapp-container
```

Le conteneur éphémère partage les namespaces du pod cible (réseau, PID, etc.). Vous disposez d'un shell complet avec `apt-get install` pour installer les outils de diagnostic nécessaires (`strace`, `gdb`, `ldd`, `tcpdump`), sans que le conteneur de production ne soit modifié.

C'est l'approche recommandée pour le debugging en production avec des images distroless : le conteneur applicatif reste minimal et sécurisé, le conteneur de debug est temporaire et supprimé après le diagnostic.

### Logging comme stratégie de diagnostic principale

En l'absence de shell, les logs deviennent le principal vecteur de diagnostic. Votre application C++ doit émettre des logs structurés et détaillés sur `stdout`/`stderr`, captés nativement par Docker et Kubernetes :

```cpp
// spdlog configuré pour écrire sur stdout (cf. section 40.1)
auto logger = spdlog::stdout_color_mt("myapp");  
logger->set_level(spdlog::level::info);  
logger->info("Server started on port {}", port);  
```

Avec un logging structuré en JSON (section 40.5) et une pile d'observabilité (ELK, Loki, Datadog), la grande majorité des incidents peuvent être diagnostiqués sans accès shell au conteneur. Le chapitre 40 (Observabilité et Monitoring) couvre ce sujet en détail.

---

## `scratch` vs distroless : le dernier échelon

L'image `scratch` est l'image vide de Docker — elle ne contient littéralement aucun fichier. C'est le choix le plus radical pour la minimalisation.

### Quand `scratch` suffit

Pour un binaire C++ entièrement statique (musl + `-static`, voir section 37.3), `scratch` est fonctionnel. L'image ne contient que le binaire :

```dockerfile
FROM scratch  
COPY --from=build /src/build/myapp /myapp  
ENTRYPOINT ["/myapp"]  
```

Taille : uniquement le binaire (15-25 MB typiquement).

### Ce que distroless apporte par rapport à `scratch`

| Composant | `scratch` | `distroless/static` | `distroless/cc` |
|-----------|-----------|---------------------|------------------|
| Certificats TLS | ❌ | ✅ | ✅ |
| Données de timezone | ❌ | ✅ | ✅ |
| `/etc/passwd` (user `nonroot`) | ❌ | ✅ | ✅ |
| `/tmp` | ❌ | ✅ | ✅ |
| `/etc/nsswitch.conf` | ❌ | ✅ | ✅ |
| glibc + dynamic linker | ❌ | ❌ | ✅ |
| libstdc++ | ❌ | ❌ | ✅ |
| Taille | 0 MB | ~5 MB | ~30 MB |

Avec `scratch`, chacun de ces éléments doit être copié manuellement si nécessaire :

```dockerfile
# scratch avec certificats et timezone copiés manuellement
FROM scratch

# Certificats TLS
COPY --from=build /etc/ssl/certs/ca-certificates.crt /etc/ssl/certs/

# Timezone data
COPY --from=build /usr/share/zoneinfo /usr/share/zoneinfo

# Fichier passwd pour l'utilisateur non-root
COPY --from=build /etc/passwd /etc/passwd

# Le binaire
COPY --from=build /install/bin/myapp /myapp

USER 65534  
ENTRYPOINT ["/myapp"]  
```

Les images distroless encapsulent cette boilerplate dans une image testée et maintenue par Google. Pour un binaire statique, le choix entre `scratch` + copie manuelle et `distroless/static` est essentiellement une question de préférence : distroless est plus propre et plus maintenable, `scratch` est plus explicite et ne dépend d'aucune image tierce.

Pour un binaire dynamique, `distroless/cc` est clairement supérieur à `scratch` — il fournit la glibc et `libstdc++` dans une configuration testée, ce qui serait complexe et fragile à reproduire manuellement dans `scratch`.

---

## Compatibilité des versions : distroless et glibc

Les images distroless sont basées sur Debian et héritent de sa version de glibc. Le suffixe de l'image indique la version Debian source :

| Image distroless | Base Debian | glibc |
|-----------------|-------------|-------|
| `cc-debian11` | Debian 11 (Bullseye) | 2.31 |
| `cc-debian12` | Debian 12 (Bookworm) | 2.36 |

Le binaire compilé dans le stage de build doit être lié contre une glibc de version **inférieure ou égale** à celle de l'image distroless. En pratique :

- **Stage de build Ubuntu 24.04** (glibc 2.39) + **runtime `cc-debian12`** (glibc 2.36) → **risque d'incompatibilité**. Le binaire peut utiliser des symboles glibc 2.39 absents de la glibc 2.36.  
- **Stage de build Debian 12** (glibc 2.36) + **runtime `cc-debian12`** (glibc 2.36) → **compatibilité garantie**. Même version.  
- **Stage de build Debian 11** (glibc 2.31) + **runtime `cc-debian12`** (glibc 2.36) → **compatible**. La glibc est rétrocompatible — un binaire lié contre une ancienne version fonctionne sur une version plus récente.

La règle la plus sûre est d'utiliser **la même version de Debian** dans le stage de build et dans l'image distroless. Si vous compilez sur Ubuntu, vérifiez que la version de glibc d'Ubuntu ne dépasse pas celle de l'image distroless cible, ou compilez sur Debian.

---

## Impact sur le pipeline CI/CD

### Scan de vulnérabilités

Les images distroless contiennent significativement moins de composants que Debian slim. Le nombre de CVE potentielles est mécaniquement réduit :

```bash
# Scan d'une image Debian slim typique
trivy image myapp:slim
# Total: 42 (LOW: 28, MEDIUM: 11, HIGH: 2, CRITICAL: 1)

# Scan de la même application en distroless
trivy image myapp:distroless
# Total: 8 (LOW: 6, MEDIUM: 2, HIGH: 0, CRITICAL: 0)
```

Ces chiffres sont illustratifs, mais l'ordre de grandeur est représentatif : une image distroless remonte typiquement 5 à 10 fois moins de CVE qu'une image slim équivalente. Dans un environnement où les politiques de sécurité imposent le traitement de toutes les CVE de sévérité haute et critique, cette réduction simplifie considérablement le travail de maintenance.

### Temps de déploiement

Les images distroless se téléchargent plus vite (30 MB vs 90 MB), ce qui accélère le déploiement des pods Kubernetes. Sur un cluster avec des centaines de nœuds qui doivent tous télécharger l'image lors d'un rolling update, la différence de 60 MB par nœud est mesurable. Le gain est encore plus significatif dans les environnements multi-régions où les images transitent par des registries distribués.

---

## Comparaison finale des approches runtime

| Approche | Taille | Surface d'attaque | Complexité Dockerfile | Debug en prod | Compatibilité |
|----------|--------|-------------------|----------------------|---------------|---------------|
| Debian slim | ~90 MB | Modérée | Faible | Facile (shell) | Excellente |
| Distroless cc + hybride | ~35 MB | Faible | Faible | Difficile | Excellente |
| Distroless cc + copie `.so` | ~40 MB | Faible | Élevée | Difficile | Excellente |
| Alpine | ~18 MB | Modérée | Moyenne | Facile (shell) | Limitée (musl) |
| Distroless static | ~20 MB | Très faible | Moyenne | Très difficile | Statique requis |
| scratch | ~15 MB | Minimale | Élevée | Impossible | Statique requis |

La recommandation de cette formation pour un projet C++ de production est la combinaison **distroless `cc-debian12` + stratégie hybride** (Conan `shared=False`). Elle offre le meilleur rapport entre légèreté (~35 MB), sécurité (pas de shell, CVE minimales), simplicité du Dockerfile (un seul `COPY`), et compatibilité (glibc native).

Pour les équipes qui débutent avec Docker ou qui ont besoin d'un accès shell en production, Debian slim reste un excellent choix. Pour les projets capables de produire un binaire 100% statique, `distroless/static` ou `scratch` offrent la minimalité absolue.

---

## Récapitulatif du chapitre 37

Ce chapitre a couvert l'intégralité du workflow de conteneurisation d'une application C++. En partant d'un Dockerfile naïf de 720 MB, nous avons progressivement construit une image de production de 35 MB, sécurisée, reproductible, et optimisée :

| Section | Technique | Impact |
|---------|-----------|--------|
| 37.1 | Choix Ubuntu/Debian vs Alpine | Compatibilité glibc vs légèreté musl |
| 37.2 | Multi-stage builds | 720 MB → 90 MB (stage build éliminé) |
| 37.3 | Gestion des `.so` / stratégie hybride | Élimination des dépendances tierces du runtime |
| 37.4 | Sécurité et reproductibilité | Non-root, RELRO, pinning, scans CVE |
| 37.5 | Images distroless | 90 MB → 35 MB, surface d'attaque minimale |

Le chapitre suivant (38 — CI/CD pour C++) intègre ces techniques dans un pipeline automatisé, avec l'accélération par ccache/sccache et les matrix builds multi-compilateur.

---


⏭️ [CI/CD pour C++](/38-cicd/README.md)
