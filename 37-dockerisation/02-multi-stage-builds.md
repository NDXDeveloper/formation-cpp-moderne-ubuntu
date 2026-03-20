🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 37.2 — Multi-stage builds : Optimisation de la taille

> **Section** : 37.2  
> **Chapitre** : 37 — Dockerisation d'Applications C++  
> **Prérequis** : Section 37.1 (Images Docker Ubuntu vs Alpine), Chapitre 26 (CMake)

---

## Le problème : compiler du C++ produit des images énormes

Pour comprendre pourquoi les multi-stage builds sont indispensables en C++, il suffit de mesurer ce que contient un conteneur après une compilation naïve en un seul stage :

```dockerfile
# ⚠️ ANTI-PATTERN — Ne pas reproduire en production
FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    g++ cmake ninja-build pkg-config \
    libssl-dev libcurl4-openssl-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /src  
COPY . .  

RUN cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_CXX_COMPILER=g++ \
    && cmake --build build --parallel $(nproc)

ENTRYPOINT ["./build/myapp"]
```

Ce Dockerfile fonctionne. Le binaire est produit et s'exécute. Mais regardons ce qui se retrouve dans l'image finale :

```
Composant                     Taille approximative
─────────────────────────────────────────────────
Image de base ubuntu:24.04         ~78 MB  
GCC 14 (g++, libstdc++-dev, ...)  ~350 MB  
CMake + Ninja                      ~80 MB  
Headers de développement (-dev)   ~120 MB  
Fichiers source du projet          ~5 MB  
Fichiers objets (.o) du build     ~30 MB  
Cache CMake (build/)              ~50 MB  
Binaire final                      ~8 MB  
─────────────────────────────────────────────────
TOTAL IMAGE                       ~720 MB
```

Sur ces 720 MB, **seuls ~8 MB sont nécessaires à l'exécution**. Le compilateur, les headers, CMake, Ninja, les fichiers objets, les sources — tout cela est du poids mort qui n'a aucune raison d'exister en production. Pire encore, chaque composant superflu est un vecteur potentiel de vulnérabilité : un attaquant qui compromet le conteneur dispose d'un compilateur complet et d'outils de build pour élaborer des exploits sur place.

---

## La solution : séparer build et runtime

Le multi-stage build, introduit dans Docker 17.05, permet de définir **plusieurs images intermédiaires** dans un même `Dockerfile`, chacune partant de sa propre base. L'instruction `COPY --from=<stage>` permet ensuite d'extraire des fichiers d'un stage précédent vers le stage courant. Seul le **dernier stage** constitue l'image finale.

Le principe appliqué au C++ est simple :

1. **Stage de build** — Image lourde contenant la toolchain complète. On compile, on teste, on produit le binaire.
2. **Stage de runtime** — Image minimale ne contenant que le binaire, ses dépendances dynamiques, et le strict nécessaire à l'exécution.

Le stage de build est jeté après extraction du binaire. Il ne fait pas partie de l'image déployée.

```
  Stage BUILD (jetable)              Stage RUNTIME (déployé)
┌───────────────────────┐          ┌──────────────────────────┐
│ ubuntu:24.04          │          │ ubuntu:24.04     │
│                       │          │                          │
│ g++                   │          │                          │
│ cmake, ninja          │          │                          │
│ headers -dev          │          │                          │
│ sources (.cpp, .h)    │          │                          │
│ fichiers objets (.o)  │          │                          │
│ cache CMake           │          │                          │
│                       │  COPY    │                          │
│ ► binaire myapp ──────│─────────►│ binaire myapp            │
│                       │  --from  │ libssl3, libcurl4        │
│ ~1.2 GB               │          │ ca-certificates          │
│                       │          │                          │
│ ❌ Non inclus dans    │          │ ~95 MB                   │
│    l'image finale     │          │ ✅ Image de production   │
└───────────────────────┘          └──────────────────────────┘
```

Le résultat est spectaculaire : on passe de ~720 MB à ~95 MB, soit une réduction de **87%**, sans aucun compromis fonctionnel.

---

## Anatomie d'un Dockerfile multi-stage C++

Voici la structure générale que nous allons détailler dans les sous-sections 37.2.1 et 37.2.2 :

```dockerfile
# ============================================================
# Stage 1 : BUILD
# ============================================================
FROM ubuntu:24.04 AS build

# ... installation toolchain ...
# ... copie des sources ...
# ... compilation ...
# ... tests ...

# ============================================================
# Stage 2 : RUNTIME
# ============================================================
FROM ubuntu:24.04 AS runtime

# ... dépendances runtime uniquement ...
# ... copie du binaire depuis stage build ...
# ... configuration sécurité ...

ENTRYPOINT ["myapp"]
```

Le mot-clé `AS` donne un nom au stage, utilisé ensuite par `COPY --from=build`. C'est ce mécanisme qui crée la frontière étanche entre l'environnement de compilation et l'environnement d'exécution.

---

## Au-delà de deux stages : architectures avancées

Pour des projets de taille réelle, deux stages ne suffisent pas toujours. L'ajout de stages intermédiaires permet d'optimiser le **cache Docker** et de **paralléliser** certaines étapes du build.

### Stage dédié aux dépendances

Les dépendances tierces (Conan, vcpkg, ou APT) changent beaucoup moins souvent que le code source. En les isolant dans un stage séparé, on permet à Docker de réutiliser ce layer depuis le cache tant que le fichier de dépendances n'a pas changé :

```dockerfile
# Stage 1 : Dépendances (cache long)
FROM ubuntu:24.04 AS deps
# ... installation Conan, résolution des dépendances ...

# Stage 2 : Compilation (invalidé à chaque changement de source)
FROM deps AS build  
COPY src/ src/  
RUN cmake --build build  

# Stage 3 : Runtime
FROM ubuntu:24.04 AS runtime  
COPY --from=build /src/build/myapp /usr/local/bin/myapp  
```

Avec cette approche, une modification dans `src/main.cpp` ne déclenche que le stage 2 — les dépendances sont déjà en cache. Sur un projet avec 50 dépendances Conan, cela peut économiser **plusieurs minutes** par build.

### Stage dédié aux tests

Séparer les tests dans leur propre stage a deux avantages : les fichiers de test et le framework Google Test ne polluent pas l'image de production, et on peut construire l'image de release indépendamment du stage de test :

```dockerfile
FROM ubuntu:24.04 AS build
# ... compilation du binaire ...

FROM build AS test  
RUN ctest --test-dir build --output-on-failure  

FROM ubuntu:24.04 AS runtime  
COPY --from=build /src/build/myapp /usr/local/bin/myapp  
```

La subtilité ici est que le stage `runtime` copie depuis `build`, pas depuis `test`. Le stage `test` est exécuté (et doit réussir pour que le build Docker aboutisse), mais ses artefacts ne contaminent pas l'image finale.

### Architecture complète à quatre stages

Voici un schéma réaliste pour un projet C++ professionnel :

```
┌──────────────┐
│  Stage DEPS  │  Résolution Conan/vcpkg, installation librairies
│  (cache long)│  Invalidé seulement si conanfile.py change
└──────┬───────┘
       │ FROM deps AS build
┌──────▼───────┐
│  Stage BUILD │  Copie sources, cmake --build
│              │  Invalidé à chaque changement de code
└──────┬───────┘
       │                    │
       │ FROM build AS test │ FROM ... AS runtime
┌──────▼────────┐     ┌─────▼──────────┐
│  Stage TEST   │     │ Stage RUNTIME  │
│  ctest, GTest │     │ Binaire seul   │
│  (non déployé)│     │ (image finale) │
└───────────────┘     └────────────────┘
```

---

## Impact sur la CI/CD

Les multi-stage builds s'intègrent naturellement dans un pipeline CI/CD. Un seul `docker build` exécute la compilation, les tests, et produit l'image de production. Si les tests échouent, le build Docker échoue — pas besoin de scripts CI séparés pour orchestrer ces étapes.

Dans un fichier GitHub Actions ou GitLab CI, cela simplifie considérablement la configuration :

```yaml
# Un seul step remplace build + test + package
- name: Build and test
  run: docker build --target runtime -t myapp:${{ github.sha }} .
```

L'option `--target` permet de construire jusqu'à un stage spécifique. On peut par exemple construire uniquement le stage `test` dans un job CI de validation, et le stage `runtime` dans un job de déploiement.

Le chapitre 38 (CI/CD pour C++) reviendra en détail sur l'intégration des multi-stage builds avec ccache et sccache pour accélérer davantage les builds conteneurisés.

---

## Pièges courants

### Copier trop de fichiers dans le stage de build

Sans fichier `.dockerignore`, l'instruction `COPY . .` envoie l'intégralité du répertoire de travail au daemon Docker — y compris le dossier `build/` local, le répertoire `.git`, les fichiers temporaires de l'IDE, et potentiellement des artefacts de build obsolètes qui pèsent des centaines de mégaoctets.

Un `.dockerignore` minimal pour un projet C++ :

```
build/
.git/
.cache/
.vscode/
.idea/
*.o
*.a
CMakeCache.txt  
CMakeFiles/  
```

### Invalider le cache inutilement

L'ordre des instructions dans le Dockerfile a un impact direct sur l'efficacité du cache. Une règle d'or : **copier d'abord ce qui change le moins, et en dernier ce qui change le plus**. Pour un projet C++, cela signifie :

1. Le fichier de dépendances (`conanfile.py`, `vcpkg.json`) — change rarement.
2. Le `CMakeLists.txt` — change occasionnellement.
3. Les fichiers source (`src/`, `include/`) — changent à chaque commit.

Si vous commencez par `COPY . .`, tout changement dans n'importe quel fichier invalide le cache de toutes les instructions suivantes, y compris l'installation des dépendances.

### Oublier les dépendances runtime

Le piège le plus fréquent en multi-stage C++ : le binaire est copié dans le stage runtime, mais une librairie partagée est manquante. Le conteneur démarre et crashe immédiatement avec une erreur du type :

```
error while loading shared libraries: libssl.so.3:   
cannot open shared object file: No such file or directory  
```

La section 37.3 est entièrement consacrée à l'identification et la gestion de ces dépendances dynamiques. La commande `ldd` est votre alliée principale pour les détecter avant le déploiement.

---

## Ce que couvrent les sous-sections

Les deux sous-sections suivantes détaillent chaque stage individuellement :

| Sous-section | Focus | Contenu |
|---|---|---|
| **37.2.1** | Stage de compilation | Copie sélective des sources, configuration CMake dans Docker, optimisation du cache de layers, intégration Conan/vcpkg, compilation parallèle |
| **37.2.2** | Stage d'exécution minimal | Choix de l'image de base runtime, installation des seules dépendances dynamiques, configuration de l'ENTRYPOINT, vérification de l'image finale |

---


⏭️ [Stage de compilation](/37-dockerisation/02.1-stage-compilation.md)
