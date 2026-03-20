🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 38.3 Accélération CI : ccache et sccache (cache distribué) 🔥

## Introduction

La compilation C++ est lente. C'est un fait structurel du langage : chaque unité de traduction inclut et parse ses headers (souvent des milliers de lignes avant même d'atteindre le code métier), le compilateur effectue des optimisations coûteuses, et les templates génèrent du code à chaque instanciation. Un projet de taille moyenne peut nécessiter 5 à 15 minutes de compilation complète ; un projet de grande envergure dépasse facilement la demi-heure.

En CI, ce coût est payé à chaque exécution du pipeline. Sur un projet actif avec 20 à 30 push par jour et une matrice de 4 combinaisons compilateur/standard, cela représente des centaines de minutes de compilation quotidiennes — dont l'immense majorité recompile des fichiers source qui n'ont pas changé depuis l'exécution précédente.

Le cache de compilation est la réponse à ce problème. En stockant les résultats des compilations précédentes et en les réutilisant lorsque les entrées sont identiques, un cache bien configuré peut réduire les temps de build de 80 à 95 % sur les exécutions incrémentales. La différence entre un pipeline sans cache (12 minutes) et un pipeline avec cache chaud (90 secondes) change fondamentalement l'expérience de développement : le feedback passe de "je vais chercher un café" à "je regarde le résultat immédiatement".

Cette section couvre les deux outils de cache de compilation dominants dans l'écosystème C++ — **ccache** et **sccache** — et les stratégies pour les exploiter efficacement dans un contexte CI.

## Le problème : anatomie d'un build CI sans cache

Pour comprendre l'impact du cache, examinons ce qui se passe lors d'un build CI typique sans aucune forme de cache. Prenons un projet de taille moyenne : 200 fichiers source C++, 50 000 lignes de code, utilisant la STL et quelques librairies tierces.

### Scénario : un développeur pousse un commit modifiant 3 fichiers

Sans cache, le pipeline effectue une **compilation complète from scratch** :

```
Étape                         Durée typique
─────────────────────────────────────────────
Checkout du code               5s  
Installation des dépendances   30s (Conan/vcpkg)  
CMake configure                10s  
Compilation (200 fichiers)     8min  
Link                           15s  
Tests                          30s  
─────────────────────────────────────────────
Total                          ~10min
```

Les 200 fichiers sont recompilés intégralement, alors que seuls 3 ont changé. Les 197 fichiers restants produisent exactement les mêmes fichiers objets que lors de l'exécution précédente du pipeline. Ce travail est purement gaspillé.

### Le même scénario avec ccache (cache chaud)

Avec un cache de compilation correctement configuré et un historique de builds précédents :

```
Étape                         Durée typique
─────────────────────────────────────────────
Checkout du code               5s  
Restauration du cache          10s (download + extraction)  
Installation des dépendances   5s (cache Conan/vcpkg)  
CMake configure                5s  
Compilation (3 fichiers réels, 197 cache hits)   25s  
Link                           15s  
Tests                          30s  
Sauvegarde du cache            8s  
─────────────────────────────────────────────
Total                          ~1min 45s
```

Le temps de compilation passe de 8 minutes à 25 secondes. Les 197 fichiers inchangés sont servis directement depuis le cache sans invoquer le compilateur. Seuls les 3 fichiers modifiés (et éventuellement leurs dépendants directs) sont réellement compilés.

Ce facteur d'accélération de 5x à 10x n'est pas théorique — c'est ce qu'on observe en pratique sur des projets réels avec des taux de cache hit de 85 à 98%.

## ccache vs sccache : deux philosophies

Les deux outils remplissent le même rôle fondamental — intercepter les appels au compilateur et retourner un résultat caché lorsque les entrées sont identiques — mais divergent dans leur architecture et leurs cas d'usage.

### ccache — Le standard historique

ccache (Compiler Cache) est l'outil de cache de compilation le plus ancien et le plus éprouvé de l'écosystème C/C++. Créé en 2002 par Andrew Tridgell (le créateur de rsync et Samba), il est activement maintenu et utilisé par une grande partie de l'industrie.

**Fonctionnement.** ccache s'interpose entre le build system et le compilateur. Lorsqu'il est invoqué à la place de `g++` ou `clang++`, il calcule un hash des entrées de la compilation (fichier source préprocessé, flags de compilation, version du compilateur) et vérifie si un résultat correspondant existe dans son cache local. Si oui, il retourne le fichier objet caché sans invoquer le compilateur. Sinon, il appelle le compilateur réel, stocke le résultat dans le cache, et le retourne.

**Caractéristiques clés :**

- Cache local sur le système de fichiers (répertoire configurable, par défaut `~/.cache/ccache`).  
- Supporte GCC, Clang et MSVC.  
- Gestion automatique de la taille du cache avec éviction LRU.  
- Statistiques détaillées (hits, misses, taux de hit, taille du cache).  
- Extrêmement mature et stable — peu de surprises en production.  
- Aucune dépendance réseau.

**Limitation principale** : le cache est local au système de fichiers. En CI, cela signifie que le cache doit être sauvegardé et restauré entre les exécutions via le mécanisme de cache de la plateforme CI (cache GitLab, `actions/cache` sur GitHub). Chaque runner possède son propre cache, et il n'y a pas de partage natif entre runners.

### sccache — Le cache distribué

sccache (Shared Compilation Cache) est développé par Mozilla comme une alternative à ccache avec un focus sur le partage de cache entre machines. Il a été conçu pour les grands projets (Firefox en particulier) où de nombreux développeurs et runners CI compilent le même code.

**Fonctionnement.** Le principe de base est le même que ccache — interception des appels au compilateur et lookup par hash — mais sccache ajoute une couche de stockage distant. Le cache peut être stocké sur un backend cloud (Amazon S3, Google Cloud Storage, Azure Blob Storage) ou sur un serveur local, ce qui permet à tous les runners CI (et potentiellement aux développeurs) de partager le même cache.

**Caractéristiques clés :**

- Backends de stockage multiples : S3, GCS, Azure Blob, Redis, Memcached, système de fichiers local.  
- Cache partagé entre tous les runners et toutes les branches.  
- Supporte GCC, Clang, MSVC et Rust (rustc).  
- Mode serveur/client pour le partage en équipe.  
- Intégration native avec le cache GitHub Actions (`type=gha`).

**Limitations :**

- Latence réseau pour chaque lookup de cache (quelques millisecondes par fichier, cumulable sur un grand nombre de fichiers).  
- Configuration plus complexe que ccache (credentials cloud, permissions, endpoint).  
- Moins mature que ccache — des cas limites peuvent survenir avec certaines combinaisons de flags de compilation.

### Arbre de décision : quand choisir lequel

Le choix entre ccache et sccache dépend de l'infrastructure CI et de la taille du projet :

```
Votre projet a-t-il besoin de partager le cache  
entre plusieurs runners/branches ?  
│
├── NON ──▶ Utilisez ccache
│           (simple, éprouvé, aucune dépendance externe)
│
└── OUI ──▶ Avez-vous accès à un bucket cloud (S3/GCS/Azure) ?
            │
            ├── OUI ──▶ Utilisez sccache avec backend cloud
            │           (cache partagé, scalable, survit aux évictions)
            │
            └── NON ──▶ Utilisez sccache avec le cache GitHub Actions
                        (compromis : partagé mais limité à 10 Go par dépôt)
```

**En pratique, ccache est le choix par défaut pour la majorité des projets.** Sa simplicité et sa fiabilité le rendent adapté à la grande majorité des cas. sccache devient pertinent lorsque l'infrastructure CI comprend un pool de runners éphémères (Kubernetes, auto-scaling) où le cache local est perdu à chaque exécution, ou lorsque le projet est suffisamment grand pour qu'un cache partagé apporte un gain mesurable.

Les deux outils peuvent coexister dans une même organisation : ccache pour le développement local et les runners dédiés, sccache pour les runners éphémères cloud.

## Principe de fonctionnement détaillé

Pour exploiter efficacement le cache en CI, il faut comprendre ce que ccache et sccache hashent pour décider si un résultat peut être réutilisé. Les deux outils fonctionnent sur le même principe fondamental, avec des variations mineures.

### Ce qui entre dans le hash

Le hash qui identifie une entrée de cache est calculé à partir de :

1. **Le contenu préprocessé du fichier source.** Le fichier `.cpp` est d'abord passé au préprocesseur, qui résout les `#include`, les macros et les directives conditionnelles. C'est le résultat de cette étape qui est hashé, pas le fichier source brut. Cela signifie qu'un changement dans un commentaire ou un reformatage qui n'affecte pas le résultat du préprocesseur ne provoque pas de cache miss.

2. **Les flags de compilation.** `-O2`, `-std=c++20`, `-DNDEBUG`, `-fsanitize=address` et tous les autres flags sont inclus dans le hash. Changer un flag invalide l'entrée de cache — ce qui est le comportement correct car le fichier objet résultant serait différent.

3. **L'identité du compilateur.** Par défaut, ccache vérifie le chemin et le timestamp du compilateur (`mtime`). En environnement Docker, le timestamp peut varier entre deux builds de la même image, provoquant des invalidations de cache injustifiées. C'est pourquoi la variable `CCACHE_COMPILERCHECK=content` est recommandée en CI : elle utilise le hash du binaire du compilateur plutôt que son timestamp, ce qui est plus robuste dans les environnements conteneurisés.

4. **Le répertoire de travail** (optionnel). Par défaut, ccache inclut le répertoire courant dans le hash. En CI, le chemin du checkout peut varier (par exemple `/home/runner/work/projet/projet` sur GitHub vs `/builds/groupe/projet` sur GitLab). La variable `CCACHE_BASEDIR` permet de définir un préfixe à ignorer dans les chemins, rendant le cache portable entre différents chemins de checkout.

### Ce qui provoque un cache miss

Comprendre les causes de cache miss permet de les prévenir :

| Cause du miss | Impact | Remède |
|---------------|--------|--------|
| Fichier source modifié | Un seul fichier recompilé | Normal — c'est le comportement attendu |
| Header modifié | Tous les fichiers incluant ce header recompilés | Normal — architecture modulaire pour limiter l'impact |
| Flag de compilation changé | Tous les fichiers recompilés | Séparer les clés de cache par configuration (Debug/Release) |
| Version du compilateur changée | Cache entièrement invalidé | Séparer les clés de cache par compilateur |
| Macro `__TIME__` / `__DATE__` | Cache inutilisable pour les fichiers concernés | `CCACHE_SLOPPINESS=time_macros` |
| Chemin de checkout différent | Cache invalidé | `CCACHE_BASEDIR` |
| Image Docker reconstruite | Timestamp compilateur changé | `CCACHE_COMPILERCHECK=content` |

### Séparation des caches par configuration

Un point crucial en CI est la **séparation des caches par combinaison compilateur/type de build**. Le fichier objet produit par `g++-15 -O2 -DNDEBUG` (GCC Release) n'est pas réutilisable par `clang++-20 -O0 -g` (Clang Debug). Si les deux configurations partagent le même cache, chaque alternance provoque une invalidation complète et le taux de hit s'effondre.

La solution est simple : une clé de cache distincte par combinaison. Nous l'avons vu dans les sections précédentes :

**GitLab CI :**
```yaml
cache:
  key: "gcc15-release-${CI_COMMIT_REF_SLUG}"
```

**GitHub Actions (ccache-action) :**
```yaml
- uses: hendrikmuhs/ccache-action@v1
  with:
    key: gcc15-Release
```

Chaque combinaison dispose de son propre espace de cache, et le taux de hit reste optimal.

## Impact mesurable : métriques de référence

Pour illustrer l'impact concret du cache de compilation en CI, voici des métriques issues de scénarios représentatifs sur un projet de 150 fichiers source C++ (environ 40 000 lignes) :

| Scénario | Sans cache | Avec ccache (froid) | Avec ccache (chaud) |
|----------|------------|--------------------|--------------------|
| Build complet (from scratch) | 7 min 20s | 7 min 35s (+2%) | — |
| Commit modifiant 1 fichier | 7 min 20s | 7 min 35s | 45s (-90%) |
| Commit modifiant 5 fichiers | 7 min 20s | 7 min 35s | 1 min 10s (-84%) |
| Commit modifiant 20 fichiers | 7 min 20s | 7 min 35s | 2 min 30s (-66%) |
| Commit ne modifiant aucun source (docs, CI) | 7 min 20s | 7 min 35s | 20s (-95%) |

**Cache froid** : première exécution avec un cache vide. Le temps est légèrement supérieur au build sans cache (quelques pourcents) à cause du surcoût de hashing et de stockage dans le cache. C'est l'investissement initial.

**Cache chaud** : exécutions suivantes avec un cache alimenté par les builds précédents. L'accélération est spectaculaire, variant de 66% (modification significative) à 95% (aucun source modifié).

Le surcoût du cache froid est négligeable et amorti dès la deuxième exécution. La question n'est jamais "faut-il utiliser le cache ?" mais "comment maximiser le taux de hit ?".

## Interaction avec le build system

ccache et sccache s'intègrent à la chaîne de compilation via le concept de **compiler launcher** de CMake :

```bash
cmake -B build \
  -DCMAKE_CXX_COMPILER_LAUNCHER=ccache \
  -DCMAKE_C_COMPILER_LAUNCHER=ccache
```

Avec cette configuration, CMake génère des règles de build qui invoquent `ccache g++-15` au lieu de `g++-15` directement. ccache reçoit les mêmes arguments que le compilateur, effectue le lookup de cache, et soit retourne le résultat caché, soit délègue au compilateur réel.

Cette approche est propre et non-invasive : le build system ne change pas, le compilateur réel ne change pas, seule l'interception est ajoutée. Ninja et Make sont tous deux compatibles avec ce mécanisme.

> ⚠️ **Piège fréquent : oublier le launcher.** Installer ccache et configurer le cache ne suffit pas — il faut explicitement dire à CMake de l'utiliser via `CMAKE_CXX_COMPILER_LAUNCHER`. Sans cette directive, le compilateur est invoqué directement et le cache n'est jamais consulté. Les statistiques ccache montreront zéro hit et zéro miss, signalant que ccache n'est tout simplement pas sollicité. C'est la cause la plus fréquente de cache "inopérant" en CI.

## Coût du cache en CI : sauvegarde et restauration

Le cache de compilation n'est gratuit ni en temps ni en espace. En CI, il faut transférer le cache entre le stockage persistant et le runner à chaque exécution :

| Opération | Durée typique (cache de 500 Mo) | Durée typique (cache de 2 Go) |
|-----------|-------------------------------|-------------------------------|
| Download et extraction | 5-10s | 15-30s |
| Compression et upload | 8-15s | 20-45s |
| **Surcoût total** | **15-25s** | **35-75s** |

Ce surcoût est à comparer avec le temps économisé sur la compilation. Si le cache permet de gagner 6 minutes de compilation pour un surcoût de 30 secondes de transfert, le bénéfice net est de 5 minutes 30 — un retour sur investissement très favorable.

Cependant, un cache surdimensionné peut devenir contre-productif : si le transfert prend plus de temps que la compilation économisée (par exemple un cache de 5 Go pour un projet qui compile en 2 minutes), le cache ralentit le pipeline au lieu de l'accélérer. La taille maximale du cache (`CCACHE_MAXSIZE`) doit être calibrée en fonction du projet.

**Règle empirique** : commencez avec `CCACHE_MAXSIZE=2G`, surveillez le taux de hit et la taille effective via `ccache --show-stats`, puis ajustez. Si le taux de hit est élevé (>90%) et que le cache n'est pas plein, vous pouvez réduire la taille maximale. Si le taux de hit est bas à cause d'évictions fréquentes, augmentez-la.

## Ce qui suit

Les deux sous-sections suivantes détaillent la configuration concrète de chaque outil :

- **Section 38.3.1** — Configuration de ccache en CI : variables d'environnement, intégration GitLab CI et GitHub Actions, stratégies de clé de cache, monitoring et diagnostic des cache miss.  
- **Section 38.3.2** — sccache et le cache distribué : configuration des backends cloud (S3, GCS), intégration avec le cache GitHub Actions, et architecture pour les runners éphémères.

---

> 📎 *Cette section s'appuie sur les fondamentaux de ccache couverts en **section 2.3** (installation et configuration de ccache). Les concepts de base (installation, intégration CMake, statistiques) y sont détaillés — cette section se concentre sur les spécificités CI.*

⏭️ [Configuration ccache en CI](/38-cicd/03.1-ccache-ci.md)
