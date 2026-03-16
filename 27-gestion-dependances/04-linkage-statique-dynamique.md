🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 27.4 Linkage statique (`.a`) vs dynamique (`.so`)

> **Objectif** : Comprendre les mécanismes du linkage statique et dynamique sous Linux, leurs implications concrètes sur la taille des binaires, les performances, le déploiement et la maintenance, et savoir choisir le bon mode selon le contexte.

---

## Deux façons d'assembler un programme

Quand le linker (`ld`) construit un exécutable à partir de fichiers objets et de bibliothèques, il dispose de deux mécanismes pour incorporer le code des bibliothèques :

**Le linkage statique** copie le code de la bibliothèque directement dans l'exécutable. Le fichier de bibliothèque est une archive `.a` (un regroupement de fichiers objets `.o`). Après le linkage, l'exécutable est autonome — il n'a plus besoin du fichier `.a` pour s'exécuter.

**Le linkage dynamique** enregistre une **référence** vers la bibliothèque dans l'exécutable. Le fichier de bibliothèque est un *shared object* `.so`. Au moment de l'exécution, le *dynamic linker* (`ld-linux-x86-64.so`) charge le `.so` en mémoire et résout les symboles. L'exécutable dépend de la présence du `.so` au runtime.

```
                    Linkage statique              Linkage dynamique
                    ================              =================

  Compilation       main.o + libfoo.a             main.o + libfoo.so
       │                   │                              │
       ▼                   ▼                              ▼
  Linkage          Code de foo copié              Référence à foo.so
                   dans l'exécutable              enregistrée dans l'ELF
       │                                                  │
       ▼                                                  ▼
  Exécution        Binaire autonome               Binaire + libfoo.so
                   (pas de .so nécessaire)        (chargé par ld-linux au runtime)
```

---

## Anatomie des fichiers

### Bibliothèque statique : `.a`

Une bibliothèque statique est une **archive** de fichiers objets, créée par `ar` :

```bash
# Créer une bibliothèque statique manuellement
g++ -c foo.cpp -o foo.o  
g++ -c bar.cpp -o bar.o  
ar rcs libmylib.a foo.o bar.o  

# Inspecter le contenu
ar t libmylib.a
# foo.o
# bar.o

# Voir les symboles
nm libmylib.a
```

Convention de nommage : `lib<nom>.a`. CMake produit ce fichier avec `add_library(mylib STATIC ...)`.

Le linker extrait de l'archive **uniquement** les fichiers objets qui contiennent des symboles référencés par le programme. Si votre code n'appelle aucune fonction de `bar.o`, ce fichier objet n'est pas inclus dans l'exécutable final. Cette granularité est au niveau du fichier objet, pas de la fonction individuelle (sauf avec LTO — section 41.5).

### Bibliothèque dynamique : `.so`

Une bibliothèque dynamique est un fichier ELF spécial, compilé avec le flag `-fPIC` (*Position Independent Code*) :

```bash
# Créer une bibliothèque dynamique manuellement
g++ -fPIC -c foo.cpp -o foo.o  
g++ -fPIC -c bar.cpp -o bar.o  
g++ -shared -o libmylib.so foo.o bar.o  

# Voir les dépendances dynamiques d'un exécutable
ldd my_app
#   linux-vdso.so.1
#   libmylib.so => /usr/local/lib/libmylib.so
#   libstdc++.so.6 => /usr/lib/x86_64-linux-gnu/libstdc++.so.6
#   libc.so.6 => /lib/x86_64-linux-gnu/libc.so.6
```

Convention de nommage : `lib<nom>.so`. Avec versioning : `lib<nom>.so.<major>.<minor>.<patch>`, accompagné de symlinks :

```
libmylib.so       → libmylib.so.1       (linker name — utilisé à la compilation)  
libmylib.so.1     → libmylib.so.1.2.0   (soname — utilisé au runtime)  
libmylib.so.1.2.0                        (fichier réel)  
```

Le **soname** (`libmylib.so.1`) est encodé dans l'exécutable qui lie contre cette bibliothèque. Au runtime, le dynamic linker cherche un fichier correspondant au soname, pas au nom exact. Cela permet de mettre à jour la bibliothèque (passer de 1.2.0 à 1.2.1) sans recompiler les exécutables — tant que le soname reste `libmylib.so.1`.

---

## Comparaison détaillée

### Taille du binaire

Le linkage statique produit des **exécutables plus gros** car le code de chaque bibliothèque est copié dans le binaire. Si trois exécutables lient statiquement la même bibliothèque, le code est triplé.

Le linkage dynamique produit des **exécutables plus petits** car ils ne contiennent que les références aux `.so`. La bibliothèque elle-même existe en un seul exemplaire sur le système de fichiers.

```bash
# Comparaison sur un projet réel (indicatif)
$ ls -lh build-static/apps/my-app
-rwxr-xr-x 1 dev dev 12M  my-app           # Statique — bibliothèques embarquées

$ ls -lh build-shared/apps/my-app
-rwxr-xr-x 1 dev dev 840K my-app            # Dynamique — seulement les références
```

Toutefois, la taille **totale de l'installation** (exécutable + bibliothèques partagées) est souvent comparable. Le gain réel du dynamique apparaît quand plusieurs exécutables partagent les mêmes bibliothèques.

### Performances au démarrage

Le linkage dynamique impose un **coût au démarrage** : le dynamic linker doit charger les `.so` en mémoire et résoudre les symboles (relocation). Pour un programme avec peu de dépendances, ce coût est négligeable (quelques millisecondes). Pour un programme avec des centaines de bibliothèques partagées (certaines applications KDE/GNOME), il peut atteindre plusieurs centaines de millisecondes.

Le linkage statique n'a pas ce coût — le code est déjà dans le binaire, prêt à s'exécuter.

### Performances à l'exécution

Le code généré est légèrement différent. Le linkage dynamique nécessite du code **PIC** (*Position Independent Code*), qui utilise un niveau d'indirection supplémentaire (la GOT — *Global Offset Table*) pour accéder aux données et fonctions globales. Sur les architectures modernes (x86_64), l'overhead PIC est minime (de l'ordre de 1-2%) grâce au support matériel. Sur certaines architectures (ARM 32 bits), il peut être plus significatif.

Le linkage statique peut bénéficier davantage de **LTO** (*Link-Time Optimization*, section 41.5) car le linker a accès à l'ensemble du code et peut optimiser à travers les frontières de bibliothèques.

### Déploiement et distribution

C'est souvent le critère le plus décisif en pratique.

**Statique** : l'exécutable est un fichier unique et autonome. Vous le copiez sur la machine cible et il fonctionne. Pas de problème de version de `.so` manquante, pas de `LD_LIBRARY_PATH` à configurer, pas de dépendance runtime (à l'exception de la libc et parfois de `libpthread`). C'est l'approche idéale pour les outils CLI, les conteneurs Docker minimalistes, et les déploiements embarqués.

**Dynamique** : l'exécutable nécessite que toutes les `.so` soient présentes au runtime, aux bons emplacements, dans des versions compatibles. Sur un système où les bibliothèques sont gérées par le gestionnaire de paquets (`apt`), c'est transparent. Sur un conteneur Docker ou un système embarqué, il faut s'assurer que les bonnes `.so` sont incluses. Le célèbre `error while loading shared libraries: libfoo.so.1: cannot open shared object file` est la manifestation la plus courante d'un problème de déploiement dynamique.

### Mises à jour de sécurité

**Dynamique** : quand une faille est découverte dans OpenSSL, il suffit de mettre à jour `libssl.so` sur le système. Tous les programmes qui la partagent bénéficient immédiatement du correctif sans recompilation.

**Statique** : chaque exécutable qui a embarqué OpenSSL en statique doit être **recompilé et redéployé** pour intégrer le correctif. Si vous avez dix services qui lient OpenSSL statiquement, vous devez rebuilder et redéployer les dix.

C'est un argument majeur en faveur du dynamique dans les environnements de production où la réactivité aux CVE est critique.

### Partage mémoire

Quand plusieurs processus chargent la même `.so`, le système d'exploitation ne charge le **segment de code** (texte) qu'une seule fois en mémoire physique. Chaque processus dispose de son propre mapping virtuel, mais la mémoire physique est partagée. C'est une économie significative sur les systèmes avec de nombreux processus utilisant les mêmes bibliothèques (serveurs web, environnements de bureau).

Avec le linkage statique, chaque processus a sa propre copie du code en mémoire. Aucun partage n'est possible.

---

## Tableau récapitulatif

| Critère | Statique (`.a`) | Dynamique (`.so`) |
|---------|:--------------:|:-----------------:|
| Taille de l'exécutable | Plus gros | Plus petit |
| Autonomie au runtime | ✅ Binaire autonome | ❌ Nécessite les `.so` |
| Déploiement | ✅ Simple — un fichier | ⚠️ Gérer les dépendances runtime |
| Mises à jour de sécurité | ❌ Recompilation nécessaire | ✅ Mise à jour du `.so` suffit |
| Partage mémoire entre processus | ❌ | ✅ |
| Performance au démarrage | ✅ Pas de résolution dynamique | ⚠️ Coût de relocation |
| Performance à l'exécution | ✅ Léger avantage (pas de PIC) | ~Équivalent sur x86_64 |
| LTO (optimisation globale) | ✅ Plein bénéfice | ⚠️ Limité aux frontières |
| Taille totale (multi-binaire) | ❌ Code dupliqué | ✅ Code partagé |
| Conflits de versions | ❌ Risque de duplication ODR | ⚠️ « .so hell » |

---

## Choix selon le contexte

### Outil CLI ou utilitaire autonome → statique

Un outil en ligne de commande (à la `kubectl`, `ripgrep`, `fd`) est distribué comme un binaire unique que l'utilisateur télécharge et exécute. Le linkage statique est le choix naturel : aucune dépendance, déploiement trivial, fonctionnement garanti sur toute distribution Linux compatible.

```cmake
add_executable(my_tool main.cpp)  
target_link_libraries(my_tool PRIVATE my_project::core)  
target_link_options(my_tool PRIVATE -static-libstdc++ -static-libgcc)  
```

### Service cloud / conteneur Docker → statique (souvent)

Pour un microservice déployé dans un conteneur Docker, le linkage statique permet d'utiliser des **images distroless** ou `scratch` — des images sans système de fichiers de base, réduisant la surface d'attaque et la taille de l'image :

```dockerfile
# Multi-stage : compilation dans une image complète
FROM ubuntu:24.04 AS builder
# ... compilation en statique ...

# Exécution dans une image minimale
FROM gcr.io/distroless/cc-debian12  
COPY --from=builder /app/my-service /my-service  
ENTRYPOINT ["/my-service"]  
```

Attention toutefois aux mises à jour de sécurité : si le service lie OpenSSL statiquement, chaque CVE nécessite un rebuild de l'image.

### Bibliothèque système / partagée entre applications → dynamique

Si votre bibliothèque est installée sur le système via un paquet `.deb` et consommée par plusieurs applications, le linkage dynamique est le choix correct. C'est le modèle de distribution standard sous Linux — `apt install libssl3` installe la `.so` que toutes les applications partagent.

### Application de bureau avec plugins → dynamique

Les systèmes de plugins reposent par nature sur le chargement dynamique (`dlopen`). L'application principale et les plugins doivent être des `.so` ou charger des `.so`. Le linkage dynamique est obligatoire pour ce pattern.

### Embarqué / RTOS → statique (presque toujours)

Les systèmes embarqués ont souvent un système de fichiers minimal ou inexistant. Le linkage statique produit un binaire unique chargé en mémoire, sans dépendance au système de fichiers runtime. C'est le mode par défaut dans la plupart des toolchains embarquées.

---

## Contrôler le mode de linkage

### Dans CMake

```cmake
# Au niveau du projet — affecte add_library() sans type explicite
set(BUILD_SHARED_LIBS OFF)   # Statique par défaut  
set(BUILD_SHARED_LIBS ON)    # Dynamique par défaut  

# Au niveau d'une cible spécifique
add_library(my_lib STATIC src/foo.cpp)   # Toujours statique  
add_library(my_lib SHARED src/foo.cpp)   # Toujours dynamique  
add_library(my_lib src/foo.cpp)          # Dépend de BUILD_SHARED_LIBS  
```

### Dans Conan

```python
# conanfile.py
default_options = {
    "*:shared": False,   # Toutes les dépendances en statique
}
```

Ou en ligne de commande :

```bash
conan install . -o "*:shared=True" --build=missing
```

### Dans vcpkg

Le mode de linkage est contrôlé par le **triplet** :

```bash
# Dynamique
cmake -B build -DVCPKG_TARGET_TRIPLET=x64-linux ...

# Statique
cmake -B build -DVCPKG_TARGET_TRIPLET=x64-linux-static ...
```

---

## Le piège du mélange statique/dynamique

Un piège classique est de mélanger des bibliothèques statiques et dynamiques dans le même exécutable. C'est techniquement possible et courant, mais peut causer des problèmes subtils.

### Duplication de symboles

Si votre exécutable lie `libA.a` (statique) et `libB.so` (dynamique), et que les deux dépendent de `libC`, vous pouvez vous retrouver avec **deux copies** de `libC` dans le processus : une embarquée dans l'exécutable (via `libA.a`) et une chargée dynamiquement (via `libB.so`). Si `libC` contient des variables globales ou des singletons, les deux copies sont indépendantes, ce qui viole le principe de l'instance unique et peut causer des bugs mystérieux.

### Règle pratique

Choisissez un mode et restez-y pour l'ensemble du projet. Si vous liez vos bibliothèques principales en statique, liez aussi leurs dépendances en statique. Le mélange est parfois inévitable (la libc est presque toujours dynamique), mais minimisez-le.

---

## Vérifier le mode de linkage d'un binaire

```bash
# Lister les dépendances dynamiques
ldd build/apps/my-app
# Si la sortie est courte (juste libc, libpthread, ld-linux), le binaire est
# essentiellement statique.

# Pour un binaire entièrement statique :
ldd build/apps/my-app
# not a dynamic executable

# Vérifier si un binaire est statique ou dynamique
file build/apps/my-app
# ... statically linked         → entièrement statique
# ... dynamically linked        → au moins une dépendance dynamique

# Lister les symboles nécessaires non résolus (undefined)
nm -u build/apps/my-app | head
```

---

> **À suivre** : La section 27.5 couvre l'installation et la distribution de bibliothèques sur un système Linux — les emplacements conventionnels, `pkg-config`, et les bonnes pratiques pour rendre vos bibliothèques consommables par d'autres projets.

⏭️ [Installation et distribution de librairies sur Linux](/27-gestion-dependances/05-distribution-linux.md)
