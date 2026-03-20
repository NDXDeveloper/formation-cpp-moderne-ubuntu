🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 37.4 — Best practices : Sécurité et reproductibilité

> **Section** : 37.4  
> **Chapitre** : 37 — Dockerisation d'Applications C++  
> **Prérequis** : Sections 37.1 à 37.3, Chapitre 45 (Sécurité en C++)

---

## Deux objectifs complémentaires

Un conteneur Docker de production doit répondre à deux exigences qui se renforcent mutuellement.

**La sécurité** — Réduire la surface d'attaque, limiter les privilèges, empêcher l'escalade en cas de compromission du binaire. Un serveur C++ manipule souvent des données sensibles (authentification, paiements, données personnelles) et tourne exposé au réseau. Chaque composant superflu dans l'image est une opportunité pour un attaquant.

**La reproductibilité** — Garantir que le même `Dockerfile` produit la même image, quel que soit le moment et la machine où le build est exécuté. Sans reproductibilité, une image qui fonctionne aujourd'hui peut échouer demain parce qu'une dépendance a changé de version en amont. Dans un contexte réglementé (finance, santé, défense), la capacité à reconstruire un binaire identique à partir des mêmes sources est une exigence contractuelle.

Cette section présente les pratiques essentielles pour atteindre ces deux objectifs dans le contexte spécifique du C++ conteneurisé.

---

## Sécurité de l'image

### Exécution en utilisateur non-root

Nous avons introduit cette pratique en section 37.2.2. Approfondissons-la.

Par défaut, le processus principal d'un conteneur Docker tourne en tant que `root` (UID 0). Même si les namespaces Linux isolent ce root du root de l'hôte, un conteneur root dispose de privilèges étendus à l'intérieur de son espace de noms : il peut modifier les fichiers système du conteneur, installer des paquets, écouter sur des ports privilégiés, et potentiellement exploiter des vulnérabilités du noyau pour s'échapper de l'isolation.

La création d'un utilisateur applicatif dédié élimine ces risques :

```dockerfile
# Créer un groupe et un utilisateur avec UID/GID déterministes
RUN groupadd --gid 10001 appgroup \
    && useradd --uid 10001 --gid appgroup \
       --no-create-home --shell /bin/false appuser

# Toute instruction après USER s'exécute en tant que appuser
USER appuser
```

Le choix d'un UID élevé (10001 plutôt que 1000) évite les collisions avec les utilisateurs système prédéfinis dans certaines images de base. L'option `--no-create-home` est appropriée pour les services qui n'ont pas besoin d'un répertoire home — moins de fichiers signifie moins de surface d'attaque. Le shell `/bin/false` empêche toute tentative de connexion interactive.

### Contraintes Kubernetes

Dans un cluster Kubernetes, les politiques de sécurité (`PodSecurityStandards` ou `SecurityContext`) peuvent imposer des contraintes strictes :

```yaml
# Extrait d'un PodSpec Kubernetes
securityContext:
  runAsNonRoot: true
  runAsUser: 10001
  runAsGroup: 10001
  readOnlyRootFilesystem: true
  allowPrivilegeEscalation: false
  capabilities:
    drop: ["ALL"]
```

**`runAsNonRoot: true`** — Kubernetes refuse de démarrer le pod si l'image ne spécifie pas un `USER` non-root. C'est le filet de sécurité : même si le Dockerfile oublie l'instruction `USER`, le déploiement échoue plutôt que de tourner en root.

**`readOnlyRootFilesystem: true`** — Le système de fichiers du conteneur est monté en lecture seule. Le binaire ne peut écrire nulle part sauf dans des volumes explicitement montés (`emptyDir`, `PersistentVolumeClaim`). Cela empêche un attaquant de modifier des fichiers du conteneur, d'installer des outils, ou de déposer un backdoor.

Pour que votre conteneur C++ fonctionne avec `readOnlyRootFilesystem`, votre application doit :

- Écrire ses logs sur `stdout`/`stderr` (plutôt que dans `/var/log/`).  
- Utiliser un volume `emptyDir` monté sur `/tmp` si elle a besoin de fichiers temporaires.  
- Ne pas appeler `ldconfig` au démarrage (le cache doit être construit au build-time).

```dockerfile
# Préparer l'image pour readOnlyRootFilesystem
FROM debian:bookworm-slim AS runtime

RUN apt-get update && apt-get install -y --no-install-recommends \
    libssl3 libcurl4 ca-certificates \
    && rm -rf /var/lib/apt/lists/*

# ldconfig au build-time, pas au runtime
COPY --from=build /install/lib/ /usr/local/lib/  
RUN ldconfig  

RUN groupadd --gid 10001 appgroup \
    && useradd --uid 10001 --gid appgroup \
       --no-create-home --shell /bin/false appuser

COPY --from=build /install/bin/myapp /usr/local/bin/myapp

USER appuser

ENTRYPOINT ["myapp"]
```

**`capabilities: drop: ["ALL"]`** — Supprime toutes les capacités Linux du processus. Par défaut, Docker accorde un sous-ensemble de capacités même aux processus non-root. Les supprimer toutes est le principe de moindre privilège poussé à son terme. Si votre binaire a besoin d'écouter sur un port < 1024 (par exemple le port 443), vous pouvez réaccorder uniquement `NET_BIND_SERVICE` au lieu d'accorder toutes les capacités.

### Supprimer les shells et outils de diagnostic

Dans une image de production, la présence d'un shell (`/bin/sh`, `/bin/bash`) permet à un attaquant ayant compromis le binaire d'exécuter des commandes arbitraires dans le conteneur. Supprimer le shell réduit drastiquement les possibilités d'exploitation post-compromission.

Sur une image Debian slim, la suppression manuelle est possible mais délicate :

```dockerfile
# Approche radicale — fonctionnel mais fragile
RUN rm -f /bin/sh /bin/bash /usr/bin/apt* /usr/bin/dpkg
```

Cette approche est fragile et incomplète. Les images **distroless** (section 37.5) sont une solution architecturale à ce problème : elles ne contiennent tout simplement pas de shell. Si la suppression du shell est une exigence de sécurité, passez directement aux images distroless plutôt que d'élaguer une image Debian.

### Compiler le binaire avec les protections de sécurité

Le chapitre 45 (Sécurité en C++) couvre ce sujet en détail. Dans le contexte Docker, assurez-vous que les flags de sécurité sont activés lors de la compilation dans le stage de build :

```dockerfile
RUN cmake -B build -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_CXX_FLAGS="-fstack-protector-strong -D_FORTIFY_SOURCE=2" \
    -DCMAKE_EXE_LINKER_FLAGS="-Wl,-z,relro,-z,now -pie" \
    -DCMAKE_POSITION_INDEPENDENT_CODE=ON
```

**`-fstack-protector-strong`** — Détecte les débordements de buffer sur la stack (section 45.4.1).

**`-D_FORTIFY_SOURCE=2`** — Active les vérifications de taille dans les fonctions de manipulation de chaînes et de mémoire (section 45.4.2).

**`-Wl,-z,relro,-z,now`** — Full RELRO : rend la GOT (Global Offset Table) en lecture seule après le chargement des librairies, empêchant sa réécriture par un exploit.

**`-pie`** — Produit un exécutable indépendant de la position, nécessaire pour bénéficier pleinement de l'ASLR (Address Space Layout Randomization) dans le conteneur (section 45.4.3).

Ces flags ajoutent une protection en profondeur : même si un attaquant trouve une vulnérabilité dans votre code C++, chaque couche rend l'exploitation plus difficile.

### Scan de vulnérabilités

Les librairies embarquées dans l'image (via APT ou copiées manuellement) peuvent contenir des CVE (Common Vulnerabilities and Exposures) connues. Un scan automatisé de l'image permet de les détecter avant le déploiement.

Plusieurs outils sont disponibles. **Trivy** (open-source, par Aqua Security) est le plus répandu pour les images Docker :

```bash
# Scanner une image locale
trivy image myapp:latest

# Scanner et échouer si des vulnérabilités critiques sont trouvées
trivy image --severity CRITICAL --exit-code 1 myapp:latest
```

**Docker Scout** est intégré nativement depuis Docker Desktop 4.17 :

```bash
docker scout cves myapp:latest
```

Ces scans doivent être intégrés dans le pipeline CI/CD (chapitre 38) pour bloquer le déploiement d'images vulnérables. Un scan typique sur une image Debian slim avec OpenSSL et curl peut remonter quelques CVE de sévérité faible ou moyenne — l'important est de traiter les CVE critiques et élevées en priorité, et de reconstruire l'image régulièrement pour intégrer les correctifs de sécurité APT.

Les images distroless et scratch contiennent moins de composants et donc moins de CVE potentielles — c'est un argument supplémentaire en faveur de la minimalisation de l'image runtime.

---

## Reproductibilité du build

### Pinning des versions d'images de base

Un `FROM ubuntu:24.04` est en réalité une cible mouvante : Canonical publie régulièrement des mises à jour de sécurité pour cette image. Un `docker pull ubuntu:24.04` effectué le lundi et le vendredi peut ramener deux images différentes.

Pour la majorité des projets, c'est un avantage — vous bénéficiez automatiquement des correctifs de sécurité. Mais si la reproductibilité bit-à-bit est une exigence, il faut **pinner le digest SHA256** de l'image :

```dockerfile
# Tag mutable — peut changer entre deux pulls
FROM ubuntu:24.04

# Digest immuable — toujours la même image
FROM ubuntu:24.04@sha256:6a7b8c9d0e1f2a3b4c5d6e7f8a9b0c1d2e3f4a5b6c7d8e9f0a1b2c3d4e5f6a7
```

Le digest est un hash SHA256 du manifeste de l'image. Il est calculé sur le contenu exact des layers — toute modification (même un correctif de sécurité) produit un digest différent. Le pinning garantit que vous construisez toujours sur la même base.

Pour récupérer le digest d'une image :

```bash
docker inspect --format='{{index .RepoDigests 0}}' ubuntu:24.04
```

Le compromis est clair : le pinning améliore la reproductibilité mais bloque les mises à jour de sécurité automatiques. La bonne pratique est de combiner les deux approches : pinner le digest dans le Dockerfile pour la reproductibilité, et utiliser un outil automatisé (Dependabot, Renovate) pour proposer des mises à jour régulières du digest via des pull requests. Ainsi, chaque mise à jour est explicite, traçable, et vérifiable dans l'historique Git.

### Pinning des versions de paquets APT

Le même problème existe pour les paquets installés via `apt-get` :

```dockerfile
# Version flottante — le contenu change avec les mises à jour du dépôt
RUN apt-get install -y libssl3

# Version pinnée — toujours le même paquet
RUN apt-get install -y libssl3=3.0.13-1ubuntu3.4
```

Le pinning des paquets APT est plus contraignant en pratique : les anciennes versions sont supprimées des dépôts au fil du temps, ce qui peut faire échouer le build. Un compromis raisonnable est de pinner uniquement les paquets critiques (le compilateur GCC, les librairies de sécurité comme OpenSSL) et de laisser les paquets utilitaires en version flottante.

### Pinning des dépendances Conan/vcpkg

Conan et vcpkg possèdent leurs propres mécanismes de lockfile :

```bash
# Conan : générer un lockfile
conan lock create .

# Le fichier conan.lock est commité dans Git
# et utilisé dans le Dockerfile
```

```dockerfile
COPY conanfile.py conan.lock ./  
RUN conan install . --lockfile=conan.lock --build=missing  
```

Le lockfile gèle les versions exactes de toutes les dépendances (y compris les dépendances transitives) au moment de sa création. Tant que le lockfile est le même, les mêmes versions sont installées — indépendamment des nouvelles versions publiées en amont.

Pour vcpkg, le fichier `vcpkg.json` avec les champs `version>=` et le registre de versions (`vcpkg-configuration.json`) jouent le même rôle.

### Désactiver la mise à jour des index APT en production

Dans un contexte de reproductibilité stricte, les dépôts APT eux-mêmes peuvent changer entre deux builds. Une technique avancée consiste à utiliser des **snapshots de dépôts** :

```dockerfile
# Utiliser un snapshot daté des dépôts Debian
RUN echo "deb [check-valid-until=no] https://snapshot.debian.org/archive/debian/20260301T000000Z bookworm main" \
    > /etc/apt/sources.list \
    && apt-get update && apt-get install -y --no-install-recommends \
    libssl3 libcurl4 \
    && rm -rf /var/lib/apt/lists/*
```

Le service `snapshot.debian.org` archive l'état des dépôts Debian à chaque instant. En pointant vers un snapshot daté, vous obtenez exactement les mêmes paquets à chaque build. L'option `check-valid-until=no` est nécessaire car les signatures des snapshots anciens expirent.

C'est une mesure avancée qui n'est justifiée que pour les environnements réglementés exigeant une traçabilité absolue du contenu de l'image.

---

## Optimisation des layers et du cache

### Un `RUN` = un concept

Chaque instruction `RUN` crée un layer dans l'image Docker. Ces layers sont empilés et chacun consomme de l'espace disque. La stratégie d'organisation des layers doit équilibrer deux objectifs contradictoires : **granularité du cache** (plus de layers = cache plus fin) et **taille de l'image** (moins de layers = moins d'overhead).

La règle pratique est de regrouper dans un seul `RUN` les opérations qui forment une unité logique :

```dockerfile
# ✅ Bon — installation et nettoyage dans le même layer
RUN apt-get update && apt-get install -y --no-install-recommends \
    libssl3 libcurl4 ca-certificates \
    && rm -rf /var/lib/apt/lists/*

# ❌ Mauvais — le cache APT est capturé dans le premier layer
RUN apt-get update  
RUN apt-get install -y libssl3 libcurl4  
RUN rm -rf /var/lib/apt/lists/*  
```

Dans la version incorrecte, le premier `RUN apt-get update` crée un layer contenant le cache APT (~30 MB). Le `rm` dans le troisième `RUN` supprime les fichiers, mais le premier layer conserve ces 30 MB à jamais. Les layers Docker sont additifs : supprimer un fichier dans un layer ultérieur ne réduit pas la taille de l'image.

### Ordonnancement des instructions `COPY`

Nous avons abordé ce sujet en section 37.2.1, mais il mérite d'être formalisé. L'ordonnancement optimal des instructions dans un Dockerfile C++ est :

```
FROM ...                           ← Change quasiment jamais  
ENV ...                            ← Change très rarement  
RUN apt-get install (toolchain)    ← Change très rarement  
RUN apt-get install (dépendances)  ← Change rarement  
COPY conanfile.py / vcpkg.json     ← Change occasionnellement  
RUN conan install / vcpkg install  ← Invalidé si le fichier ci-dessus change  
COPY CMakeLists.txt cmake/         ← Change de temps en temps  
COPY src/ include/                 ← Change à chaque commit  
RUN cmake -B build ...             ← Invalidé si les sources changent  
RUN cmake --build build            ← Invalidé si les sources changent  
```

Docker invalide le cache d'une instruction dès que cette instruction ou l'une de ses prédécesseurs change. En plaçant ce qui change le plus souvent à la fin, vous maximisez le nombre de layers servis depuis le cache.

### Labels et métadonnées

Les labels OCI (Open Container Initiative) documentent l'image de manière standardisée :

```dockerfile
LABEL org.opencontainers.image.title="myapp" \
      org.opencontainers.image.version="2.1.0" \
      org.opencontainers.image.description="HTTP API server" \
      org.opencontainers.image.source="https://github.com/org/myapp" \
      org.opencontainers.image.revision="${GIT_SHA}" \
      org.opencontainers.image.created="${BUILD_DATE}" \
      org.opencontainers.image.licenses="MIT"
```

Pour injecter le SHA Git et la date de build dynamiquement :

```dockerfile
ARG GIT_SHA=unknown  
ARG BUILD_DATE=unknown  

LABEL org.opencontainers.image.revision="${GIT_SHA}" \
      org.opencontainers.image.created="${BUILD_DATE}"
```

```bash
docker build \
    --build-arg GIT_SHA=$(git rev-parse HEAD) \
    --build-arg BUILD_DATE=$(date -u +"%Y-%m-%dT%H:%M:%SZ") \
    -t myapp:latest .
```

Ces labels sont consultables via `docker inspect` et permettent de tracer précisément quel commit Git correspond à quelle image de production — une information précieuse lors du diagnostic d'un incident.

---

## Reproductibilité au niveau du binaire

### Builds déterministes (reproducible builds)

Au-delà de l'image Docker, la reproductibilité du **binaire C++ lui-même** est un sujet avancé mais de plus en plus important. Par défaut, GCC et Clang intègrent des informations non déterministes dans le binaire : le chemin absolu du fichier source, la date de compilation (`__DATE__`, `__TIME__`), et l'ordre de traitement des fichiers (qui peut varier selon le système de fichiers).

Pour produire un binaire identique bit-à-bit à chaque build :

```dockerfile
RUN cmake -B build -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_CXX_FLAGS="-ffile-prefix-map=/src=. -fdebug-prefix-map=/src=." \
    -DCMAKE_C_FLAGS="-ffile-prefix-map=/src=. -fdebug-prefix-map=/src=."
```

**`-ffile-prefix-map=/src=.`** — Remplace le chemin absolu `/src` (le `WORKDIR` dans le conteneur) par `.` dans les chaînes du binaire. Deux builds dans des répertoires différents produisent le même résultat.

**`-fdebug-prefix-map=/src=.`** — Même chose pour les informations de debug DWARF.

Il faut aussi éviter les macros `__DATE__` et `__TIME__` dans votre code source, ou les neutraliser via les flags du préprocesseur :

```dockerfile
-DCMAKE_CXX_FLAGS="-Werror=date-time"
```

Le flag `-Werror=date-time` transforme l'utilisation de `__DATE__` et `__TIME__` en erreur de compilation, forçant leur suppression du code source.

### Vérifier la reproductibilité

Deux builds successifs du même commit devraient produire des binaires identiques (ou au moins des images Docker identiques en termes de contenu fonctionnel). Vous pouvez vérifier cela :

```bash
# Build 1
docker build -t myapp:check1 .
# Build 2
docker build -t myapp:check2 .

# Comparer les binaires extraits
docker create --name c1 myapp:check1  
docker create --name c2 myapp:check2  
docker cp c1:/usr/local/bin/myapp /tmp/myapp1  
docker cp c2:/usr/local/bin/myapp /tmp/myapp2  
sha256sum /tmp/myapp1 /tmp/myapp2  
docker rm c1 c2  
```

Si les hash diffèrent, des informations non déterministes se sont glissées dans le build. Les causes les plus courantes sont `__DATE__`/`__TIME__`, des chemins absolus dans les informations de debug, ou un ordre de linkage non déterministe.

---

## Gestion des secrets dans le build

### Le problème

Certains builds C++ nécessitent des secrets : un token pour accéder à un registry Conan privé, une clé SSH pour cloner un dépôt Git privé contenant une librairie interne, ou des credentials pour télécharger un SDK propriétaire. Ces secrets ne doivent **jamais** apparaître dans l'image finale.

### Ce qu'il ne faut jamais faire

```dockerfile
# ❌ DANGEREUX — le token est capturé dans un layer Docker
ENV CONAN_LOGIN_USERNAME=deploy  
ENV CONAN_PASSWORD=s3cr3t_t0ken  
RUN conan install . --build=missing  

# ❌ DANGEREUX — la clé SSH est dans un layer
COPY id_rsa /root/.ssh/id_rsa  
RUN git clone git@github.com:org/private-lib.git  
RUN rm /root/.ssh/id_rsa  # Trop tard : le fichier est dans le layer COPY  
```

Même si le secret est supprimé dans un `RUN` ultérieur, il reste accessible dans le layer qui l'a introduit. N'importe qui ayant accès à l'image peut extraire le secret via `docker history` ou en inspectant les layers.

### Secrets BuildKit

Docker BuildKit propose un mécanisme sûr pour injecter des secrets pendant le build sans les capturer dans un layer :

```dockerfile
# Le secret est monté temporairement, uniquement pendant ce RUN
RUN --mount=type=secret,id=conan_token \
    CONAN_LOGIN_USERNAME=deploy \
    CONAN_PASSWORD=$(cat /run/secrets/conan_token) \
    conan install . --build=missing
```

```bash
# Le secret est passé au build via --secret
echo "s3cr3t_t0ken" > /tmp/conan_token  
docker buildx build --secret id=conan_token,src=/tmp/conan_token -t myapp .  
```

Le secret est disponible dans le conteneur pendant l'exécution de l'instruction `RUN`, mais n'est capturé dans aucun layer. Après l'exécution du `RUN`, le fichier `/run/secrets/conan_token` disparaît.

Pour les clés SSH (clone de dépôts privés) :

```dockerfile
RUN --mount=type=ssh \
    git clone git@github.com:org/private-lib.git /src/deps/private-lib
```

```bash
# L'agent SSH de l'hôte est forwarded au build
docker buildx build --ssh default -t myapp .
```

---

## `.dockerignore` complet pour un projet C++

Le fichier `.dockerignore` contrôle ce qui est envoyé au daemon Docker lors du build. Un `.dockerignore` strict accélère le build (moins de données à transférer) et élimine le risque de fuite de fichiers sensibles dans l'image.

Voici un `.dockerignore` complet et commenté pour un projet C++ professionnel :

```dockerignore
# ── Artefacts de build ─────────────────────────
build/  
cmake-build-*/  
out/  
_build/
*.o
*.a
*.so
*.d

# ── Contrôle de version ───────────────────────
.git/
.gitignore
.gitmodules

# ── IDE et éditeurs ───────────────────────────
.vscode/
.idea/
*.swp
*.swo
*~
.cache/
compile_commands.json

# ── Gestionnaires de dépendances (cache local) ─
.conan2/
vcpkg_installed/
_deps/

# ── Docker (éviter la récursion) ──────────────
Dockerfile*  
docker-compose*.yml  
.dockerignore

# ── CI/CD ─────────────────────────────────────
.github/
.gitlab-ci.yml
Jenkinsfile

# ── Documentation et métadonnées ──────────────
docs/
*.md
LICENSE  
CHANGELOG*  
CONTRIBUTING*  

# ── Tests de données volumineux ───────────────
test/data/  
benchmarks/data/  

# ── Secrets (ne devraient pas être dans le repo,
#    mais ceinture et bretelles) ───────────────
*.pem
*.key
.env
.env.*
```

---

## Checklist de production

Voici une liste de vérification synthétique à parcourir avant de déployer une image C++ en production :

```
Sécurité
  □ Le conteneur tourne en utilisateur non-root (USER dans le Dockerfile)
  □ Aucun shell n'est nécessaire (ou image distroless utilisée)
  □ Le binaire est compilé avec -fstack-protector-strong et -D_FORTIFY_SOURCE=2
  □ Full RELRO activé (-Wl,-z,relro,-z,now)
  □ PIE activé (CMAKE_POSITION_INDEPENDENT_CODE=ON)
  □ Aucun secret n'apparaît dans les layers (docker history --no-trunc)
  □ Scan de vulnérabilités exécuté (trivy / docker scout)
  □ SecurityContext Kubernetes configuré (runAsNonRoot, drop ALL)

Reproductibilité
  □ Image de base pinnée par digest SHA256 (ou tag + Renovate/Dependabot)
  □ Dépendances Conan/vcpkg verrouillées par lockfile
  □ Labels OCI présents (version, commit, date de build)
  □ .dockerignore couvre les artefacts de build, .git, IDE, secrets

Optimisation
  □ Multi-stage build : stage build séparé du stage runtime
  □ Image runtime slim ou distroless
  □ Aucun paquet -dev dans le stage runtime
  □ Layers ordonnés par fréquence de changement
  □ ccache / sccache intégré via BuildKit cache mount
  □ Toutes les librairies .so sont résolues (ldd sans "not found")
```

---

Les pratiques présentées dans cette section constituent le socle de toute conteneurisation C++ professionnelle. La section suivante pousse l'optimisation encore plus loin avec les images distroless — des conteneurs si minimalistes qu'ils ne contiennent littéralement rien d'autre que le binaire et ses dépendances.

---


⏭️ [Distroless images : Conteneurs minimaux](/37-dockerisation/05-distroless.md)
