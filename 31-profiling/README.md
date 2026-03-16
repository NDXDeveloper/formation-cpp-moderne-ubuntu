🔝 Retour au [Sommaire](/SOMMAIRE.md)

# Chapitre 31 : Profiling de Performance

## Module 10 : Débogage, Profiling et Qualité Code — Niveau Avancé

---

## Vue d'ensemble

Le chapitre 30 a traité l'analyse mémoire : détecter les fuites, profiler la consommation heap, résoudre les problèmes d'allocation. Ce chapitre s'attaque à l'autre dimension critique de la performance d'un programme : le **temps CPU**. Pourquoi un programme est-il lent ? Quelles fonctions consomment le plus de cycles ? Où se trouvent les goulots d'étranglement ? Le profiling de performance apporte des réponses factuelles à ces questions, remplaçant les intuitions par des mesures.

L'optimisation sans profiling est l'un des pièges les plus courants en ingénierie logicielle. Un développeur convaincu qu'une fonction est le goulot d'étranglement réécrit un algorithme complexe en version optimisée, pour découvrir que cette fonction ne représentait que 2% du temps d'exécution total. Pendant ce temps, une simple copie de `std::string` dans une boucle appelée un million de fois — invisible sans profiling — consommait 40% du temps CPU. Donald Knuth résumait ce problème par sa célèbre formule : *« premature optimization is the root of all evil »*. Le profiling est l'antidote : il identifie les *hotspots* réels avant d'investir du temps dans l'optimisation.

Sur Linux, l'écosystème d'outils de profiling est riche et mature. Ce chapitre se concentre sur les outils les plus pertinents pour le développeur C++ : `perf` (l'outil natif du noyau Linux, puissant et polyvalent), `gprof` (l'outil classique basé sur l'instrumentation), et les techniques de visualisation — en particulier les flamegraphs — qui transforment des données de profiling brutes en représentations immédiatement exploitables.

---

## Objectifs du chapitre

À l'issue de ce chapitre, vous serez en mesure de :

- Utiliser **`perf`** pour échantillonner l'activité CPU d'un programme C++, enregistrer un profil, et identifier les fonctions les plus coûteuses grâce à `perf record` et `perf report`.
- Exploiter **`perf stat`** pour mesurer les compteurs matériels du processeur (cache misses, branch mispredictions, instructions par cycle) et diagnostiquer les problèmes de performance liés à l'architecture matérielle.
- Utiliser **`gprof`** pour obtenir un profil basé sur l'instrumentation, comprendre ses avantages et ses limites par rapport à `perf`.
- Générer et interpréter des **flamegraphs** pour visualiser la répartition du temps CPU à travers l'ensemble de la pile d'appels d'un programme.
- Choisir l'outil graphique adapté à votre contexte — **Hotspot**, **Flamescope**, **KCachegrind** — pour exploiter visuellement les profils de performance.

---

## Plan du chapitre

- **31.1 — perf : Profiling CPU et sampling**
  Présentation de `perf`, l'outil de profiling natif du noyau Linux. Installation, principe du sampling statistique, enregistrement de profils avec `perf record`, analyse interactive avec `perf report`, et utilisation de `perf stat` pour accéder aux compteurs matériels du processeur.

- **31.2 — gprof : Profiling basé sur instrumentation**
  L'approche alternative par instrumentation du binaire. Compilation avec `-pg`, exécution, génération et lecture du profil. Comparaison avec `perf` : quand chaque approche est-elle la plus adaptée.

- **31.3 — Flamegraphs et visualisation**
  Génération de flamegraphs à partir de données `perf`. Lecture et interprétation de ces visualisations. Flamegraphs inversés (*icicle graphs*) pour identifier les appelants les plus coûteux. Différence entre *on-CPU* et *off-CPU* flamegraphs.

- **31.4 — Hotspot et outils graphiques**
  Présentation de Hotspot (interface graphique pour les profils `perf`), KCachegrind (visualisation de profils Callgrind), et autres outils de l'écosystème. Workflows de profiling assistés par des interfaces graphiques pour le développement quotidien.

---

## Prérequis

Ce chapitre s'appuie sur les connaissances suivantes :

- **Chapitre 2 — Toolchain sur Ubuntu** : compilation avec `g++` et `clang++`, options de compilation (`-g`, `-O2`, `-O3`). Le profiling nécessite de comprendre l'impact des niveaux d'optimisation sur le code généré.
- **Chapitre 29 — Débogage Avancé** : familiarité avec GDB et les symboles de débogage. Les outils de profiling exploitent les mêmes informations de débogage pour associer les échantillons aux lignes de code source.
- **Chapitre 30 — Analyse Mémoire** : compréhension de Valgrind et de son outil Callgrind, qui sera mentionné en complément pour le profiling de graphes d'appels.

Une familiarité avec la ligne de commande Linux est indispensable. Certaines fonctionnalités de `perf` nécessitent des privilèges système (accès aux compteurs matériels, `perf_event_paranoid`), qui seront détaillés en section 31.1.

---

## Deux approches fondamentales du profiling

Avant d'aborder les outils individuellement, il est essentiel de comprendre les deux paradigmes de profiling qui sous-tendent l'ensemble de ce chapitre.

### Profiling par sampling (échantillonnage)

Le profiler interrompt périodiquement le programme (typiquement plusieurs milliers de fois par seconde) et enregistre l'état de la pile d'appels à chaque interruption. En agrégeant des milliers d'échantillons, un profil statistique émerge : les fonctions qui apparaissent le plus souvent dans les échantillons sont celles qui consomment le plus de temps CPU.

**Avantages** : surcoût très faible (typiquement 1 à 5%), fonctionne sur n'importe quel binaire sans recompilation, capable de profiler le noyau et les librairies système. **Inconvénients** : résultats statistiques (les fonctions très rarement appelées peuvent ne pas apparaître), granularité limitée par la fréquence d'échantillonnage.

`perf` utilise cette approche.

### Profiling par instrumentation

Le compilateur insère du code supplémentaire au début et à la fin de chaque fonction pour mesurer exactement le nombre d'appels et le temps passé. Le résultat est un profil déterministe : chaque appel de fonction est comptabilisé.

**Avantages** : résultats exacts (pas d'erreur statistique), comptage précis du nombre d'appels. **Inconvénients** : surcoût significatif (10 à 30%, parfois plus), nécessite une recompilation, le surcoût peut modifier le comportement du programme au point de fausser les résultats (effet « Heisenberg » du profiling).

`gprof` et Callgrind (de la suite Valgrind) utilisent cette approche.

### Choisir entre les deux

En pratique, le profiling par sampling est le choix par défaut pour la grande majorité des situations. Son faible surcoût le rend utilisable même sur des programmes en cours d'exécution en pré-production, et sa capacité à profiler sans recompilation est un avantage majeur. Le profiling par instrumentation est réservé aux cas où un comptage exact des appels est nécessaire ou lorsque `perf` n'est pas disponible.

| Critère | Sampling (`perf`) | Instrumentation (`gprof`) |
|---|---|---|
| **Recompilation requise** | Non (mais `-g` recommandé) | Oui (`-pg`) |
| **Surcoût à l'exécution** | 1–5% | 10–30% |
| **Précision** | Statistique | Exacte |
| **Comptage d'appels** | Non (sauf `perf stat`) | Oui |
| **Profiling du noyau** | Oui | Non |
| **Profiling de librairies tierces** | Oui | Non (sauf si compilées avec `-pg`) |
| **Disponibilité** | Noyau Linux 2.6.31+ | GCC (toutes plateformes) |

---

## Conventions de compilation pour le profiling

Le profiling soulève une tension entre l'observabilité et la représentativité. Compiler en `-O0 -g` facilite l'interprétation des résultats (chaque ligne de code source correspond à des instructions distinctes), mais le profil obtenu ne représente pas le comportement réel du programme en production, où les optimisations du compilateur transforment radicalement le code généré.

La recommandation pour le profiling de performance est de compiler avec les **optimisations activées et les symboles de débogage conservés** :

```bash
g++ -std=c++23 -O2 -g -o mon_programme mon_programme.cpp
```

L'option `-O2` (ou `-O3`) produit un binaire représentatif des conditions de production. L'option `-g` conserve les informations de débogage nécessaires pour que `perf`, `gprof` et les outils de visualisation puissent associer les échantillons aux fonctions et aux lignes du code source.

> ⚠️ **Différence avec l'analyse mémoire** : le chapitre 30 recommandait `-O0 -g` pour Valgrind, car les optimisations perturbent le suivi mémoire. Pour le profiling de performance, c'est l'inverse : `-O0` produit un profil non représentatif des conditions réelles. Profilez toujours avec le même niveau d'optimisation que la production.

Pour les profils les plus précis au niveau du code source, l'option `-fno-omit-frame-pointer` est fortement recommandée :

```bash
g++ -std=c++23 -O2 -g -fno-omit-frame-pointer -o mon_programme mon_programme.cpp
```

Par défaut, les compilateurs en mode optimisé utilisent le registre du *frame pointer* (`rbp` sur x86_64) comme registre général pour gagner en performance. Cela empêche `perf` de reconstruire les piles d'appels par *frame pointer walking*, qui est la méthode la plus rapide et la plus fiable. L'option `-fno-omit-frame-pointer` restaure le frame pointer, avec un impact négligeable sur les performances (typiquement < 1%) mais un bénéfice majeur pour la qualité des profils.

---

## Vue d'ensemble de l'écosystème d'outils

Le profiling de performance sur Linux repose sur un écosystème d'outils complémentaires. Ce chapitre couvre les plus importants pour un développeur C++, mais il est utile de connaître la carte d'ensemble :

| Outil | Rôle | Couvert dans |
|---|---|---|
| `perf record` / `perf report` | Profiling CPU par sampling | Section 31.1 |
| `perf stat` | Compteurs matériels (IPC, cache misses) | Section 31.1 |
| `gprof` | Profiling par instrumentation | Section 31.2 |
| Flamegraphs (Brendan Gregg) | Visualisation de profils en flammes | Section 31.3 |
| Hotspot | Interface graphique pour profils `perf` | Section 31.4 |
| KCachegrind | Visualisation de profils Callgrind | Section 31.4 |
| Callgrind (Valgrind) | Profiling de graphes d'appels | Chapitre 30 (mentionné) |
| `time` / `hyperfine` | Mesure du temps d'exécution global | — |
| Google Benchmark | Micro-benchmarking de fonctions | Chapitre 35 |

Le workflow typique d'une session de profiling est :

1. **Mesurer globalement** : `time` ou `hyperfine` pour confirmer qu'un problème de performance existe et quantifier sa magnitude.
2. **Profiler** : `perf record` pour identifier les hotspots CPU, ou `perf stat` pour diagnostiquer un problème matériel (cache, branches).
3. **Visualiser** : flamegraph ou Hotspot pour obtenir une vue d'ensemble exploitable.
4. **Optimiser** : modifier le code en ciblant les hotspots identifiés.
5. **Vérifier** : re-profiler pour confirmer l'amélioration et détecter d'éventuelles régressions.

Ce cycle mesure → profile → optimise → vérifie est le fondement d'une démarche d'optimisation disciplinée, guidée par les données plutôt que par l'intuition.

⏭️ [perf : Profiling CPU et sampling](/31-profiling/01-perf.md)
