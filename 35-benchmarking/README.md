🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 35. Benchmarking

## Introduction

Les chapitres 33 et 34 ont couvert la vérification de la **correction** du code : est-ce que le programme fait ce qu'il est censé faire ? Le benchmarking aborde une question complémentaire et tout aussi critique en C++ : **est-ce que le programme le fait assez vite ?**

C++ est choisi précisément pour les contextes où la performance compte — serveurs haute fréquence, moteurs de jeu, systèmes embarqués, pipelines de données. Dans ces contextes, une régression de performance de 15% peut être aussi grave qu'un bug fonctionnel. Pourtant, là où les tests unitaires sont devenus une pratique standard, la mesure systématique des performances reste souvent artisanale : un `std::chrono::high_resolution_clock` glissé dans le `main`, un `time ./programme` dans le terminal, voire un "ça semble plus rapide" subjectif.

Le micro-benchmarking rigoureux remplace ces approximations par des mesures reproductibles, statistiquement fiables et intégrables dans la CI. C'est la différence entre "je crois que c'est rapide" et "cette fonction traite 2,3 millions d'éléments par seconde avec un écart-type de 1,2% sur 100 itérations".

## Pourquoi mesurer est difficile

Mesurer la performance d'un fragment de code semble trivial — chronométrer avant et après, calculer la différence. En réalité, les pièges sont nombreux et les résultats naïfs sont presque toujours faux.

### Le compilateur optimise votre benchmark

Le premier adversaire est le compilateur lui-même. GCC et Clang avec `-O2` ou `-O3` sont remarquablement agressifs pour éliminer le code dont le résultat n'est pas utilisé. Un benchmark naïf comme celui-ci :

```cpp
auto start = std::chrono::high_resolution_clock::now();  
for (int i = 0; i < 1'000'000; ++i) {  
    std::vector<int> v{1, 2, 3, 4, 5};
    std::sort(v.begin(), v.end());
}
auto end = std::chrono::high_resolution_clock::now();
```

peut être optimisé à **zéro instruction** par le compilateur : le vecteur est local, le résultat du tri n'est jamais lu, la boucle entière est du code mort. Le benchmark affiche alors un temps de 0 nanosecondes — non pas parce que le tri est infiniment rapide, mais parce qu'il n'a jamais eu lieu.

Google Benchmark résout ce problème avec des mécanismes dédiés (`benchmark::DoNotOptimize`, `benchmark::ClobberMemory`) qui forcent le compilateur à conserver le code mesuré sans en altérer la sémantique.

### Le CPU n'est pas déterministe

Un processeur moderne est un système complexe avec plusieurs niveaux de cache (L1, L2, L3), un prédicteur de branches, du prefetching automatique, du frequency scaling (turbo boost) et du partage de ressources avec les autres processus. La même fonction exécutée deux fois de suite sur le même processeur peut varier de 10 à 30% simplement à cause de l'état du cache ou de la fréquence courante du CPU.

Un benchmark fiable nécessite de multiples itérations et un traitement statistique qui produit une moyenne, une médiane et une mesure de dispersion. Exécuter une opération une seule fois et rapporter le résultat est une anecdote, pas une mesure.

### L'environnement interfère

Un benchmark exécuté sur un ordinateur portable pendant une compilation en arrière-plan ne mesure pas la même chose qu'un benchmark exécuté sur une machine dédiée au repos. Les context switches, la pression mémoire, les interruptions hardware et les démons système introduisent du bruit qui peut noyer le signal.

En CI, les machines sont souvent partagées (conteneurs, VMs) et la variabilité est encore plus forte. Google Benchmark fournit des outils pour détecter ce bruit et avertir quand les résultats sont potentiellement peu fiables.

## Micro-benchmark vs macro-benchmark

La discipline du benchmarking distingue deux échelles de mesure complémentaires.

### Micro-benchmarking

Le micro-benchmark mesure la performance d'une **unité isolée** : une fonction, un algorithme, un conteneur, une opération. Il répond à des questions comme "combien de temps prend une insertion dans un `std::map` vs un `std::unordered_map` ?" ou "quel est le coût d'une allocation dynamique ?".

C'est l'équivalent du test unitaire pour la performance. L'unité est isolée de son contexte, mesurée dans des conditions contrôlées, et les résultats sont précis et reproductibles. Google Benchmark est l'outil de référence pour cette échelle — c'est le sujet principal de ce chapitre.

### Macro-benchmarking

Le macro-benchmark mesure la performance d'un **système complet** ou d'un scénario end-to-end : temps de réponse d'une API sous charge, débit d'un pipeline de traitement, latence d'une requête à travers toutes les couches applicatives.

Les outils sont différents : `perf` (section 31.1) pour le profiling CPU, `wrk` ou `ab` pour le load testing HTTP, des harnais de test spécifiques au domaine. Le macro-benchmark capture les interactions entre composants (cache, réseau, I/O disque) que le micro-benchmark ignore par conception.

Les deux échelles se complètent. Le micro-benchmark identifie les fonctions lentes ; le macro-benchmark vérifie que leur optimisation a un impact réel sur le système complet. Une optimisation qui divise par deux le temps d'une fonction n'a aucune valeur si cette fonction représente 0,1% du temps total d'exécution.

## Google Benchmark : le standard de l'industrie

**Google Benchmark** est la bibliothèque de micro-benchmarking de référence en C++, au même titre que Google Test l'est pour les tests unitaires. Développée par Google et utilisée en interne sur leur codebase, elle est adoptée par la majorité des projets C++ open source qui mesurent leurs performances : Abseil, Folly, LLVM, Protocol Buffers, gRPC.

Les raisons de cette domination sont similaires à celles de Google Test :

**Protection contre l'optimisation.** Le framework empêche le compilateur d'éliminer le code mesuré, sans introduire de surcoût artificiel qui fausserait les résultats.

**Traitement statistique automatique.** Chaque benchmark est exécuté suffisamment d'itérations pour produire des résultats stables. Le framework ajuste dynamiquement le nombre d'itérations en fonction de la durée de l'opération — une opération de 1 nanoseconde sera mesurée sur des millions d'itérations, une opération de 1 seconde sur quelques itérations seulement.

**Rapports structurés.** Les résultats sont affichés dans un tableau console lisible et exportables en JSON ou CSV pour analyse ultérieure, comparaison historique ou intégration CI.

**Benchmarks paramétrés.** Comme les tests paramétrés de GTest, Google Benchmark permet d'exécuter un même benchmark avec différentes tailles de données, différentes configurations ou différents algorithmes, et de comparer les résultats côte à côte.

**Comparaison A/B.** L'outil `compare.py` fourni avec la bibliothèque compare deux exécutions de benchmarks et calcule le speedup relatif, ce qui permet de quantifier précisément l'impact d'une modification.

## Benchmarking et cycle de développement

Le benchmarking s'insère dans le cycle de développement à deux niveaux.

**Pendant l'optimisation.** Quand on optimise un chemin critique, le workflow est itératif : mesurer → modifier → remesurer → comparer. Chaque itération doit produire des résultats comparables, ce qui nécessite des conditions de mesure stables et un outil capable de comparer deux runs de manière fiable. C'est le cœur de l'utilisation de Google Benchmark.

**En régression dans la CI.** Comme les tests unitaires détectent les régressions fonctionnelles, les benchmarks peuvent détecter les régressions de performance. L'approche est plus délicate — la variabilité des machines CI rend les seuils absolus peu fiables — mais des techniques comme la comparaison relative contre un baseline et la détection de déviations statistiques permettent d'automatiser la surveillance des performances critiques. La section 35.3 aborde l'interprétation des résultats dans ce contexte.

## Ce que vous apprendrez dans ce chapitre

La **section 35.1** couvre Google Benchmark en détail : installation, écriture de benchmarks, mécanismes anti-optimisation, benchmarks paramétrés et fixtures. C'est la section la plus conséquente du chapitre.

La **section 35.2** traite de la méthodologie de mesure fiable : stabilisation de l'environnement, réduction du bruit, et les précautions nécessaires pour produire des résultats en lesquels on peut avoir confiance.

La **section 35.3** aborde l'interprétation des résultats : lecture des rapports, comparaison entre deux versions, détection des régressions et intégration dans un workflow de suivi de performance.

## Prérequis

Ce chapitre s'appuie sur les compétences suivantes :

- **CMake** (chapitre 26) : l'intégration de Google Benchmark suit le même modèle que Google Test (`FetchContent`, `target_link_libraries`).
- **Options de compilation** (section 2.6) : comprendre la distinction entre `-O0`, `-O2` et `-O3` est fondamental — un benchmark compilé sans optimisation mesure une réalité très différente de la production.
- **STL et conteneurs** (chapitres 13-15) : les exemples comparent les performances de différents conteneurs et algorithmes de la librairie standard.
- **Cache CPU et localité des données** (section 41.1) : mentionné en contexte pour interpréter les résultats. La connaissance détaillée n'est pas requise mais enrichit la compréhension.

---


⏭️ [Google Benchmark : Micro-benchmarking](/35-benchmarking/01-google-benchmark.md)
