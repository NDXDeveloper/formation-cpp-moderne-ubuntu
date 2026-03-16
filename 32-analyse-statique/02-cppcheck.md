🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 32.2 — cppcheck : Détection d'erreurs

## Introduction

`cppcheck` est un analyseur statique open source pour C et C++, développé indépendamment de tout compilateur. C'est cette indépendance qui constitue à la fois sa force et sa particularité : là où clang-tidy est construit sur le parser de Clang et nécessite un `compile_commands.json` pour comprendre les options de compilation, cppcheck utilise son propre parser simplifié. Il peut analyser du code sans aucune information de build — il suffit de le pointer vers un répertoire de sources.

La philosophie de cppcheck privilégie la précision sur l'exhaustivité. L'outil vise un **taux de faux positifs proche de zéro** : lorsque cppcheck signale un problème, c'est presque toujours un vrai bug. Cette approche conservatrice le rend particulièrement adapté aux projets qui découvrent l'analyse statique — les développeurs ne sont pas submergés par des warnings discutables — et aux environnements CI où chaque diagnostic doit être actionnable.

cppcheck ne remplace pas clang-tidy ; il le complète. Les deux outils utilisent des techniques d'analyse différentes et détectent des catégories de bugs partiellement disjointes. Exécuter les deux sur le même projet augmente la couverture de détection.

---

## Installation sur Ubuntu

cppcheck est disponible dans les dépôts officiels :

```bash
sudo apt update  
sudo apt install cppcheck  
```

Vérification :

```bash
cppcheck --version
```

La version des dépôts Ubuntu est généralement en retard d'une ou deux versions mineures par rapport à la dernière release. Pour les projets utilisant du C++ très récent (C++23/26), il peut être préférable de compiler depuis les sources ou d'utiliser le PPA officiel :

```bash
# Compilation depuis les sources (dernière version)
git clone https://github.com/danmar/cppcheck.git  
cd cppcheck  
cmake -B build -DCMAKE_BUILD_TYPE=Release  
cmake --build build -j$(nproc)  
sudo cmake --install build  
```

---

## Première exécution

### Analyser un fichier

```bash
cppcheck mon_fichier.cpp
```

### Analyser un répertoire entier

```bash
cppcheck src/
```

cppcheck parcourt récursivement le répertoire et analyse tous les fichiers `.cpp`, `.c`, `.h` et `.hpp` qu'il trouve. Aucune configuration de build n'est nécessaire pour une première analyse — c'est l'avantage de l'indépendance vis-à-vis du compilateur.

### Exemple de sortie

Considérons un fichier avec plusieurs défauts :

```cpp
// defauts.cpp
#include <cstring>

void copier_donnees(char* dest, const char* src, int taille) {
    for (int i = 0; i <= taille; ++i) {  // Off-by-one : <= au lieu de <
        dest[i] = src[i];
    }
}

int* creer_tableau() {
    int tab[10];
    tab[0] = 42;
    return tab;  // Retourne un pointeur vers une variable locale
}

void traiter() {
    int* p = new int[100];
    if (p[0] > 0) {
        return;  // Fuite mémoire sur ce chemin
    }
    delete[] p;
}

void calcul() {
    int x;
    int y = x + 1;  // Utilisation d'une variable non initialisée
}
```

Exécution :

```bash
cppcheck --enable=all defauts.cpp
```

```
defauts.cpp:5:23: error: Buffer is accessed out of bounds: dest [CWE-788]
    for (int i = 0; i <= taille; ++i) {
                      ^
defauts.cpp:12:12: error: Pointer to local array variable returned. [returnDanglingLifetime]
    return tab;
           ^
defauts.cpp:17:5: error: Memory leak: p [memleak]
    }
    ^
defauts.cpp:23:13: error: Uninitialized variable: x [uninitvar]
    int y = x + 1;
            ^
```

Chaque diagnostic indique le fichier, la ligne, la colonne, le niveau de sévérité (`error`, `warning`, `style`, `performance`, `portability`, `information`), le message, et un identifiant entre crochets (`memleak`, `uninitvar`, etc.). L'identifiant CWE (Common Weakness Enumeration), lorsqu'il est présent, lie le diagnostic à une classification standardisée des vulnérabilités.

---

## Niveaux de sévérité

cppcheck classe ses diagnostics en six niveaux :

| Niveau | Signification | Activé par défaut |
|---|---|---|
| `error` | Bug avéré — comportement indéfini, crash probable | Oui |
| `warning` | Problème potentiel dépendant du contexte | Non |
| `style` | Convention de codage, lisibilité | Non |
| `performance` | Code fonctionnel mais inutilement lent | Non |
| `portability` | Code non portable entre plateformes ou compilateurs | Non |
| `information` | Messages informatifs, fichiers d'en-tête manquants | Non |

Par défaut, seul le niveau `error` est actif. Pour une analyse complète, l'option `--enable` active les niveaux supplémentaires :

```bash
# Activer tous les niveaux
cppcheck --enable=all src/

# Activer sélectivement
cppcheck --enable=warning,performance src/

# Activer tout sauf les messages informatifs
cppcheck --enable=all --disable=information src/
```

La recommandation est d'activer au minimum `warning` et `performance` en plus du niveau `error` par défaut. Le niveau `style` est utile mais plus bruyant — évaluez-le sur votre projet avant de l'intégrer en CI.

---

## Catégories d'erreurs détectées

### Fuites de ressources

cppcheck effectue une analyse de chemin pour traquer les allocations sans libération correspondante :

```cpp
void fuites() {
    FILE* f = fopen("data.txt", "r");
    if (!f) return;

    int* buf = new int[256];

    if (condition_erreur) {
        fclose(f);
        return;  // buf n'est pas libéré → memleak
    }

    delete[] buf;
    fclose(f);
}
```

cppcheck signale aussi les fuites de descripteurs de fichiers (`resourceLeak`) et les fuites liées à `fopen` sans `fclose`.

### Buffer overflows

L'analyse de bornes de cppcheck détecte les accès hors limites sur les tableaux statiques et certains cas dynamiques :

```cpp
void overflow() {
    int tab[10];
    tab[10] = 42;  // arrayIndexOutOfBounds

    char buf[8];
    strcpy(buf, "string trop longue");  // bufferAccessOutOfBounds
}
```

cppcheck connaît les sémantiques des fonctions standard (`strcpy`, `memcpy`, `strncpy`, `sprintf`) et vérifie que les tailles de buffers sont respectées.

### Variables non initialisées

```cpp
int calcul(bool condition) {
    int resultat;
    if (condition) {
        resultat = 42;
    }
    return resultat;  // uninitvar si condition est false
}
```

cppcheck suit les chemins d'exécution pour détecter les cas où une variable peut être lue avant d'être initialisée. L'analyse est sensible aux conditions — dans l'exemple, le diagnostic n'est émis que parce qu'il existe un chemin (condition false) où `resultat` n'est pas initialisé.

### Null pointer dereference

```cpp
void traiter(int* ptr) {
    if (ptr == nullptr) {
        log("pointeur nul");
    }
    *ptr = 42;  // nullPointerRedundantCheck : déréférencé après un test de nullité
}
```

Le raisonnement de cppcheck est : si le code teste `ptr == nullptr`, c'est que le développeur considère la nullité comme possible. Le déréférencement ultérieur sans vérification est donc suspect.

### Code mort et conditions redondantes

```cpp
void redondances(unsigned int x) {
    if (x >= 0) {  // Condition toujours vraie (unsigned >= 0)
        traiter(x);
    }

    if (x < 0) {   // Code mort (unsigned jamais négatif)
        erreur();
    }
}
```

cppcheck détecte les conditions qui sont toujours vraies ou toujours fausses en se basant sur les types et les valeurs connues. Ces diagnostics signalent souvent une erreur de logique plutôt qu'un simple problème de style.

### Mismatch d'allocation

```cpp
void mismatch() {
    int* p = new int[10];
    delete p;      // mismatchAllocDealloc : new[] avec delete (sans [])

    char* c = (char*)malloc(100);
    delete c;      // mismatchAllocDealloc : malloc avec delete
}
```

---

## Options essentielles

### Spécifier le standard C++

```bash
cppcheck --std=c++23 src/
```

Cette option informe cppcheck du standard utilisé, ce qui affecte l'interprétation de certaines constructions syntaxiques et la disponibilité de certaines fonctionnalités.

### Définir des macros et des chemins d'inclusion

cppcheck ayant son propre parser, il ne lit pas les `compile_commands.json` nativement (mais peut les exploiter via l'option `--project`). Pour une analyse correcte, il a besoin de connaître les macros et les chemins d'inclusion :

```bash
cppcheck --std=c++23 \
    -I include/ \
    -I /usr/local/include/ \
    -D LINUX \
    -D NDEBUG \
    src/
```

### Utiliser un compile_commands.json

Pour les projets utilisant CMake, cppcheck peut exploiter la base de données de compilation :

```bash
cppcheck --project=build/compile_commands.json
```

Cette méthode est recommandée lorsqu'elle est disponible : elle évite de maintenir manuellement les chemins d'inclusion et les macros, et garantit que cppcheck analyse le code avec les mêmes paramètres que le compilateur.

### Contrôler le nombre de threads

cppcheck supporte l'analyse parallèle :

```bash
cppcheck -j $(nproc) --enable=all src/
```

L'option `-j` spécifie le nombre de threads d'analyse. Sur un projet volumineux, la parallélisation réduit significativement le temps d'analyse.

> ⚠️ **Note** : avec `-j > 1`, l'analyse inter-fichiers (*whole program analysis*) est désactivée. Pour les diagnostics qui nécessitent l'analyse de plusieurs fichiers (certaines fuites de ressources, détection d'incohérences d'API), exécutez une passe supplémentaire en monothread : `cppcheck --enable=all src/`.

### Supprimer des diagnostics

Pour les faux positifs confirmés, cppcheck offre plusieurs mécanismes de suppression :

```cpp
// Suppression inline (dans le code)
int* p = new int[10];
// cppcheck-suppress memleak
return p;  // La mémoire est libérée par l'appelant (contrat documenté)
```

Le commentaire `cppcheck-suppress` doit apparaître sur la ligne précédant le diagnostic ou sur la même ligne. Il accepte un identifiant de diagnostic spécifique.

Pour les suppressions à l'échelle du projet, un fichier de suppressions :

```bash
# suppressions.txt
memleak:src/legacy/old_allocator.cpp:42  
uninitvar:src/generated/*.cpp  
*:third_party/*
```

```bash
cppcheck --suppressions-list=suppressions.txt --enable=all src/
```

Le format est `diagnostic:fichier:ligne` ou `diagnostic:pattern_glob`. La dernière ligne (`*:third_party/*`) supprime tous les diagnostics dans le code tiers.

### Format de sortie

cppcheck supporte plusieurs formats de sortie pour l'intégration avec d'autres outils :

```bash
# Format XML (pour les outils CI comme Jenkins, SonarQube)
cppcheck --enable=all --xml src/ 2> rapport.xml

# Format compatible GCC (fichier:ligne:colonne: message)
cppcheck --enable=all --template=gcc src/

# Template personnalisé
cppcheck --enable=all --template='{file}:{line}: {severity} ({id}): {message}' src/
```

Le format XML est particulièrement utile pour l'intégration CI : des plugins Jenkins, GitLab et SonarQube consomment directement ce format pour afficher les résultats dans leurs tableaux de bord.

---

## Intégration avec CMake

CMake propose une intégration native de cppcheck, similaire à celle de clang-tidy :

```cmake
option(ENABLE_CPPCHECK "Enable cppcheck analysis" OFF)

if(ENABLE_CPPCHECK)
    find_program(CPPCHECK_EXE NAMES cppcheck)
    if(CPPCHECK_EXE)
        set(CMAKE_CXX_CPPCHECK
            "${CPPCHECK_EXE}"
            "--enable=warning,performance"
            "--suppress=missingIncludeSystem"
            "--quiet"
        )
        message(STATUS "cppcheck enabled: ${CPPCHECK_EXE}")
    else()
        message(WARNING "cppcheck requested but not found")
    endif()
endif()
```

Activation :

```bash
cmake -B build -DENABLE_CPPCHECK=ON  
cmake --build build  
```

Avec cette configuration, cppcheck s'exécute automatiquement sur chaque fichier source lors de la compilation. L'option `--quiet` supprime les messages de progression pour ne laisser que les diagnostics.

L'option `--suppress=missingIncludeSystem` est presque toujours nécessaire : cppcheck signale chaque header système qu'il ne peut pas résoudre avec son propre parser, ce qui génère un bruit considérable sur les projets utilisant la STL.

---

## cppcheck vs clang-tidy : complémentarité

Les deux outils ont des approches et des forces différentes. Les exécuter en tandem maximise la couverture.

| Critère | cppcheck | clang-tidy |
|---|---|---|
| **Parser** | Propre (simplifié) | Parser Clang complet |
| **compile_commands.json** | Optionnel | Quasi indispensable |
| **Taux de faux positifs** | Très faible | Faible à modéré (selon les checks) |
| **Nombre de checks** | ~200 | ~400+ |
| **Fix automatique** | Non | Oui (`--fix`) |
| **Analyse inter-fichiers** | Oui (monothread) | Limitée |
| **Support C++23/26** | Partiel | Complet (parser Clang) |
| **Vitesse** | Rapide | Modéré à lent |
| **Diagnostics uniques** | Fuites de ressources, bornes de tableaux statiques, mismatch alloc | Modernisation, Core Guidelines, convention de nommage, analyse de chemin LLVM |

### Ce que cppcheck détecte et pas clang-tidy

- Certains patterns de **buffer overflow** sur les tableaux statiques et les fonctions C (`strcpy`, `sprintf`) que clang-tidy ne couvre pas.
- L'**analyse inter-fichiers** (whole program analysis) détecte des incohérences entre unités de compilation : fonction déclarée mais jamais définie, paramètre toujours utilisé avec la même valeur constante.
- Certaines **fuites de ressources** C (file handles, sockets) via l'analyse de chemin spécialisée.

### Ce que clang-tidy détecte et pas cppcheck

- La **modernisation** du code (migration C++11 → C++23).
- Les violations des **C++ Core Guidelines** et du **CERT C++ Coding Standard**.
- L'**analyse de chemin du Clang Static Analyzer** (interprocédurale, sensible aux conditions).
- Les problèmes liés aux **templates instanciés** (cppcheck a un support limité des templates complexes).
- Les corrections automatiques (`--fix`).

### Stratégie d'utilisation combinée

```yaml
# .gitlab-ci.yml (extrait)
analyse_statique:
  stage: quality
  script:
    # clang-tidy via CMake
    - cmake -B build -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
    - run-clang-tidy -p build/ -quiet

    # cppcheck en complément
    - cppcheck --project=build/compile_commands.json
        --enable=warning,performance
        --error-exitcode=1
        --suppress=missingIncludeSystem
        --quiet
```

L'option `--error-exitcode=1` de cppcheck fait échouer le pipeline CI lorsqu'un diagnostic de niveau `error` est détecté. Les niveaux `warning` et `performance` sont affichés mais ne bloquent pas le build — ils servent d'information pour les développeurs.

---

## Limites

### Parser simplifié

Le parser propre de cppcheck ne comprend pas toutes les subtilités du C++ moderne. Les constructions avancées de templates, les concepts (C++20), les expressions `requires`, et certaines formes de SFINAE peuvent ne pas être analysées correctement, produisant des messages `information: failed to parse...` ou des diagnostics manqués. Pour le C++ moderne avancé, clang-tidy est plus fiable grâce à son parser Clang complet.

### Pas de correction automatique

Contrairement à clang-tidy, cppcheck ne propose pas de mécanisme `--fix`. Il signale les problèmes mais ne les corrige pas. La correction reste manuelle.

### Analyse inter-fichiers lente

L'analyse *whole program* (sans `-j`) peut être longue sur les gros projets. Le compromis habituel est d'exécuter l'analyse parallélisée (rapide, mais sans analyse inter-fichiers) en CI à chaque commit, et l'analyse complète en nightly.

### Support inégal des librairies

cppcheck connaît les sémantiques de la librairie standard C et d'une partie de la STL C++, mais sa connaissance des librairies tierces (Boost, Qt, Abseil) est limitée. Il peut produire des faux positifs sur du code utilisant des patterns spécifiques à ces librairies. Les fichiers de suppression permettent de gérer ces cas.

⏭️ [clang-format 19 : Formatage automatique](/32-analyse-statique/03-clang-format.md)
