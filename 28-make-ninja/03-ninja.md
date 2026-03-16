🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 28.3 Ninja : Build system ultra-rapide ⭐

> **Objectif** : Comprendre Ninja en tant que build system — sa philosophie de conception, son architecture interne, et comment il se distingue fondamentalement de Make — avant de détailler dans les sous-sections pourquoi il est plus rapide (28.3.1) et comment lire ses fichiers de build (28.3.2).

---

## Rappel et positionnement

La section 26.5 a couvert Ninja sous l'angle **utilisateur** : comment l'utiliser avec CMake, pourquoi le préférer à Make, les commandes quotidiennes. Cette section change de perspective. Nous examinons Ninja en tant qu'**outil logiciel** : ses principes de conception, ce qui le rend fondamentalement différent de Make, et comment fonctionne le pipeline entre CMake et l'exécution des compilations.

---

## Genèse et philosophie

Ninja a été créé en 2012 par Evan Martin, un ingénieur Google qui travaillait sur le build de Chromium. À l'époque, Chromium utilisait GYP (un méta-build system, ancêtre de GN) qui générait des Makefiles pour Linux. Le problème était que Make passait **plusieurs secondes** rien qu'à charger et analyser les Makefiles générés — avant même de lancer la moindre compilation. Sur un projet de la taille de Chromium (plus de 30 000 fichiers sources), ce temps de démarrage rendait le cycle de développement pénible.

La solution d'Evan Martin a été radicale : concevoir un build system entièrement **optimisé pour être généré**, pas pour être écrit à la main. Cette décision fondatrice a des conséquences profondes sur toute l'architecture de Ninja.

### Les principes directeurs

**Faire une seule chose, la faire vite.** Ninja ne fait qu'exécuter des commandes pour produire des fichiers, dans le bon ordre, en parallèle. Il ne détecte pas les compilateurs, ne gère pas les dépendances de bibliothèques, ne supporte pas les conditions ni les boucles. Toute cette logique appartient à l'outil de niveau supérieur (CMake, Meson, GN).

**Optimiser le cas commun.** Le scénario le plus fréquent en développement est le *rebuild incrémental* : vous modifiez un fichier, lancez le build, et seuls quelques fichiers doivent être recompilés. Ninja est obsessionnellement optimisé pour ce cas — la vérification de ce qui doit être reconstruit doit prendre le moins de temps possible.

**Simplicité du format.** Le fichier `build.ninja` est un format texte plat, sans imbrication, sans conditions, sans fonctions. Chaque instruction tient sur une ou deux lignes. Le parsing est trivial — une simple machine à états, pas un interpréteur de langage comme l'est Make.

**Pas de backward compatibility contraignante.** Contrairement à Make (qui traîne 50 ans de compatibilité), Ninja est un format jeune sans dette technique. Il peut casser la compatibilité entre versions quand c'est justifié, bien qu'il le fasse rarement.

---

## Architecture : ce que Ninja fait (et ne fait pas)

Le périmètre de Ninja est délibérément étroit. Voici ce qu'il gère et ce qu'il délègue :

| Responsabilité | Ninja | Make | CMake/Meson |
|---------------|:-----:|:----:|:-----------:|
| Parsing du graphe de build | ✅ | ✅ | — |
| Détection de péremption (quoi recompiler) | ✅ | ✅ | — |
| Parallélisation des tâches | ✅ | ✅ | — |
| Variables et substitution simple | ✅ | ✅ | ✅ |
| Conditions et logique | ❌ | ✅ | ✅ |
| Fonctions de manipulation de texte | ❌ | ✅ | ✅ |
| Détection du compilateur | ❌ | ❌ | ✅ |
| Gestion des bibliothèques et dépendances | ❌ | ❌ | ✅ |
| Règles de pattern / implicites | ❌ | ✅ | ✅ |
| Fichiers de dépendances compilateur (`.d`) | ✅ | ✅ (via `-include`) | ✅ (automatique) |

L'absence de logique conditionnelle, de fonctions et de règles implicites est **intentionnelle**. Ces fonctionnalités rendent Make puissant comme build system autonome, mais elles complexifient son parsing et ralentissent son démarrage. Ninja échange l'expressivité contre la vitesse.

---

## Le pipeline CMake → Ninja → Compilation

Pour situer Ninja dans le workflow complet :

```
CMakeLists.txt
      │
      │  cmake -B build -G Ninja
      ▼
build/build.ninja          ← Un seul fichier plat  
build/rules.ninja          ← Définitions de règles (compilateur, flags)  
build/.ninja_deps           ← Cache des dépendances de headers  
build/.ninja_log            ← Historique des commandes exécutées  
      │
      │  ninja -C build  (ou cmake --build build)
      ▼
Ninja charge build.ninja (quelques ms)
  → Vérifie les timestamps et le hash des commandes
  → Détermine les tâches à exécuter
  → Lance les compilations en parallèle
  → Met à jour .ninja_deps et .ninja_log
      │
      ▼
Binaires finaux
```

Deux fichiers méritent une attention particulière :

**`.ninja_deps`** : un fichier binaire qui stocke les dépendances de headers de chaque fichier source (l'équivalent des fichiers `.d` de Make, mais dans un format compact et pré-parsé). Ninja lit ce fichier au démarrage pour connaître les dépendances sans reparsing.

**`.ninja_log`** : un journal des commandes précédemment exécutées, avec leurs timestamps de début/fin. Ninja utilise ce log pour détecter les changements de commande : si les flags de compilation d'un fichier changent (même si le fichier source n'a pas changé), Ninja sait qu'il doit recompiler. Make, en comparaison, ne détecte que les changements de timestamps de fichiers — pas les changements de commandes.

---

## Détection de péremption : timestamps vs commandes

C'est une différence fondamentale entre Ninja et Make.

**Make** décide de recompiler un fichier en comparant les **timestamps** : si un prérequis est plus récent que la cible, la cible est reconstruite. C'est simple mais incomplet — changer un flag de compilation (`-O2` → `-O3`) ne modifie aucun timestamp de fichier, donc Make ne recompile rien.

**Ninja** utilise une double vérification :

1. **Timestamps** : comme Make, si un fichier source est plus récent que le fichier objet, recompiler.
2. **Hash de la commande** : si la commande de compilation a changé depuis la dernière exécution (enregistrée dans `.ninja_log`), recompiler — même si aucun fichier n'a changé.

Cette détection de changement de commande est précieuse. Quand vous modifiez un flag dans le CMakeLists.txt et relancez CMake, les lignes de compilation changent dans `build.ninja`. Ninja détecte que les commandes ont changé et recompile les fichiers affectés. Avec Make, vous devriez faire un `make clean` suivi d'un rebuild complet pour obtenir le même résultat.

---

## Pools de jobs : contrôle de la parallélisation

Ninja parallélise les tâches par défaut, en détectant automatiquement le nombre de cœurs CPU. Mais certaines tâches ne doivent pas être parallélisées — par exemple, le linkage consomme beaucoup de mémoire et lancer plusieurs linkages simultanés peut saturer la RAM.

Ninja résout ce problème avec les **pools** :

```ninja
pool link_pool
  depth = 2

build my_app: link main.o utils.o
  pool = link_pool
```

Le pool `link_pool` autorise au maximum 2 linkages simultanés, même si Ninja dispose de 16 cœurs pour les compilations. Deux pools sont prédéfinis :

- **`console`** : un seul job à la fois, avec accès au terminal (stdin/stdout). Utilisé pour les commandes interactives.
- Le pool par défaut (non nommé) : pas de limite autre que le nombre de cœurs.

CMake utilise les pools automatiquement quand il détecte des tâches coûteuses. Vous n'avez généralement pas à les configurer manuellement.

---

## Ninja et les éditeurs / IDE

Les générateurs Ninja et Makefile de CMake supportent la génération du fichier `compile_commands.json` — la base de données de compilation utilisée par les outils d'analyse. Pour l'activer :

```bash
cmake -B build -G Ninja -DCMAKE_EXPORT_COMPILE_COMMANDS=ON  
ls build/compile_commands.json  
```

Ce fichier contient la commande de compilation exacte de chaque fichier source, au format JSON :

```json
[
  {
    "directory": "/home/dev/my_project/build",
    "command": "g++ -std=c++23 -Wall -Wextra -I../include -c ../src/core.cpp -o src/CMakeFiles/my_project_core.dir/core.cpp.o",
    "file": "../src/core.cpp"
  }
]
```

Ce fichier est exploité par `clangd` (autocomplétion, diagnostics en temps réel), `clang-tidy` (analyse statique), et les extensions IDE (VS Code C/C++, CLion). L'option `CMAKE_EXPORT_COMPILE_COMMANDS=ON` est nécessaire pour les deux générateurs (Ninja et Make). Il est recommandé de l'activer systématiquement, par exemple dans un CMake Preset ou via la variable d'environnement `CMAKE_EXPORT_COMPILE_COMMANDS=ON`.

---

## Limites de Ninja

Ninja n'est pas sans défauts. Ses limitations sont le revers de sa spécialisation :

**Diagnostic moins intuitif que Make.** Ninja supporte le dry-run (`ninja -n`) et dispose d'outils de diagnostic (`ninja -t`), mais il n'a pas d'équivalent au mode interactif de débogage de Make. Le diagnostic se fait principalement en lisant `build.ninja` ou en utilisant les sous-commandes `ninja -t` (voir section 28.3.2).

**Pas d'écriture manuelle pratique.** Le format `build.ninja` n'est pas conçu pour être écrit à la main. Les variables sont limitées, il n'y a pas de conditions ni de fonctions. Pour un petit projet sans CMake, un Makefile reste plus pratique.

**Dépendance à un générateur.** Ninja a toujours besoin d'un outil de niveau supérieur (CMake, Meson, GN) pour produire le fichier `build.ninja`. Il ne peut pas fonctionner seul comme Make. C'est un choix de conception, pas un manque — mais cela signifie que Ninja n'est utile que dans un pipeline à deux niveaux.

**Format non stable entre versions.** Bien que les changements soient rares, le format `build.ninja` peut évoluer entre les versions de Ninja. Les fichiers générés par CMake incluent une directive `ninja_required_version` qui vérifie la compatibilité.

---

## Outils intégrés : `ninja -t`

Ninja embarque des sous-commandes de diagnostic accessibles via `ninja -t` :

```bash
# Lister les cibles disponibles
ninja -C build -t targets

# Afficher les commandes qui seraient exécutées (dry-run)
ninja -C build -t commands my_project_core

# Graphe de dépendances au format Graphviz
ninja -C build -t graph my_app | dot -Tsvg -o deps.svg

# Chemins de dépendance entre deux fichiers
ninja -C build -t deps src/CMakeFiles/my_project_core.dir/core.cpp.o

# Temps de build par fichier (basé sur .ninja_log)
ninja -C build -t compdb

# Nettoyage des fichiers produits par les règles de build
ninja -C build -t clean
```

La sous-commande `graph` est particulièrement utile pour visualiser le graphe de dépendances d'un projet et comprendre pourquoi un fichier est (ou n'est pas) recompilé.

---

## Plan des sous-sections

| Sous-section | Thème | Ce que vous apprendrez |
|-------------|-------|----------------------|
| **28.3.1** | Pourquoi Ninja est plus rapide | Analyse technique des gains : parsing, no-op builds, parallélisation, détection de changements |
| **28.3.2** | Fichiers `build.ninja` | Syntaxe du format, règles, build statements, variables, et lecture des fichiers générés par CMake |

---

> **À suivre** : La sous-section 28.3.1 détaille les raisons techniques de la supériorité de performance de Ninja sur Make — du parsing du graphe de dépendances à l'exécution parallèle des tâches.

⏭️ [Pourquoi Ninja est plus rapide](/28-make-ninja/03.1-pourquoi-rapide.md)
