🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 2.3 — Accélération de compilation : ccache (Compiler Cache) ⭐

> **Chapitre 2 · Mise en place de la Toolchain sur Ubuntu** · Niveau Débutant

---

## Introduction

Sur un projet C++ de taille réelle, la compilation peut devenir un goulot d'étranglement significatif. Un rebuild complet d'un projet de 200 fichiers prend facilement plusieurs minutes, voire dizaines de minutes sur du code template-heavy. Et ce rebuild complet survient plus souvent qu'on ne le pense : après un `make clean`, un changement de branche Git, une modification d'un en-tête inclus partout, ou simplement un changement de machine (CI, nouveau poste de travail).

**ccache** (Compiler Cache) résout ce problème de manière transparente : il intercepte les appels au compilateur, calcule un hash des entrées (code source, options de compilation, en-têtes) et, si ce hash correspond à une compilation déjà effectuée, retourne le résultat depuis un cache local au lieu de relancer le compilateur. Le gain est spectaculaire : une recompilation qui prendrait 5 minutes tombe à quelques secondes.

ccache est l'un de ces outils qui, une fois configuré, se fait oublier — et dont l'absence se fait cruellement sentir quand on ne l'a pas.

---

## Comment fonctionne ccache

### Le principe : hash et cache

Quand ccache intercepte un appel de compilation (par exemple `g++ -std=c++23 -O2 -c main.cpp -o main.o`), il effectue les opérations suivantes :

1. **Calcul d'un hash** — ccache génère une empreinte unique à partir de l'ensemble des entrées qui déterminent le résultat de la compilation : le contenu du fichier source, le contenu de tous les en-têtes inclus (directement et transitivement), les options de compilation, la version du compilateur, et d'éventuelles variables d'environnement pertinentes.
2. **Recherche dans le cache** — si un fichier objet correspondant à ce hash existe déjà dans le cache, ccache le copie directement à la destination attendue. C'est un **cache hit**.
3. **Compilation effective** — si aucune entrée ne correspond (un **cache miss**), ccache passe l'appel au vrai compilateur, stocke le résultat dans le cache, et le retourne normalement.

Le résultat est que, du point de vue du build system (Ninja, Make, CMake), rien ne change : il invoque le compilateur comme d'habitude, et reçoit le fichier objet attendu. La seule différence est la vitesse.

### Modes de fonctionnement

ccache propose plusieurs modes, dont les deux principaux sont :

**Mode direct** — ccache calcule le hash directement depuis les fichiers source et les en-têtes, sans invoquer le préprocesseur. C'est le mode le plus rapide et celui utilisé par défaut. Il utilise un **inode cache** pour éviter de re-hasher les mêmes en-têtes à chaque compilation au sein d'un même build, ce qui accélère considérablement les projets avec beaucoup de fichiers partageant les mêmes includes.

**Mode depend** — ccache utilise les informations de dépendances générées par le compilateur (équivalent de `-MD`) pour déterminer les fichiers à hasher. C'est une alternative au mode direct qui peut mieux gérer certains cas particuliers.

### Algorithmes de hash et compression

ccache utilise l'algorithme **BLAKE3** pour le hash principal — un algorithme à la fois rapide et sûr, bien adapté au hashing de grandes quantités de données (des milliers de fichiers d'en-têtes). Les fichiers stockés dans le cache sont compressés avec **Zstandard** (zstd), un algorithme qui offre un excellent compromis entre taux de compression et vitesse. L'intégrité des fichiers en cache est vérifiée via des checksums **XXH3**.

---

## 2.3.1 — Installation et configuration de ccache

### Installation

Sur Ubuntu, ccache est disponible dans les dépôts standards :

```bash
sudo apt install ccache
```

Vérification :

```bash
ccache --version
# Output (exemple) :
# ccache version 4.9.1
```

La version fournie par les dépôts Ubuntu dépend de la release. Sur Ubuntu 24.04 LTS, c'est une version 4.9.x. En mars 2026, la dernière version stable de ccache est la **4.13**. Pour la plupart des usages, la version des dépôts est parfaitement suffisante. Si vous souhaitez la toute dernière version (pour bénéficier des dernières optimisations ou de nouvelles fonctionnalités), vous pouvez la télécharger depuis le [site officiel](https://ccache.dev/download.html) ou la compiler depuis les sources.

### Configuration de base

La configuration de ccache se fait via un fichier `ccache.conf` ou via des variables d'environnement. Le fichier de configuration utilisateur se situe dans `~/.config/ccache/ccache.conf`. Créez-le s'il n'existe pas :

```bash
mkdir -p ~/.config/ccache
```

```ini
# ~/.config/ccache/ccache.conf

# Taille maximale du cache (défaut : 5 Go)
max_size = 10G

# Activer la compression Zstandard (activé par défaut depuis ccache 4.0)
compression = true

# Niveau de compression (1-19, défaut : 0 = niveau par défaut de zstd, ~3)
compression_level = 1

# Répertoire du cache (défaut : ~/.cache/ccache)
# cache_dir = /chemin/personnalise/ccache
```

L'option `max_size` contrôle la taille maximale du cache sur disque. Quand cette limite est atteinte, ccache supprime automatiquement les entrées les plus anciennes (politique LRU — Least Recently Used). La valeur idéale dépend de la taille de vos projets : 5 Go convient pour un petit projet, 10-20 Go pour un projet de taille moyenne, 50-100 Go pour de gros projets.

Le niveau de compression `1` offre une compression rapide avec un bon ratio : c'est un bon compromis pour les builds quotidiens où la vitesse d'accès au cache est prioritaire.

### Vérifier que ccache fonctionne

Après installation, lancez une compilation puis vérifiez les statistiques :

```bash
# Première compilation : cache miss (normal)
ccache g++ -std=c++23 -O2 -c main.cpp -o main.o

# Deuxième compilation identique : cache hit
ccache g++ -std=c++23 -O2 -c main.cpp -o main.o

# Vérification
ccache --show-stats
```

La deuxième compilation devrait être quasi instantanée, et les statistiques devraient montrer au moins un cache hit.

### Emplacement du cache

Par défaut, ccache stocke son cache dans `~/.cache/ccache`. Vous pouvez vérifier l'emplacement et l'état du cache à tout moment :

```bash
# Afficher le répertoire du cache
ccache --get-config cache_dir

# Afficher la taille actuelle du cache
ccache --show-stats | head -5
```

Si vous travaillez sur un disque SSD (ce qui devrait être la norme en 2026), les accès au cache sont extrêmement rapides. Sur un disque dur mécanique, les bénéfices de ccache sont encore plus marqués en comparaison relative, car vous évitez de relancer le compilateur qui est lui-même intensif en I/O.

---

## 2.3.2 — Intégration avec CMake et Makefiles

L'invocation manuelle `ccache g++ ...` est utile pour comprendre le fonctionnement, mais en pratique, vous voulez que ccache soit intégré de manière transparente dans votre build system. Deux approches existent.

### Approche 1 : CMAKE_CXX_COMPILER_LAUNCHER (recommandée)

Depuis CMake 3.4, la variable `CMAKE_<LANG>_COMPILER_LAUNCHER` permet de spécifier un programme à utiliser comme lanceur devant le compilateur. C'est l'approche la plus propre et la plus portable :

```bash
cmake -B build -G Ninja \
    -DCMAKE_CXX_COMPILER_LAUNCHER=ccache \
    -DCMAKE_C_COMPILER_LAUNCHER=ccache
```

CMake inscrit alors `ccache` comme préfixe de chaque invocation du compilateur dans les fichiers de build générés. Le résultat dans les fichiers Ninja ressemble à :

```
RULE CXX_COMPILER
  command = ccache /usr/bin/g++-15 $FLAGS -c $in -o $out
```

Cette approche est idéale car elle est explicite, portable et ne modifie pas le compilateur lui-même. Elle est également compatible avec les CMake Presets :

```json
{
    "version": 6,
    "configurePresets": [
        {
            "name": "dev",
            "generator": "Ninja",
            "binaryDir": "${sourceDir}/build",
            "cacheVariables": {
                "CMAKE_CXX_COMPILER_LAUNCHER": "ccache",
                "CMAKE_C_COMPILER_LAUNCHER": "ccache",
                "CMAKE_CXX_STANDARD": "23"
            }
        }
    ]
}
```

### Approche 2 : Liens symboliques via le PATH

ccache peut aussi fonctionner en se faisant passer pour le compilateur lui-même. L'installation de ccache crée des liens symboliques dans `/usr/lib/ccache` :

```bash
ls /usr/lib/ccache
# gcc  g++  cc  c++  clang  clang++  ...
```

Ces liens symboliques portent le nom des compilateurs mais pointent vers le binaire ccache. Quand on les invoque, ccache détecte le nom sous lequel il a été appelé et transmet la compilation au vrai compilateur correspondant.

Pour activer ce mécanisme, il suffit d'ajouter ce répertoire en tête du `PATH` :

```bash
# Dans ~/.bashrc ou ~/.zshrc
export PATH="/usr/lib/ccache:$PATH"
```

Après cette modification, tout appel à `g++`, `clang++`, etc. passera automatiquement par ccache, sans aucune modification des Makefiles ou des commandes CMake. Vérification :

```bash
which g++
# Output :
# /usr/lib/ccache/g++

# Le vrai compilateur est toujours accessible directement :
/usr/bin/g++ --version
```

Cette approche est pratique pour un usage global et systématique, mais elle est moins explicite que l'approche CMake : un collègue qui lit votre `CMakeLists.txt` ne sait pas que ccache est utilisé, ce qui peut créer de la confusion en cas de débogage de problèmes de build.

### Recommandation

| Contexte | Approche recommandée |
|---|---|
| Développement local | PATH ou `CMAKE_CXX_COMPILER_LAUNCHER` |
| CI/CD | `CMAKE_CXX_COMPILER_LAUNCHER` (explicite, reproductible) |
| Projet d'équipe | `CMAKE_CXX_COMPILER_LAUNCHER` via CMake Presets |
| Makefile sans CMake | PATH (automatique) |

### Intégration avec un Makefile pur

Si vous utilisez un Makefile écrit à la main (sans CMake), l'approche PATH est la plus simple. Alternativement, vous pouvez préfixer le compilateur directement dans le Makefile :

```makefile
CXX = ccache g++  
CC  = ccache gcc  

CXXFLAGS = -std=c++23 -Wall -Wextra -O2

programme: main.o utils.o
	$(CXX) $(CXXFLAGS) -o programme main.o utils.o

main.o: main.cpp
	$(CXX) $(CXXFLAGS) -c main.cpp

# ...
```

### Compatibilité avec GCC et Clang

ccache supporte nativement GCC et Clang, sans configuration supplémentaire. Il détecte automatiquement le compilateur utilisé et adapte son comportement (par exemple, le traitement de `-march=native` diffère entre GCC et Clang, et ccache gère les deux cas).

---

## 2.3.3 — Statistiques et monitoring du cache

### Afficher les statistiques

La commande principale pour surveiller l'efficacité de ccache :

```bash
ccache --show-stats
```

Sortie typique après quelques builds :

```
Cacheable calls:   4826 / 5012 (96.29%)
  Hits:            3891 / 4826 (80.63%)
    Direct:        3654 / 3891 (93.91%)
    Preprocessed:   237 / 3891 ( 6.09%)
  Misses:           935 / 4826 (19.37%)
Uncacheable calls:  186 / 5012 ( 3.71%)  
Local storage:  
  Cache size (GB):  2.4 /  10.0 (24.00%)
  Hits:            3891 / 4826 (80.63%)
  Misses:           935 / 4826 (19.37%)
```

### Comprendre les métriques

**Cacheable calls** — le nombre total d'appels de compilation que ccache a pu traiter (par opposition aux appels « non cacheables », comme les compilations avec certaines options incompatibles).

**Hits** — le nombre de fois où ccache a pu retourner un résultat depuis le cache sans invoquer le compilateur. C'est le nombre qui compte le plus. Un **taux de hit élevé** (> 70-80 %) indique que ccache est efficace.

**Direct hits** vs **Preprocessed hits** — en mode direct, ccache évite même d'invoquer le préprocesseur. Les « direct hits » sont les plus rapides. Les « preprocessed hits » impliquent un passage par le préprocesseur avant de trouver la correspondance dans le cache — toujours bien plus rapide qu'une compilation complète, mais moins que les direct hits.

**Misses** — les compilations pour lesquelles aucune entrée de cache n'existait. Un miss est normal lors de la première compilation d'un fichier ou après un changement de code source. Un taux de miss élevé sur des rebuilds identiques indique un problème de configuration.

**Uncacheable calls** — les invocations que ccache ne peut pas mettre en cache (par exemple, les liens, les appels au préprocesseur seul, ou certaines options incompatibles). Ce nombre devrait rester faible.

**Cache size** — l'espace disque utilisé par le cache. Quand la limite `max_size` est atteinte, les entrées les plus anciennes sont automatiquement supprimées.

### Statistiques en format détaillé

Pour un affichage plus complet avec des compteurs additionnels :

```bash
ccache --show-stats --verbose
```

Cette commande affiche des compteurs supplémentaires : nombre de cleanups automatiques, fichiers compilés avec des erreurs (qui ne sont pas mis en cache), et détails sur la compression.

### Réinitialiser les statistiques

Pour repartir à zéro dans le comptage (utile pour mesurer l'impact de ccache sur un build spécifique) :

```bash
ccache --zero-stats
```

Les statistiques sont remises à zéro sans toucher au contenu du cache.

### Vider le cache

Pour supprimer tout le contenu du cache (les statistiques sont conservées) :

```bash
ccache --clear
```

Pour supprimer à la fois le cache et les statistiques :

```bash
ccache --clear  
ccache --zero-stats  
```

Le vidage complet est rarement nécessaire. Les cas typiques : changement majeur de version du compilateur, suspicion de cache corrompu, ou libération d'espace disque.

### Recompresser le cache

Si vous changez le niveau de compression dans votre configuration, vous pouvez recompresser les entrées existantes :

```bash
# Recompresser avec le niveau de compression actuel
ccache --recompress

# Recompresser avec un niveau spécifique
ccache --recompress 5
```

### Diagnostiquer un taux de hit faible

Si votre taux de hit est anormalement bas (< 50 % sur des rebuilds supposés identiques), voici les causes les plus fréquentes :

**Le chemin absolu change entre les builds.** Si vous compilez le même projet dans des répertoires différents (`/home/alice/project` vs `/home/bob/project`), les chemins absolus dans les options de compilation diffèrent et créent des hash différents. La solution est de configurer `base_dir` dans ccache.conf :

```ini
# ~/.config/ccache/ccache.conf
base_dir = /home/alice
```

Avec `base_dir`, ccache normalise les chemins relatifs à ce répertoire racine avant de calculer le hash.

**La version du compilateur change.** Même une mise à jour mineure du compilateur (par exemple de 15.1 à 15.2) invalide le cache, car le binaire du compilateur fait partie des entrées hashées. C'est un comportement voulu (la sortie peut changer entre versions), mais il faut en être conscient.

**Les macros `__DATE__` ou `__TIME__` sont utilisées.** Ces macros changent à chaque compilation, rendant le hash différent à chaque fois. ccache détecte leur présence et gère ce cas (en ignorant leur contribution au hash si `SOURCE_DATE_EPOCH` est défini), mais certaines bibliothèques tierces les utilisent dans leurs en-têtes.

**Les options de compilation varient.** Un changement de flag (ajout de `-DNDEBUG`, changement de `-O2` en `-O3`) produit un hash différent. C'est le comportement attendu — des options différentes peuvent produire un résultat différent.

---

## Bonnes pratiques

### Dimensionner le cache correctement

La taille du cache a un impact direct sur le taux de hit. Un cache trop petit entraîne des évictions fréquentes, réduisant l'efficacité. Quelques repères :

| Taille du projet | `max_size` recommandé |
|---|---|
| Petit (< 50 fichiers) | 2 – 5 Go |
| Moyen (50-500 fichiers) | 10 – 20 Go |
| Gros (> 500 fichiers) | 30 – 100 Go |
| Multi-projets | 50 – 100 Go |

Sur un SSD moderne, 20-50 Go dédiés au cache ccache est un investissement négligeable en espace pour un gain de temps considérable.

### Partager le cache entre builds Debug et Release

Un même fichier source compilé en Debug (`-O0 -g`) et en Release (`-O2`) produit des hash différents — et c'est normal, les résultats sont différents. ccache stocke les deux variantes séparément. Dimensionnez le cache en conséquence si vous alternez souvent entre les deux configurations.

### ccache et les en-têtes précompilés (PCH)

ccache supporte les en-têtes précompilés, mais avec certaines limitations selon le compilateur et le mode de fonctionnement. En pratique, ccache et PCH adressent le même problème (réduire le temps de compilation) avec des approches différentes. Si vous utilisez déjà ccache efficacement, le gain supplémentaire des PCH peut être marginal. Testez les deux configurations et mesurez.

### ccache en CI/CD

L'utilisation de ccache en CI/CD est traitée en détail à la section 38.3.1. L'idée centrale est de **persister le cache entre les jobs CI** (via un artefact de pipeline, un volume partagé, ou un cache distant HTTP/Redis). Sans persistance, chaque job repart d'un cache vide et ccache n'apporte aucun bénéfice. Avec persistance, les rebuilds CI incrémentaux passent de plusieurs minutes à quelques secondes.

---

## ccache vs sccache : quelle différence ?

**sccache** (Shared Compilation Cache) est une alternative à ccache, développée par Mozilla. Sa différence principale est le support natif du **cache distant** vers des backends cloud (Amazon S3, Google Cloud Storage, Azure Blob Storage) en plus du cache local. sccache supporte aussi le caching de compilations Rust, CUDA et HIP en plus de C/C++.

En mars 2026, la version stable est sccache 0.14. Il est installable via `cargo install sccache` ou via pip (`pip install sccache`).

| Critère | ccache | sccache |
|---|---|---|
| Cache local | ✅ Excellent | ✅ Bon |
| Cache distant (HTTP, Redis) | ✅ (depuis ccache 4.4) | ✅ (S3, GCS, Azure, Redis) |
| Langages | C, C++, assembler, CUDA | C, C++, Rust, CUDA, HIP |
| Maturité (C/C++) | Très mature | Bon |
| Performance locale | Très rapide (BLAKE3, inode cache) | Rapide |
| Distribution via apt | ✅ | ❌ (cargo/pip) |

Pour cette formation, nous utilisons **ccache** car il est plus mature pour C/C++, disponible directement dans les dépôts Ubuntu, et parfaitement suffisant pour un usage local et CI. sccache est une alternative intéressante si vous avez besoin de cacher des compilations Rust ou de partager un cache via un backend cloud sophistiqué. Nous aborderons sccache dans le contexte CI/CD à la section 38.3.2.

---

## Vérification de l'intégration complète

Ce script vérifie que ccache est correctement intégré dans votre workflow CMake + Ninja :

```bash
echo "=== Vérification ccache ==="  
ccache --version | head -1  
echo ""  

# Créer un projet test
TMPDIR=$(mktemp -d)  
cat > "$TMPDIR/CMakeLists.txt" << 'EOF'  
cmake_minimum_required(VERSION 3.20)  
project(CcacheTest LANGUAGES CXX)  
set(CMAKE_CXX_STANDARD 23)  
add_executable(test_ccache main.cpp)  
EOF  

cat > "$TMPDIR/main.cpp" << 'EOF'
#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
int main() {
    std::vector<std::string> v = {"ccache", "fonctionne", "correctement"};
    std::sort(v.begin(), v.end());
    for (const auto& s : v) std::cout << s << " ";
    std::cout << "\n";
}
EOF

# Remettre les stats à zéro
ccache --zero-stats > /dev/null

# Premier build (cache miss attendu)
cmake -S "$TMPDIR" -B "$TMPDIR/build" -G Ninja \
    -DCMAKE_CXX_COMPILER_LAUNCHER=ccache > /dev/null 2>&1
cmake --build "$TMPDIR/build" > /dev/null 2>&1

echo "--- Après premier build (miss attendu) ---"  
ccache --show-stats 2>&1 | grep -E "Hits|Misses"  

# Deuxième build (clean + rebuild = cache hit attendu)
cmake --build "$TMPDIR/build" --target clean > /dev/null 2>&1  
cmake --build "$TMPDIR/build" > /dev/null 2>&1  

echo ""  
echo "--- Après rebuild (hit attendu) ---"  
ccache --show-stats 2>&1 | grep -E "Hits|Misses"  

rm -rf "$TMPDIR"
```

Après exécution, le premier build devrait montrer un miss, et le rebuild après clean devrait montrer un hit — confirmant que ccache fonctionne correctement avec votre toolchain.

---

## Ce qu'il faut retenir

- **ccache** intercepte les appels au compilateur et retourne le résultat depuis un cache local quand les mêmes entrées (source, options, compilateur) ont déjà été compilées. Le gain est souvent spectaculaire (builds de quelques secondes au lieu de minutes).
- **L'installation** est triviale (`sudo apt install ccache`) et la configuration par défaut fonctionne immédiatement.
- **L'intégration avec CMake** se fait via `CMAKE_CXX_COMPILER_LAUNCHER=ccache` — c'est l'approche recommandée car elle est explicite et reproductible. L'alternative est de placer `/usr/lib/ccache` en tête du `PATH`.
- **Surveillez les statistiques** avec `ccache --show-stats`. Un taux de hit > 70-80 % sur les rebuilds courants indique que ccache est bien configuré. Un taux bas suggère un problème de chemins, de taille de cache ou de variation des options de compilation.
- **Dimensionnez le cache** généreusement (10-50 Go selon la taille du projet). L'espace disque est bon marché ; le temps de compilation ne l'est pas.
- En CI/CD, ccache n'est efficace que si le cache est **persisté entre les jobs** — nous verrons comment à la section 38.3.

---


⏭️ [Installation et configuration de ccache](/02-toolchain-ubuntu/03.1-installation-ccache.md)
