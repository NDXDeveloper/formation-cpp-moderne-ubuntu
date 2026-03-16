🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 32.1 — clang-tidy : Analyse statique moderne

## Introduction

`clang-tidy` est l'outil d'analyse statique et de linting de l'écosystème LLVM/Clang. Contrairement aux analyseurs statiques traditionnels qui travaillent sur une représentation simplifiée du code, clang-tidy utilise le **parser complet de Clang** — le même qui compile votre code. Il opère directement sur l'arbre syntaxique abstrait (AST) du programme, ce qui lui donne une compréhension sémantique profonde du code : résolution des templates, expansion des macros, analyse des types, suivi du flot de données.

Cette architecture a deux conséquences majeures. D'abord, clang-tidy comprend le C++ avec la même fidélité que le compilateur : les subtilités du langage (SFINAE, ADL, template metaprogramming, concepts) ne le trompent pas. Ensuite, clang-tidy peut non seulement *détecter* les problèmes, mais aussi les *corriger automatiquement* grâce à des transformations de l'AST qui préservent la sémantique du programme. L'option `--fix` applique les corrections directement dans le code source, transformant clang-tidy en un outil de refactoring automatisé.

En 2026, clang-tidy propose plus de 400 checks organisés en catégories thématiques. Le spectre couvert est remarquablement large : des bugs classiques (déréférencement de pointeur nul, integer overflow) aux violations des C++ Core Guidelines, en passant par la modernisation du code (migration vers C++17/20/23), l'optimisation de performance, et les conventions de codage spécifiques à des projets (Google, LLVM, Android).

---

## Installation sur Ubuntu

clang-tidy est distribué avec les outils LLVM. Sur Ubuntu :

```bash
sudo apt update  
sudo apt install clang-tidy  
```

Cette commande installe la version de clang-tidy correspondant à la version par défaut de LLVM dans les dépôts. Pour installer une version spécifique (par exemple la version 20, correspondant à Clang 20) :

```bash
sudo apt install clang-tidy-20
```

Vérification :

```bash
clang-tidy --version
```

```
LLVM (http://llvm.org/):
  LLVM version 20.0.0
  ...
```

Si plusieurs versions sont installées, `update-alternatives` permet de sélectionner la version par défaut (section 2.1.1) :

```bash
sudo update-alternatives --install /usr/bin/clang-tidy clang-tidy /usr/bin/clang-tidy-20 100
```

### Relation avec clangd

`clangd`, le serveur LSP de LLVM utilisé par VS Code, Neovim et d'autres éditeurs, **intègre nativement clang-tidy**. Lorsqu'un fichier `.clang-tidy` est présent à la racine du projet, clangd exécute automatiquement les checks configurés et affiche les diagnostics en temps réel dans l'éditeur — soulignements, tooltips, suggestions de correction. Cette intégration signifie que pour la majorité des développeurs, l'expérience clang-tidy au quotidien passe par l'IDE, sans exécution manuelle en ligne de commande.

La ligne de commande reste essentielle pour la CI, les pre-commit hooks, et les analyses sur l'ensemble du projet (là où clangd analyse fichier par fichier).

---

## Première exécution

### Sur un fichier unique

```bash
clang-tidy mon_fichier.cpp -- -std=c++23
```

Le `--` sépare les options de clang-tidy des options passées au compilateur. Tout ce qui suit `--` est interprété comme des flags de compilation (standard, includes, defines). clang-tidy utilise ces flags pour parser le fichier exactement comme le ferait le compilateur.

### Avec un compile_commands.json

Pour un projet multi-fichiers, clang-tidy a besoin de connaître les options de compilation de chaque fichier. Le moyen standard est le fichier `compile_commands.json`, une base de données de compilation que CMake génère automatiquement :

```bash
# Générer compile_commands.json avec CMake
cmake -B build -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
```

Le fichier `compile_commands.json` est créé dans le répertoire `build/`. Il contient, pour chaque fichier source, la commande de compilation exacte utilisée par CMake. clang-tidy le recherche automatiquement dans le répertoire courant ou celui spécifié par `-p` :

```bash
clang-tidy -p build/ src/parser.cpp
```

Pour analyser l'ensemble du projet, l'outil `run-clang-tidy` (distribué avec LLVM) exécute clang-tidy en parallèle sur tous les fichiers listés dans `compile_commands.json` :

```bash
run-clang-tidy -p build/
```

`run-clang-tidy` utilise tous les cœurs disponibles par défaut, ce qui accélère considérablement l'analyse sur les projets volumineux.

### Exemple de sortie

Considérons un fichier avec plusieurs problèmes :

```cpp
// exemple.cpp
#include <vector>
#include <string>

class Config {
    std::string host;
    int port;
public:
    Config(std::string h, int p) : host(h), port(p) {}

    std::string getHost() { return host; }

    int* getPortPtr() {
        int local_port = port;
        return &local_port;  // Retourne un pointeur vers une variable locale
    }
};

void traiter(std::vector<int> donnees) {
    for (int i = 0; i < donnees.size(); ++i) {
        if (donnees[i] == 0) {
            int resultat = 100 / donnees[i];  // Division par zéro
        }
    }
}
```

L'exécution de clang-tidy avec les checks `bugprone-*,performance-*,readability-*,clang-analyzer-*` produit des diagnostics similaires à ceux-ci (le détail exact varie selon la version de LLVM) :

```
exemple.cpp:8:26: warning: the parameter 'h' is copied for each invocation
    but only used as a const reference; consider making it a const reference
    [performance-unnecessary-value-param]
    Config(std::string h, int p) : host(h), port(p) {}
                         ^
                         const std::string&

exemple.cpp:10:5: warning: method 'getHost' can be made const
    [readability-make-member-function-const]
    std::string getHost() { return host; }
    ^

exemple.cpp:14:16: warning: address of stack memory associated with local
    variable 'local_port' returned [clang-analyzer-core.StackAddressEscape]
        return &local_port;
               ^

exemple.cpp:20:23: warning: comparison of integers of different signs:
    'int' and 'size_t' [bugprone-narrowing-conversions]
    for (int i = 0; i < donnees.size(); ++i) {
                    ~ ^ ~~~~~~~~~~~~~~

exemple.cpp:19:33: warning: the parameter 'donnees' is copied for each
    invocation; consider making it a const reference
    [performance-unnecessary-value-param]
void traiter(std::vector<int> donnees) {
                              ^
                              const std::vector<int>&
```

Chaque diagnostic suit le même format :

```
fichier:ligne:colonne: niveau: message [nom-du-check]
```

Le **nom du check** entre crochets (par exemple `performance-unnecessary-value-param`) identifie la règle déclenchée. Ce nom est la clé pour configurer les checks — l'activer, le désactiver, ou le paramétrer dans le fichier `.clang-tidy`.

Le **niveau** est `warning` pour la plupart des checks, ou `error` pour les checks promus via la configuration. clang-tidy n'émet pas de `warning` du compilateur — ses diagnostics sont distincts de ceux de `g++` ou `clang++`.

---

## Catégories de checks

Les 400+ checks de clang-tidy sont organisés en préfixes thématiques. Comprendre ces catégories est essentiel pour configurer un jeu de checks adapté à votre projet.

### bugprone-*

Checks ciblant les patterns de code qui sont des sources fréquentes de bugs. Ces checks ont un taux de vrais positifs élevé — lorsqu'ils signalent un problème, c'est presque toujours un bug réel ou un code ambigu qui mérite attention.

Exemples notables :

| Check | Ce qu'il détecte |
|---|---|
| `bugprone-use-after-move` | Utilisation d'un objet après `std::move` |
| `bugprone-narrowing-conversions` | Conversions implicites avec perte de données |
| `bugprone-dangling-handle` | `string_view` ou span pointant vers un objet détruit |
| `bugprone-infinite-loop` | Boucles dont la condition ne peut jamais devenir fausse |
| `bugprone-integer-division` | Division entière là où un résultat flottant est attendu |
| `bugprone-swapped-arguments` | Arguments de fonction dans le mauvais ordre |

### performance-*

Checks identifiant les patterns qui sont fonctionnellement corrects mais inutilement coûteux en performance.

| Check | Ce qu'il détecte |
|---|---|
| `performance-unnecessary-value-param` | Paramètre copié inutilement (devrait être `const&`) |
| `performance-unnecessary-copy-initialization` | Variable locale copiée inutilement |
| `performance-move-const-arg` | `std::move` sur un argument constant (inutile) |
| `performance-for-range-copy` | Copie dans un range-based for (devrait être `const auto&`) |
| `performance-inefficient-string-concatenation` | Concaténation de strings dans une boucle |

### modernize-*

Checks proposant la migration vers les idiomes du C++ moderne (C++11 → C++23). Ces checks sont accompagnés de corrections automatiques qui effectuent la transformation.

| Check | Transformation proposée |
|---|---|
| `modernize-use-nullptr` | `NULL` / `0` → `nullptr` |
| `modernize-use-auto` | Déclarations redondantes → `auto` |
| `modernize-use-override` | Méthodes virtuelles → `override` |
| `modernize-use-emplace` | `push_back(T(...))` → `emplace_back(...)` |
| `modernize-loop-convert` | Boucle par index → range-based for |
| `modernize-use-using` | `typedef` → `using` |
| `modernize-use-starts-ends-with` | Comparaisons manuelles → `starts_with`/`ends_with` (C++20) |
| `modernize-use-std-print` | `printf`/`cout` → `std::print` (C++23) |

### cppcoreguidelines-*

Checks implémentant les [C++ Core Guidelines](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines), le document de bonnes pratiques maintenu par Bjarne Stroustrup et Herb Sutter. Ces checks sont parmi les plus stricts et les plus opinionated — ils reflètent une vision spécifique du « bon C++ ».

| Check | Guideline |
|---|---|
| `cppcoreguidelines-owning-memory` | Utiliser les smart pointers pour la propriété |
| `cppcoreguidelines-no-malloc` | Pas de `malloc`/`free` en C++ |
| `cppcoreguidelines-avoid-goto` | Pas de `goto` |
| `cppcoreguidelines-slicing` | Avertir sur le slicing d'objets |
| `cppcoreguidelines-special-member-functions` | Respecter la Rule of Five |
| `cppcoreguidelines-pro-type-reinterpret-cast` | Éviter `reinterpret_cast` |

### readability-*

Checks centrés sur la lisibilité et la maintenabilité du code. Moins critiques que `bugprone-*` en termes de bugs, mais importants pour la qualité à long terme d'une base de code.

| Check | Ce qu'il améliore |
|---|---|
| `readability-identifier-naming` | Convention de nommage (camelCase, snake_case) |
| `readability-braces-around-statements` | Accolades obligatoires sur `if`/`for`/`while` |
| `readability-magic-numbers` | Constantes magiques non nommées |
| `readability-function-cognitive-complexity` | Fonctions trop complexes |
| `readability-redundant-string-cstr` | Appels `.c_str()` inutiles |

### clang-analyzer-*

Checks issus du **Clang Static Analyzer**, un moteur d'analyse statique plus profond intégré à clang-tidy. Ces checks effectuent une analyse interprocédurale et un suivi de chemin (*path-sensitive analysis*) pour détecter des bugs que les checks basés sur des patterns syntaxiques ne peuvent pas trouver.

| Check | Ce qu'il détecte |
|---|---|
| `clang-analyzer-core.NullDereference` | Déréférencement de pointeur potentiellement nul |
| `clang-analyzer-core.StackAddressEscape` | Retour de pointeur vers une variable locale |
| `clang-analyzer-unix.Malloc` | Fuites mémoire (malloc sans free) |
| `clang-analyzer-deadcode.DeadStores` | Affectations à des variables jamais lues |
| `clang-analyzer-cplusplus.NewDeleteLeaks` | Fuites mémoire (new sans delete) |

Les checks `clang-analyzer-*` sont plus lents que les autres catégories car ils explorent les chemins d'exécution possibles. Ils sont aussi ceux qui ont le meilleur taux de détection pour les bugs graves (déréférencements nuls, fuites, use-after-free).

### Autres catégories

- **`cert-*`** : implémentation des règles du CERT C++ Coding Standard (orienté sécurité).
- **`hicpp-*`** : implémentation du High Integrity C++ Coding Standard (logiciels critiques).
- **`misc-*`** : checks divers qui ne rentrent pas dans les catégories ci-dessus.
- **`portability-*`** : détection de code non portable entre plateformes.
- **`concurrency-*`** : détection de patterns problématiques en programmation concurrente.

---

## Correction automatique avec `--fix`

L'un des atouts majeurs de clang-tidy est sa capacité à corriger automatiquement une partie des problèmes détectés. Les checks qui proposent une correction sont signalés par le label `[fixable]` dans la documentation.

```bash
# Appliquer les corrections automatiquement
clang-tidy --fix -p build/ src/parser.cpp

# Prévisualiser les corrections sans les appliquer
clang-tidy --fix-notes -p build/ src/parser.cpp
```

Exemple : le check `modernize-use-nullptr` transforme automatiquement :

```cpp
// Avant
Widget* w = NULL;  
if (ptr == 0) { ... }  

// Après --fix
Widget* w = nullptr;  
if (ptr == nullptr) { ... }  
```

Et `performance-unnecessary-value-param` transforme :

```cpp
// Avant
void traiter(std::vector<int> donnees) { ... }

// Après --fix
void traiter(const std::vector<int>& donnees) { ... }
```

> ⚠️ **Précaution** : les corrections automatiques ne sont pas toujours sémantiquement correctes dans tous les contextes. Certaines transformations peuvent modifier le comportement du programme de manière subtile (par exemple, changer un passage par valeur en passage par référence dans une fonction qui modifie son paramètre intentionnellement). Relisez toujours le diff après un `--fix`, et exécutez les tests avant de committer. Le workflow recommandé est :

```bash
# 1. Appliquer les corrections
clang-tidy --fix -p build/ src/parser.cpp

# 2. Vérifier le diff
git diff src/parser.cpp

# 3. Exécuter les tests
cmake --build build --target tests && ctest --test-dir build

# 4. Committer si tout passe
git add src/parser.cpp && git commit -m "fix: apply clang-tidy suggestions"
```

Pour les projets existants contenant de nombreux avertissements, la correction en masse est une stratégie efficace de mise à niveau. L'outil `run-clang-tidy` supporte également `--fix` pour corriger tout le projet en parallèle :

```bash
run-clang-tidy -p build/ -fix -checks='modernize-use-nullptr'
```

Ce type de correction ciblée — un seul check à la fois, sur tout le projet — produit des commits atomiques et faciles à reviewer.

---

## Intégration avec CMake

CMake propose une intégration native de clang-tidy via la propriété `CMAKE_CXX_CLANG_TIDY`. Lorsqu'elle est définie, CMake exécute automatiquement clang-tidy sur chaque fichier source au moment de la compilation :

```cmake
# Dans CMakeLists.txt
set(CMAKE_CXX_CLANG_TIDY "clang-tidy;-p;${CMAKE_BINARY_DIR}")
```

Ou, plus flexible, en option activable :

```cmake
option(ENABLE_CLANG_TIDY "Enable clang-tidy analysis" OFF)

if(ENABLE_CLANG_TIDY)
    find_program(CLANG_TIDY_EXE NAMES clang-tidy clang-tidy-20)
    if(CLANG_TIDY_EXE)
        set(CMAKE_CXX_CLANG_TIDY "${CLANG_TIDY_EXE}")
        message(STATUS "clang-tidy enabled: ${CLANG_TIDY_EXE}")
    else()
        message(WARNING "clang-tidy requested but not found")
    endif()
endif()
```

Activation à la configuration :

```bash
cmake -B build -DENABLE_CLANG_TIDY=ON  
cmake --build build  
```

Chaque fichier compilé est analysé par clang-tidy, et les diagnostics apparaissent dans la sortie de compilation, mélangés avec les warnings du compilateur. Les warnings de clang-tidy sont identifiables par leur nom entre crochets.

Cette intégration a un surcoût en temps de compilation (typiquement +30 à 100% selon le nombre de checks actifs). Elle est généralement activée en CI plutôt que pendant le développement quotidien, où l'intégration via clangd dans l'IDE est préférée.

---

## Lister et explorer les checks disponibles

Pour afficher la liste complète des checks disponibles dans votre version de clang-tidy :

```bash
clang-tidy --list-checks -checks='*'
```

Pour filtrer par catégorie :

```bash
# Tous les checks bugprone
clang-tidy --list-checks -checks='bugprone-*'

# Tous les checks de performance et de modernisation
clang-tidy --list-checks -checks='performance-*,modernize-*'
```

Pour obtenir la documentation d'un check spécifique, la référence est la [documentation LLVM en ligne](https://clang.llvm.org/extra/clang-tidy/checks/list.html), qui détaille pour chaque check : ce qu'il détecte, des exemples de code, les options de configuration, et si une correction automatique est disponible.

---

## Limites

### Faux positifs

Malgré la qualité de l'analyse, clang-tidy produit des faux positifs. Certains checks (notamment `cppcoreguidelines-*` et `hicpp-*`) sont très stricts et signalent du code qui est techniquement correct mais ne respecte pas la lettre de la guideline. Le fichier `.clang-tidy` (section 32.1.1) permet de désactiver les checks trop bruyants pour un projet donné.

Pour supprimer un diagnostic sur une ligne spécifique :

```cpp
int* ptr = reinterpret_cast<int*>(buffer);  // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
```

Le commentaire `// NOLINT(check-name)` désactive un check spécifique sur la ligne. `// NOLINTNEXTLINE(check-name)` désactive le check sur la ligne suivante. Ces annotations doivent être utilisées avec parcimonie et accompagnées d'un commentaire expliquant pourquoi le code est correct malgré l'avertissement.

### Temps d'analyse

Les checks `clang-analyzer-*` effectuent une analyse de chemin qui peut être coûteuse sur les fonctions complexes. Sur les très gros projets, l'analyse complète peut prendre plusieurs minutes. En développement quotidien, l'intégration via clangd amortit ce coût en analysant fichier par fichier de manière incrémentale.

### Dépendance à la base de compilation

clang-tidy nécessite un `compile_commands.json` ou des flags de compilation explicites pour analyser correctement le code. Sans cette information, il ne peut pas résoudre les includes, les macros conditionnelles ou les options de standard. C'est une source fréquente de frustration pour les débutants : clang-tidy qui « ne trouve pas les headers » est presque toujours un problème de `compile_commands.json` absent ou mal configuré.

---

## Plan des sous-sections

Les sous-sections qui suivent détaillent la configuration et l'utilisation avancée de clang-tidy :

- **32.1.1 — Configuration `.clang-tidy`** : création et structure du fichier de configuration, sélection des checks par pattern, options par check, héritage de configuration entre répertoires.

- **32.1.2 — Checks recommandés** : sélections de checks éprouvées pour différents contextes (nouveau projet C++23, projet legacy en migration, projet orienté sécurité), avec justification de chaque choix.

⏭️ [Configuration .clang-tidy](/32-analyse-statique/01.1-configuration.md)
