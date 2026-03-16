🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 31.2 — gprof : Profiling basé sur instrumentation

## Introduction

`gprof` (*GNU Profiler*) est l'un des plus anciens outils de profiling sous Unix. Intégré à la chaîne de compilation GCC depuis les années 1980, il repose sur un paradigme fondamentalement différent de `perf` : au lieu d'échantillonner périodiquement l'état du programme pendant son exécution, `gprof` **instrumente le binaire à la compilation** pour que chaque fonction enregistre elle-même ses appels et le temps qu'elle consomme.

Cette approche par instrumentation produit des résultats déterministes — chaque appel de fonction est comptabilisé avec exactitude — ce qui constitue à la fois la force et la faiblesse de `gprof`. La force, car le profil est exhaustif : aucune fonction n'échappe au comptage, même celles appelées une seule fois. La faiblesse, car l'instrumentation modifie le comportement du programme, introduisant un surcoût de 10 à 30% qui peut altérer les proportions de temps mesurées.

En 2026, `perf` a largement supplanté `gprof` pour le profiling de performance au quotidien. Cependant, `gprof` conserve sa pertinence dans plusieurs contextes : systèmes embarqués où `perf` n'est pas disponible, environnements sans accès aux compteurs matériels (certaines machines virtuelles, conteneurs restreints), ou besoin spécifique de comptage exact du nombre d'appels par fonction. Cette section couvre son utilisation et le compare systématiquement à `perf` pour vous aider à choisir l'outil adapté.

---

## Principe de fonctionnement

`gprof` combine deux mécanismes de collecte :

### Instrumentation des appels de fonctions

Lorsqu'un programme est compilé avec l'option `-pg`, GCC insère un appel à la fonction `mcount` (ou `_mcount`) au début de chaque fonction du programme. À chaque invocation, `mcount` enregistre la paire (appelant, appelé) dans une table interne. En fin d'exécution, cette table contient le **graphe d'appels complet** du programme : quelles fonctions ont appelé quelles autres, et combien de fois.

```
Compilation normale :
    func_A() {          func_B() {
        ...                 ...
        func_B();           ...
        ...             }
    }

Compilation avec -pg :
    func_A() {          func_B() {
        _mcount();          _mcount();      ← Ajouté par le compilateur
        ...                 ...
        func_B();           ...
        ...             }
    }
```

### Échantillonnage du compteur programme

En parallèle de l'instrumentation, `gprof` utilise le même mécanisme que les profilers par sampling : un timer d'interruption (signal `SIGPROF`) échantillonne périodiquement l'adresse de l'instruction en cours d'exécution (le *Program Counter*). La fréquence par défaut est de 100 Hz (100 échantillons par seconde) — nettement inférieure aux 4 000 Hz par défaut de `perf`.

### Fusion des deux sources

Le profil final fusionne les deux sources d'information :

- L'**instrumentation** fournit le comptage exact des appels et le graphe de dépendances entre fonctions.
- L'**échantillonnage** fournit la répartition statistique du temps CPU par fonction.

Le temps passé dans chaque fonction est estimé statistiquement (par sampling), mais le nombre d'appels et les relations appelant/appelé sont exactes (par instrumentation). C'est cette combinaison qui distingue `gprof` d'un profiler purement par sampling comme `perf`.

---

## Workflow complet

### Étape 1 : Compiler avec `-pg`

L'option `-pg` active l'instrumentation pour `gprof`. Elle doit être passée aussi bien à la compilation qu'à l'édition de liens :

```bash
g++ -std=c++23 -O2 -pg -g -o mon_programme mon_programme.cpp
```

> ⚠️ **`-pg` et `-O2`** : contrairement à une idée reçue, `-pg` est compatible avec les optimisations. Cependant, les fonctions inlinées par le compilateur n'apparaissent pas individuellement dans le profil (elles sont fusionnées avec leur appelant), ce qui peut masquer certains hotspots. Pour un profil plus fidèle au code source, `-O0 -pg` évite l'inlining, au prix d'un profil non représentatif des performances réelles.

Si le projet utilise CMake :

```cmake
# Ajout des flags de profiling gprof
target_compile_options(mon_programme PRIVATE -pg)  
target_link_options(mon_programme PRIVATE -pg)  
```

### Étape 2 : Exécuter le programme

```bash
./mon_programme < donnees_test.txt
```

Le programme s'exécute normalement mais génère automatiquement un fichier `gmon.out` dans le répertoire courant à la fin de son exécution. Ce fichier binaire contient les données de profiling brutes.

Points importants :

- Le fichier `gmon.out` n'est généré que si le programme se termine normalement (retour de `main` ou appel à `exit()`). Un programme tué par un signal (`kill`, `SIGKILL`, `SIGSEGV`) ne produit pas de fichier.
- Chaque exécution écrase le `gmon.out` précédent. Renommez-le si vous souhaitez conserver plusieurs profils.
- Le programme doit s'exécuter suffisamment longtemps pour que l'échantillonnage à 100 Hz produise des résultats significatifs. Pour un programme qui dure moins d'une seconde, le profil contiendra très peu d'échantillons et sera peu fiable.

### Étape 3 : Analyser avec `gprof`

```bash
gprof mon_programme gmon.out
```

`gprof` prend en entrée le binaire (pour les symboles) et le fichier `gmon.out` (pour les données), puis produit un rapport textuel sur la sortie standard. Ce rapport comporte deux sections principales : le *flat profile* et le *call graph*.

---

## Le Flat Profile

La première section du rapport est le profil plat, qui classe les fonctions par temps CPU décroissant :

```
Flat profile:

Each sample counts as 0.01 seconds.
  %   cumulative   self              self     total
 time   seconds   seconds    calls  ms/call  ms/call  name
 34.52     1.12     1.12   250000     0.00     0.01  Parser::analyser_token(...)
 18.77     1.73     0.61  1500000     0.00     0.00  Buffer::copier_octet(...)
 12.31     2.13     0.40   250000     0.00     0.00  Lexer::classifier(...)
  8.92     2.42     0.29       12     24.17    54.33  Pipeline::executer(...)
  7.08     2.65     0.23  3000000     0.00     0.00  is_delimiter(char)
  ...
```

Chaque colonne apporte une information distincte :

**% time** : le pourcentage du temps CPU total consommé par cette fonction (self time uniquement, sans les fonctions appelées). C'est l'équivalent de la colonne `Self` dans `perf report`.

**cumulative seconds** : la somme cumulée des self seconds en descendant dans la liste. Lorsque cette valeur atteint le temps total d'exécution, toutes les fonctions sont couvertes.

**self seconds** : le temps CPU passé exclusivement dans cette fonction, sans compter le temps passé dans les fonctions qu'elle appelle.

**calls** : le nombre exact d'invocations de la fonction. C'est l'information unique qu'apporte l'instrumentation — `perf` ne fournit pas cette donnée (sauf via des tracepoints spécifiques). Ici, `analyser_token` a été appelée exactement 250 000 fois.

**self ms/call** : le temps moyen par appel (self time / calls). Utile pour distinguer les fonctions intrinsèquement lentes (ms/call élevé, peu d'appels) des fonctions rapides mais appelées massivement (ms/call faible, millions d'appels).

**total ms/call** : le temps moyen par appel incluant les fonctions appelées. Pour `Pipeline::executer()`, 24.17 ms de self time mais 54.33 ms au total — elle passe plus de la moitié de son temps dans des sous-fonctions.

### Exploiter le flat profile

Le flat profile se lit de haut en bas. Les premières lignes sont les hotspots, les cibles prioritaires d'optimisation. La combinaison de `calls` et `self ms/call` oriente la stratégie :

- **Beaucoup d'appels, faible ms/call** (comme `Buffer::copier_octet` : 1.5M appels, ~0.0004 ms/appel) : le coût unitaire est faible mais le volume est le problème. L'optimisation consiste à réduire le nombre d'appels (regrouper les copies, utiliser `memcpy` au lieu de copier octet par octet).

- **Peu d'appels, fort ms/call** (comme `Pipeline::executer` : 12 appels, 24.17 ms/appel) : chaque appel est coûteux. L'optimisation se concentre sur l'algorithme interne de la fonction.

---

## Le Call Graph

La seconde section du rapport est le graphe d'appels, qui montre les relations appelant/appelé avec les temps associés :

```
                     Call graph


granularity: each sample hit covers 2 byte(s) for 0.31% of 3.25 seconds

index % time    self  children    called     name
                                                <spontaneous>
[1]    100.0    0.00    3.25                 main [1]
                0.29    0.36      12/12          Pipeline::executer(...) [2]
                0.04    0.12       1/1           Config::charger(...) [5]
                0.00    0.01       1/1           Logger::initialiser() [12]
-----------------------------------------------
                0.29    0.36      12/12          main [1]
[2]     20.0    0.29    0.36      12         Pipeline::executer(...) [2]
                1.12    0.25  250000/250000      Parser::analyser_token(...) [3]
                0.10    0.00  250000/250000      Resultat::stocker(...) [7]
-----------------------------------------------
                1.12    0.25  250000/250000      Pipeline::executer(...) [2]
[3]     42.2    1.12    0.25  250000         Parser::analyser_token(...) [3]
                0.61    0.00 1500000/1500000     Buffer::copier_octet(...) [4]
                0.23    0.00 3000000/3000000     is_delimiter(char) [6]
                0.40    0.00  250000/250000      Lexer::classifier(...) [8]
-----------------------------------------------
```

Le call graph se lit par blocs séparés par des lignes de tirets. Chaque bloc est centré sur une fonction (identifiée par un numéro d'index entre crochets) :

- Les lignes **au-dessus** de la fonction centrale montrent ses **appelants** (*callers*) : qui appelle cette fonction, combien de fois, et combien de temps est passé dans cet appel.
- La ligne **centrale** (en gras conceptuellement, marquée par l'index) montre la fonction elle-même avec son self time et son children time.
- Les lignes **en dessous** montrent ses **appelés** (*callees*) : quelles fonctions elle appelle, combien de fois, et combien de temps y est passé.

Dans le bloc de `Parser::analyser_token` (index [3]) :

- **Appelant** : `Pipeline::executer` l'appelle 250 000 fois, consommant 1.12s de self time et 0.25s dans ses enfants.
- **Self** : 1.12s passées dans `analyser_token` elle-même.
- **Children** : 0.25s passées dans les fonctions appelées depuis `analyser_token`.
- **Appelés** : `Buffer::copier_octet` (1.5M appels, 0.61s), `is_delimiter` (3M appels, 0.23s), `Lexer::classifier` (250K appels, 0.40s).

Le pourcentage `% time` (42.2% pour `analyser_token`) est le *cumulative time* — la part du temps total passée dans cette fonction *et tout ce qu'elle appelle*. C'est l'équivalent de la colonne `Children` dans `perf report`.

### Valeur du call graph

Le call graph apporte une information que le flat profile ne donne pas : la **chaîne de causalité**. Le flat profile montre que `is_delimiter` consomme 7% du temps, mais ne dit pas *qui* l'appelle. Le call graph révèle que 100% de ces appels proviennent de `analyser_token`. Si on optimise `analyser_token` pour appeler `is_delimiter` moins souvent (en regroupant les tests, par exemple), l'impact se propage dans tout le graphe.

---

## Options utiles de `gprof`

### Sortie condensée

```bash
# Flat profile uniquement (sans call graph)
gprof -p mon_programme gmon.out

# Call graph uniquement (sans flat profile)
gprof -q mon_programme gmon.out
```

### Suppression des fonctions système

Pour se concentrer sur les fonctions du programme et ignorer les fonctions de la librairie standard :

```bash
gprof -E __libc_start_main -E _init mon_programme gmon.out
```

L'option `-E` (*exclude*) supprime une fonction et tous ses enfants du profil.

### Format lisible par machine

```bash
gprof -b mon_programme gmon.out    # Mode brief : supprime les explications textuelles
```

Le mode `-b` (*brief*) supprime les blocs d'aide intercalés dans le rapport, ne laissant que les données. Utile pour le traitement automatisé ou lorsqu'on est déjà familier avec le format.

### Redirection vers un fichier

```bash
gprof mon_programme gmon.out > profil_rapport.txt
```

---

## Limites de `gprof`

### Surcoût de l'instrumentation

L'insertion de `_mcount` au début de chaque fonction a un coût non négligeable. Pour les fonctions très courtes appelées des millions de fois (comme `is_delimiter` dans notre exemple), le surcoût de l'instrumentation peut représenter une fraction significative du temps mesuré. Le profil reflète alors le coût de l'instrumentation autant que le coût réel de la fonction — un biais systématique absent du profiling par sampling.

### Résolution temporelle limitée

L'échantillonnage à 100 Hz signifie qu'un intervalle de 10 ms est le quantum de temps minimal. Les fonctions dont le temps total est inférieur à 10 ms peuvent apparaître avec un self time de 0.00 secondes, même si elles consomment du temps CPU mesurable. `perf` échantillonne à 4 000 Hz par défaut (quantum de 0.25 ms), offrant une résolution 40× supérieure.

### Pas de profiling du noyau ni des librairies non instrumentées

`gprof` ne profile que le code compilé avec `-pg`. Le temps passé dans le noyau (appels système), dans les librairies dynamiques non recompilées avec `-pg`, ou dans des modules chargés dynamiquement est invisible. Ce temps apparaît comme du « temps perdu » — le total des self times est inférieur au temps d'exécution réel.

### Incompatibilité avec les programmes multithreads

`gprof` a un support très limité des programmes multithreads. Le fichier `gmon.out` n'est généré que pour le thread principal. Les threads créés avec `std::thread` ou `pthread_create` ne sont pas profilés, ce qui rend `gprof` inadapté à la majorité des programmes concurrents modernes.

### Pas de support des fonctions inlinées

Les fonctions inlinées par le compilateur (avec `-O2` ou supérieur) n'apparaissent pas dans le profil — elles sont fusionnées avec leur appelant. Le comptage d'appels et le self time de l'appelant incluent implicitement le travail des fonctions inlinées, sans les distinguer.

### Le fichier `gmon.out` est fragile

Le fichier `gmon.out` est lié au binaire spécifique qui l'a produit. Si vous recompilez le programme, même sans modifier le code source, le `gmon.out` précédent devient inutilisable (les adresses ont changé). Il faut toujours analyser un `gmon.out` avec le binaire exact qui l'a généré.

---

## `gprof` vs `perf` : guide de choix

| Critère | `gprof` | `perf` |
|---|---|---|
| **Comptage exact des appels** | Oui | Non (sauf tracepoints) |
| **Graphe d'appels** | Exact (instrumentation) | Statistique (sampling) |
| **Recompilation requise** | Oui (`-pg`) | Non (mais `-g` recommandé) |
| **Surcoût** | 10–30% | 1–5% |
| **Résolution temporelle** | 10 ms (100 Hz) | 0.25 ms (4 000 Hz) |
| **Support multithreading** | Thread principal uniquement | Tous les threads |
| **Profiling noyau** | Non | Oui |
| **Profiling librairies tierces** | Non (sauf recompilation avec `-pg`) | Oui |
| **Compteurs matériels** | Non | Oui (cache, branches, IPC) |
| **Disponibilité** | Partout où GCC est présent | Linux uniquement |

**Recommandation générale** : utilisez `perf` comme outil de profiling par défaut. Réservez `gprof` aux situations où :

- Vous avez besoin du **comptage exact** du nombre d'appels par fonction (pour valider un modèle de complexité algorithmique, par exemple).
- `perf` n'est pas disponible (système embarqué, environnement virtualisé sans accès PMU, système non Linux).
- Vous travaillez sur un projet **monothread** de taille modeste où la simplicité d'utilisation de `gprof` (compiler, exécuter, lire) est un avantage.

Pour les programmes multithreads, les services de longue durée, ou toute situation nécessitant un diagnostic au niveau matériel (cache, branches), `perf` est le seul choix viable.

---

## Alternatives modernes à `gprof`

L'approche par instrumentation de `gprof` a inspiré des outils plus modernes qui corrigent certaines de ses limitations :

**Callgrind** (suite Valgrind) instrumente dynamiquement le binaire sans recompilation, profile tous les threads, et produit un graphe d'appels détaillé visualisable avec **KCachegrind** (section 31.4). Le surcoût est cependant très élevé (10 à 50×), comparable à Memcheck.

**uftrace** est un traceur de fonctions moderne pour Linux qui combine instrumentation à la compilation (`-pg` ou `-finstrument-functions`) et reconstruction dynamique du graphe d'appels. Il supporte le multithreading, produit des flamegraphs, et offre un surcoût moindre que `gprof` grâce à un mécanisme de buffering optimisé. C'est l'alternative la plus directe à `gprof` pour ceux qui souhaitent un comptage exact des appels avec un outillage moderne.

**Tracy** est un profiler orienté jeux vidéo et applications temps réel. Il combine instrumentation manuelle (macros insérées dans le code) et visualisation en temps réel avec une timeline interactive. Son surcoût est minimal grâce à un design *lock-free*, ce qui le rend utilisable en production.

Ces outils sont mentionnés ici pour compléter la carte de l'écosystème. Les sections 31.3 et 31.4 couvrent les outils de visualisation (flamegraphs, Hotspot, KCachegrind) qui s'appliquent à la plupart de ces profilers.

⏭️ [Flamegraphs et visualisation](/31-profiling/03-flamegraphs.md)
