🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 34.1 gcov et lcov : Mesure de la couverture

## La chaîne de mesure en trois étapes

La mesure de couverture avec GCC repose sur un pipeline en trois phases distinctes. Comprendre ce pipeline est essentiel pour diagnostiquer les problèmes courants et adapter la configuration à son projet.

**Phase 1 — Instrumentation à la compilation.** GCC compile le code source avec des drapeaux spéciaux qui injectent des compteurs dans le binaire. Pour chaque ligne exécutable et chaque point de décision, le compilateur ajoute une instruction qui incrémente un compteur. Le binaire résultant est plus gros et plus lent que la version non instrumentée, mais fonctionnellement identique.

**Phase 2 — Exécution et collecte.** On exécute la suite de tests normalement. Pendant l'exécution, les compteurs enregistrent quelles lignes ont été traversées et combien de fois. À la fin du processus (terminaison propre du programme), les données sont écrites dans des fichiers de compteurs sur le disque.

**Phase 3 — Analyse et rapport.** `gcov` lit les fichiers de compteurs et produit des rapports texte par fichier source. `lcov` agrège ces rapports à l'échelle du projet et `genhtml` les transforme en pages HTML navigables.

## Installation des outils

`gcov` est distribué avec GCC — il est déjà présent si vous avez installé `g++` (section 2.1). `lcov` s'installe depuis les dépôts Ubuntu :

```bash
sudo apt update  
sudo apt install lcov  
```

Le paquet `lcov` inclut à la fois la commande `lcov` (agrégation des données) et `genhtml` (génération HTML). Vérifiez l'installation :

```bash
gcov --version  
lcov --version  
genhtml --version  
```

> 📝 **Versions.** lcov 2.x (disponible sur Ubuntu 24.04+) apporte des améliorations significatives par rapport à la branche 1.x : support natif des fichiers `.gcno`/`.gcda` de GCC 14+, meilleure gestion du C++20, et génération de rapports plus rapide. Si votre distribution fournit une version 1.x, envisagez d'installer la version 2.x depuis le dépôt GitHub officiel.

## Phase 1 : compilation instrumentée

### Les drapeaux de compilation

GCC propose un flag unique qui active toute l'instrumentation nécessaire :

```bash
g++ --coverage -o mon_programme main.cpp module.cpp
```

Le flag `--coverage` est un raccourci qui active simultanément deux mécanismes :

- **`-fprofile-arcs`** — Instrumente le binaire pour enregistrer le nombre de passages dans chaque arc du graphe de flot de contrôle (chaque branche de chaque condition).
- **`-ftest-coverage`** — Génère des fichiers `.gcno` (*GCC Notes*) au moment de la compilation, qui contiennent la cartographie entre les compteurs et les lignes du code source.

Les deux flags peuvent être spécifiés séparément, mais `--coverage` est la forme recommandée — plus concise et moins sujette à l'oubli de l'un des deux.

### Fichiers générés à la compilation

Après compilation, chaque fichier `.cpp` produit, en plus du fichier objet `.o` habituel, un fichier `.gcno` :

```
src/math.cpp      →  math.o  +  math.gcno  
src/validator.cpp  →  validator.o  +  validator.gcno  
```

Les fichiers `.gcno` sont des fichiers binaires qui décrivent la structure du code : quelles lignes correspondent à quels compteurs, quels arcs existent dans le graphe de contrôle. Ils sont générés à la compilation et ne changent qu'en cas de recompilation. Ils ne contiennent aucune donnée d'exécution — ce sont des métadonnées statiques.

### Impact sur le binaire

L'instrumentation a un coût mesurable :

- **Taille** : le binaire est typiquement 20 à 40% plus gros, à cause des compteurs et du code d'instrumentation.
- **Performance** : l'exécution est 10 à 30% plus lente, selon la densité du code en points de décision. Pour une suite de tests unitaires, cette dégradation est généralement imperceptible. Pour du benchmarking, elle est rédhibitoire — ne jamais mesurer les performances sur un binaire instrumenté.
- **Optimisation** : il est recommandé de compiler avec `-O0` (pas d'optimisation) lors de la mesure de couverture. Les optimisations `-O2`/`-O3` réorganisent le code, fusionnent des branches et éliminent du code mort, ce qui produit des résultats de couverture trompeurs — des lignes marquées non couvertes alors qu'elles ont été inlinées, ou des branches "couvertes" qui n'existent plus dans le binaire.

La combinaison recommandée est donc :

```bash
g++ --coverage -O0 -g -std=c++20 -o tests mon_code.cpp tests.cpp -lgtest -lgtest_main
```

Le `-g` (informations de débogage) n'est pas strictement nécessaire pour la couverture mais améliore les diagnostics en cas de problème.

## Phase 2 : exécution et collecte

### Exécuter les tests

L'exécution se fait normalement — le binaire instrumenté est un exécutable standard :

```bash
./tests
```

Pendant l'exécution, chaque compteur est incrémenté en mémoire. À la terminaison du programme, les données sont écrites dans des fichiers `.gcda` (*GCC Data*), un par fichier source instrumenté :

```
math.gcno  +  exécution  →  math.gcda  
validator.gcno  +  exécution  →  validator.gcda  
```

Les fichiers `.gcda` sont écrits dans le même répertoire que les fichiers `.gcno` — c'est-à-dire le répertoire où la compilation a eu lieu, pas le répertoire d'exécution. Ce détail est source de confusion fréquente dans les projets utilisant CMake avec un répertoire de build séparé (out-of-source build).

### Exécutions multiples

Si le binaire est exécuté plusieurs fois, les compteurs dans les fichiers `.gcda` sont **cumulés** par défaut. C'est un comportement utile : on peut exécuter plusieurs binaires de test (tests unitaires, tests d'intégration, tests end-to-end) et obtenir une couverture agrégée qui reflète l'ensemble de la suite.

```bash
./unit_tests
./integration_tests
./e2e_tests
# Les .gcda contiennent maintenant la couverture cumulée des trois exécutions
```

### Réinitialiser les compteurs

Pour recommencer une mesure depuis zéro, il faut supprimer les fichiers `.gcda` existants. `lcov` fournit une commande dédiée :

```bash
lcov --zerocounters --directory build/
```

Cette commande parcourt le répertoire spécifié et supprime tous les fichiers `.gcda`. C'est l'équivalent propre d'un `find build/ -name '*.gcda' -delete`, avec la garantie de ne toucher que les fichiers de couverture.

### Problème courant : terminaison anormale

Les fichiers `.gcda` ne sont écrits que si le programme se termine proprement (retour de `main()`, appel à `exit()`). Si le programme crashe (segfault, abort, exception non attrapée qui provoque `std::terminate`), les données de couverture sont perdues. C'est une raison supplémentaire d'exécuter la suite de tests sous les sanitizers (section 29.4) — un crash pendant la mesure de couverture signale un bug qui doit être corrigé avant de se soucier de couverture.

## Phase 3 : analyse avec gcov

### Utilisation basique

`gcov` est l'outil de bas niveau qui lit les fichiers `.gcno` et `.gcda` pour produire un rapport texte par fichier source :

```bash
gcov math.cpp
```

Cette commande produit un fichier `math.cpp.gcov` dans le répertoire courant. Le fichier ressemble à ceci :

```
        -:    0:Source:src/math.cpp
        -:    1:#include "math.hpp"
        -:    2:
        3:    3:int mp::add(int a, int b) {
        3:    4:    return a + b;
        -:    5:}
        -:    6:
        2:    7:int mp::divide(int num, int den) {
        2:    8:    if (den == 0)
        1:    9:        throw std::invalid_argument("division by zero");
        1:   10:    return num / den;
        -:   11:}
```

Le format est `compteur : numéro_de_ligne : code_source`. Le compteur indique combien de fois la ligne a été exécutée. Un tiret (`-`) signifie que la ligne n'est pas exécutable (commentaire, déclaration, accolade). Un `#####` signifie que la ligne est exécutable mais n'a jamais été atteinte — c'est la marque d'un déficit de couverture :

```
    #####:   15:    return "fallback";  // Jamais exécuté
```

### Couverture de branches avec gcov

Le flag `-b` ajoute les informations de couverture de branches :

```bash
gcov -b math.cpp
```

La sortie inclut alors des lignes supplémentaires :

```
        2:    8:    if (den == 0)
branch  0 taken 1 (fallthrough)  
branch  1 taken 1  
```

`branch 0 taken 1` signifie que la branche "vrai" (`den == 0` est vrai) a été empruntée une fois. `branch 1 taken 1` signifie que la branche "faux" a aussi été empruntée une fois. Si l'une des deux affichait `branch X never executed`, ce serait un signal de couverture incomplète.

### Limites de gcov brut

`gcov` est un outil par fichier — il faut l'appeler une fois par fichier source, et chaque invocation produit un fichier `.gcov` séparé. Sur un projet de cinquante fichiers, cela devient vite ingérable. De plus, `gcov` ne produit pas de synthèse globale (couverture totale du projet, couverture par répertoire) ni de sortie HTML. C'est là que `lcov` prend le relais.

## Agrégation avec lcov

`lcov` est une surcouche qui automatise l'appel à `gcov` sur tous les fichiers du projet et agrège les résultats dans un format unifié (`.info`).

### Capturer les données de couverture

```bash
lcov --capture \
     --directory build/ \
     --output-file coverage.info
```

La commande parcourt le répertoire `build/`, trouve tous les couples `.gcno`/`.gcda`, appelle `gcov` en interne et produit un fichier `coverage.info` qui contient la couverture agrégée de l'ensemble du projet. Ce fichier `.info` est le format pivot de lcov — c'est lui qui sera transformé en HTML par `genhtml`.

### Filtrer les résultats

Le rapport brut inclut tout le code instrumenté, y compris les headers système, les headers de GTest, et les sources de bibliothèques tierces récupérées via FetchContent. Ces éléments polluent le rapport et faussent les pourcentages. Le filtrage est indispensable :

```bash
# Supprimer les fichiers système et les dépendances externes
lcov --remove coverage.info \
     '/usr/*' \
     '*/build/_deps/*' \
     '*/tests/*' \
     --output-file coverage_filtered.info
```

Les patterns de `--remove` acceptent des wildcards. Les chemins à exclure dépendent de la structure de votre projet, mais les trois catégories ci-dessus sont quasi universelles :

- `/usr/*` — Headers système et librairie standard.
- `*/build/_deps/*` — Dépendances téléchargées par FetchContent (GTest, Google Mock, etc.).
- `*/tests/*` — Le code de test lui-même. On mesure la couverture du code de **production**, pas du code de test.

### Alternative : capturer uniquement le code pertinent

Au lieu de capturer tout puis filtrer, on peut restreindre la capture dès le départ avec `--include` (lcov 2.x) :

```bash
lcov --capture \
     --directory build/ \
     --include '*/src/*' \
     --include '*/include/*' \
     --output-file coverage.info
```

Cette approche est souvent plus propre et plus rapide — elle évite de traiter puis de jeter des milliers de fichiers système.

### Capturer une baseline

Pour distinguer le code non couvert du code non exécuté (code dans des bibliothèques non instrumentées), lcov permet de capturer une **baseline** avant l'exécution des tests :

```bash
# Avant l'exécution des tests : capturer les compteurs à zéro
lcov --capture --initial \
     --directory build/ \
     --output-file coverage_base.info

# Exécuter les tests
cd build && ctest --output-on-failure

# Capturer les compteurs après exécution
lcov --capture \
     --directory build/ \
     --output-file coverage_test.info

# Combiner baseline et résultats
lcov --add-tracefile coverage_base.info \
     --add-tracefile coverage_test.info \
     --output-file coverage_total.info
```

La baseline garantit que chaque fichier instrumenté apparaît dans le rapport, même si aucun test ne l'a exécuté. Sans baseline, un fichier entièrement non couvert pourrait être absent du rapport (pas de `.gcda` généré), donnant une fausse impression de couverture élevée.

## Génération du rapport HTML

`genhtml` transforme le fichier `.info` en un ensemble de pages HTML statiques :

```bash
genhtml coverage_filtered.info \
       --output-directory coverage_report/ \
       --title "Mon Projet — Couverture" \
       --legend \
       --branch-coverage
```

Les options les plus utiles :

- **`--title`** — Titre affiché en haut de chaque page du rapport.
- **`--legend`** — Ajoute une légende expliquant le code couleur.
- **`--branch-coverage`** — Inclut la couverture de branches en plus de la couverture de lignes. Fortement recommandé — sans cette option, seule la couverture de lignes est affichée, ce qui masque les branches non testées.

Le rapport est un répertoire de fichiers HTML statiques que l'on peut ouvrir directement :

```bash
xdg-open coverage_report/index.html    # Linux  
open coverage_report/index.html         # macOS  
```

### Lire le rapport

La page d'accueil affiche une vue hiérarchique du projet avec trois colonnes de pourcentages : couverture de lignes, couverture de fonctions et couverture de branches. Le code couleur est immédiat :

- **Vert** — Couverture supérieure au seuil haut (par défaut 90%).
- **Orange** — Couverture entre le seuil bas et le seuil haut (75-90%).
- **Rouge** — Couverture inférieure au seuil bas (< 75%).

En cliquant sur un répertoire puis sur un fichier, on accède à la vue source annotée. Chaque ligne est colorée selon son statut :

- **Bleu/vert** — Ligne exécutée au moins une fois.
- **Rouge** — Ligne exécutable mais jamais atteinte.
- **Blanc** — Ligne non exécutable (déclarations, commentaires, accolades).

Le nombre à gauche de chaque ligne indique le nombre d'exécutions. Une ligne exécutée 10 000 fois n'est pas "mieux couverte" qu'une ligne exécutée une fois — mais un compteur très élevé dans une boucle confirme que les tests exercent bien les itérations.

Les informations de branches apparaissent dans une colonne dédiée à droite du code source. Chaque point de décision affiche un indicateur compact : `+` pour une branche empruntée, `-` pour une branche jamais empruntée. Repérer les `-` dans le rapport est le moyen le plus efficace d'identifier les cas de test manquants.

## Workflow complet en ligne de commande

Récapitulons la séquence complète, de la compilation au rapport HTML, dans un projet CMake standard :

```bash
# 1. Configuration avec instrumentation
cmake -B build-cov -G Ninja \
      -DCMAKE_BUILD_TYPE=Debug \
      -DCMAKE_CXX_FLAGS="--coverage -O0"

# 2. Compilation
cmake --build build-cov

# 3. Réinitialiser les compteurs (si build-cov existait déjà)
lcov --zerocounters --directory build-cov/

# 4. Capturer la baseline
lcov --capture --initial \
     --directory build-cov/ \
     --output-file coverage_base.info

# 5. Exécuter les tests
cd build-cov && ctest --output-on-failure && cd ..

# 6. Capturer les résultats
lcov --capture \
     --directory build-cov/ \
     --output-file coverage_test.info

# 7. Combiner baseline et résultats
lcov --add-tracefile coverage_base.info \
     --add-tracefile coverage_test.info \
     --output-file coverage_total.info

# 8. Filtrer
lcov --remove coverage_total.info \
     '/usr/*' '*/build-cov/_deps/*' '*/tests/*' \
     --output-file coverage_filtered.info

# 9. Générer le rapport HTML
genhtml coverage_filtered.info \
       --output-directory coverage_report/ \
       --branch-coverage

# 10. Ouvrir le rapport
xdg-open coverage_report/index.html
```

Ce workflow en dix commandes est le socle sur lequel la section 34.3 construira une cible CMake automatisée. En attendant, il est parfaitement fonctionnel en l'état pour une mesure ponctuelle.

## Alternative : llvm-cov avec Clang

Si vous compilez avec Clang plutôt que GCC, la chaîne d'outils diffère. Clang utilise son propre système de profiling, incompatible avec `gcov` :

```bash
# Compilation instrumentée avec Clang
clang++ -fprofile-instr-generate -fcoverage-mapping -O0 -g \
        -o tests mon_code.cpp tests.cpp -lgtest -lgtest_main

# Exécution (génère un fichier .profraw)
LLVM_PROFILE_FILE="coverage.profraw" ./tests

# Fusion des données
llvm-profdata merge -sparse coverage.profraw -o coverage.profdata

# Rapport texte
llvm-cov report ./tests -instr-profile=coverage.profdata

# Rapport HTML
llvm-cov show ./tests -instr-profile=coverage.profdata \
         -format=html -output-dir=coverage_report/
```

La chaîne Clang offre une couverture de **régions** (*region coverage*) plus fine que la couverture de lignes — elle distingue par exemple les deux branches d'un opérateur ternaire sur une même ligne. En contrepartie, l'intégration avec lcov/genhtml n'est pas native (bien que lcov 2.x supporte un mode `llvm` expérimental).

Pour les projets qui compilent avec les deux compilateurs (matrice CI, section 38.7), il est courant de mesurer la couverture avec GCC/gcov (meilleure intégration lcov) et de réserver Clang pour l'analyse statique et les sanitizers.

## Diagnostiquer les problèmes courants

### Aucun fichier .gcda généré

Le programme a probablement crashé avant de terminer proprement, ou les fichiers `.gcno` ne sont pas trouvés. Vérifiez que le programme retourne un code de sortie normal (`exit(0)` ou retour de `main()`) et que les chemins de compilation correspondent aux chemins d'exécution.

### Couverture à 0% malgré des tests qui passent

Les fichiers `.gcno` et `.gcda` sont dans des répertoires différents. Cela arrive quand le répertoire de travail à l'exécution diffère du répertoire de compilation. Utilisez `--directory` dans `lcov` pour pointer vers le bon répertoire, ou spécifiez `GCOV_PREFIX` et `GCOV_PREFIX_STRIP` comme variables d'environnement pour rediriger l'écriture des `.gcda`.

### Couverture anormalement basse

Les fichiers source ont été recompilés après la dernière exécution des tests — les `.gcno` ne correspondent plus aux `.gcda`. Reconstruisez et relancez les tests. Alternativement, les fichiers `.gcda` d'une exécution précédente polluent les résultats : utilisez `lcov --zerocounters` avant de recommencer.

### lcov signale des erreurs sur des fichiers système

Les headers système instrumentés produisent des avertissements ou des erreurs dans lcov. Le filtrage via `--remove` ou `--include` (détaillé plus haut) résout ce problème. Avec lcov 2.x, l'option `--ignore-errors source` permet de contourner les erreurs liées à des sources inaccessibles.

---


⏭️ [Génération de rapports HTML](/34-couverture-code/02-rapports-html.md)
