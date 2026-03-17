🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 35.3 Interprétation des résultats

## Au-delà des chiffres bruts

Un benchmark qui affiche `4521 ns` ne dit rien par lui-même. Ce chiffre n'acquiert de sens que dans un contexte : comparé à quoi ? Mesuré dans quelles conditions ? Représentatif de quel scénario d'usage ? Un développeur qui optimise du code passe autant de temps à interpréter les résultats qu'à les produire — et une mauvaise interprétation conduit à des optimisations inutiles, voire contre-productives.

Cette section couvre la lecture des rapports Google Benchmark, les techniques de comparaison entre versions, les pièges d'interprétation courants et les méthodes pour transformer des chiffres en décisions d'ingénierie.

## Lire un rapport Google Benchmark

### Les colonnes standard

```
---------------------------------------------------------------------
Benchmark                          Time        CPU     Iterations
---------------------------------------------------------------------
BM_MapInsert/1000               8421 ns     8415 ns        83102  
BM_MapInsert/10000             98712 ns    98634 ns         7094  
BM_MapInsert/100000          1284531 ns  1283712 ns          545  
BM_UnorderedMapInsert/1000      3214 ns     3211 ns       217834  
BM_UnorderedMapInsert/10000    35891 ns    35842 ns        19523  
BM_UnorderedMapInsert/100000  412345 ns   411987 ns         1701  
```

**Time** est le temps mur (*wall clock*) — il inclut tout ce qui s'est passé pendant l'itération, y compris les interruptions, les context switches et les attentes I/O. **CPU** est le temps CPU consommé par le processus seul. Pour du code purement calculatoire sans I/O, les deux colonnes sont quasi identiques. Un écart significatif entre Time et CPU signale une interférence extérieure :

| Rapport Time/CPU | Interprétation |
|-------------------|----------------|
| ≈ 1.0 | Normal — pas d'attente | 
| 1.05 - 1.15 | Interférences mineures (scheduling, IRQ) |
| > 1.2 | Interférences significatives — résultats suspects |
| >> 2.0 | Le benchmark attend quelque chose (I/O, lock, réseau) |

**Iterations** reflète la durée de l'opération. Google Benchmark ajuste automatiquement ce nombre pour atteindre une durée totale de mesure suffisante. Un nombre d'itérations très faible (< 10) signale une opération longue dont la mesure est intrinsèquement plus bruitée — le framework a moins d'échantillons pour calculer une moyenne stable.

### Les colonnes de métriques personnalisées

Quand le benchmark utilise `SetItemsProcessed`, `SetBytesProcessed` ou des compteurs personnalisés (section 35.1), des colonnes supplémentaires apparaissent :

```
BM_JsonParse/1000     4521 ns    4518 ns    154832    221.34M items/s  
BM_Compress/65536    18.4 us    18.3 us     38124    3.41GB/s    ratio=3.72  
```

Le **débit** (items/s ou bytes/s) est souvent plus parlant que le temps brut. Dire "le parser traite 221 millions d'éléments par seconde" est immédiatement comparable à un objectif de performance ("nous avons besoin de traiter 100 millions d'événements par seconde"), là où "4521 nanosecondes par appel" nécessite un calcul mental supplémentaire.

### Les agrégats statistiques

Avec `--benchmark_repetitions=N`, Google Benchmark produit des lignes d'agrégats :

```
BM_Sort/10000_mean       62.4 us    62.3 us         10  
BM_Sort/10000_median     61.8 us    61.7 us         10  
BM_Sort/10000_stddev      2.1 us     2.1 us         10  
BM_Sort/10000_cv          3.37 %     3.36 %         10  
```

La **médiane** est généralement plus représentative que la moyenne pour les benchmarks — elle est résistante aux outliers (une itération polluée par un context switch). L'**écart-type** quantifie la dispersion. Le **coefficient de variation** (cv) est le ratio écart-type/moyenne en pourcentage — c'est l'indicateur de fiabilité principal (voir la grille en section 35.2).

Pour la prise de décision, la lecture recommandée est :

1. Vérifier le CV — en dessous de 5%, les résultats sont exploitables.
2. Lire la médiane — c'est la valeur la plus représentative.
3. Consulter la moyenne — un écart important entre médiane et moyenne signale des outliers.
4. Regarder l'écart-type — il définit la marge d'erreur naturelle de la mesure.

## Comparer deux versions

La comparaison est le cœur de l'interprétation. On compare rarement un benchmark à un objectif absolu — on compare presque toujours une version à une autre : avant/après une optimisation, deux algorithmes concurrents, deux structures de données.

### Utiliser compare.py

L'outil `compare.py` fourni avec Google Benchmark (section 35.1) produit un rapport de changement relatif :

```
Comparing baseline.json to feature.json  
Benchmark                        Time       CPU    Time Old   Time New  
----------------------------------------------------------------------
BM_Sort/1000                  -0.1523   -0.1518       4521       3832  
BM_Sort/10000                 -0.2104   -0.2098      62400      49270  
BM_Sort/100000                -0.1891   -0.1887     843000     683600  
BM_MapLookup/1000             +0.0034   +0.0031        891        894  
BM_MapLookup/10000            -0.0021   -0.0019       1243       1240  
```

Les colonnes Time et CPU montrent le **changement relatif** : -0.1523 signifie que le temps a diminué de 15,23% (amélioration), +0.0034 signifie qu'il a augmenté de 0,34% (régression marginale ou bruit).

### Interpréter les changements

La question centrale est : "Ce changement est-il réel ou est-ce du bruit ?" La réponse dépend du coefficient de variation des mesures.

**Règle pratique : un changement est significatif s'il dépasse 2× le CV.** Si vos benchmarks ont un CV de 3%, un changement de 6% est probablement réel. Un changement de 2% est probablement du bruit. Un changement de 4% est ambigu — il faut plus de répétitions ou un environnement plus stable pour trancher.

| Changement mesuré | CV des mesures | Conclusion |
|--------------------|----------------|------------|
| -15% | 3% | Amélioration significative et claire |
| -8% | 3% | Amélioration probable — confirmer avec plus de répétitions |
| -3% | 3% | Indistinguable du bruit — ne pas conclure |
| +2% | 3% | Bruit — aucune régression détectable |
| +12% | 3% | Régression significative — investiguer |
| -5% | 8% | Impossible de conclure — stabiliser l'environnement d'abord |

### Comparaison avec contexte de taille

Les résultats paramétrés permettent de vérifier que le changement est **cohérent** à travers les tailles de données. Dans l'exemple ci-dessus, le tri est amélioré de 15-21% sur toutes les tailles — c'est un signal fort. Si l'amélioration n'apparaissait que pour N=1000 mais pas pour N=100000, cela suggérerait un effet de cache plutôt qu'une amélioration algorithmique.

À l'inverse, `BM_MapLookup` montre des changements inférieurs à 1% sur toutes les tailles — c'est clairement du bruit. Aucune action n'est nécessaire.

## Analyser la complexité algorithmique

Google Benchmark peut déduire automatiquement la complexité algorithmique d'un benchmark paramétré, en ajustant une courbe sur les données mesurées :

```cpp
BENCHMARK(BM_Sort)
    ->RangeMultiplier(2)
    ->Range(1 << 10, 1 << 20)
    ->Complexity();  // Demande l'analyse de complexité
```

La sortie inclut alors une ligne de complexité estimée :

```
BM_Sort/1024              3.12 us  
BM_Sort/2048              6.89 us  
BM_Sort/4096             15.1  us  
...
BM_Sort/1048576          18.4  ms  
BM_Sort_BigO             17.21 NlgN  
BM_Sort_RMS                  2 %  
```

**`BigO`** indique la complexité estimée — ici O(N log N), conforme à la complexité théorique de `std::sort`. **`RMS`** (*Root Mean Square* error) indique la qualité de l'ajustement : un RMS inférieur à 5% signifie que le modèle de complexité colle bien aux données mesurées.

On peut aussi spécifier la complexité attendue pour vérifier qu'elle correspond :

```cpp
BENCHMARK(BM_Sort)
    ->RangeMultiplier(2)
    ->Range(1 << 10, 1 << 20)
    ->Complexity(benchmark::oNLogN);
```

Si les données mesurées ne correspondent pas à O(N log N), le RMS sera élevé, signalant un problème — peut-être un algorithme dégénéré sur certaines entrées, ou un effet de cache qui modifie le comportement asymptotique pour les petites tailles.

Les complexités disponibles sont `benchmark::o1`, `benchmark::oN`, `benchmark::oNSquared`, `benchmark::oNCubed`, `benchmark::oNLogN` et `benchmark::oLogN`. Pour une complexité non standard, un lambda peut être passé :

```cpp
->Complexity([](benchmark::IterationCount n) { return n * std::sqrt(n); })
```

## Visualiser les résultats

Les chiffres textuels sont utiles pour la CI et les comparaisons ponctuelles. Pour l'analyse approfondie — comprendre les tendances, repérer les anomalies, présenter des résultats à l'équipe — la visualisation graphique est bien plus efficace.

### Générer des graphiques depuis le JSON

Le fichier JSON produit par Google Benchmark contient toutes les données nécessaires. Un script Python avec matplotlib transforme ces données en graphiques :

```python
#!/usr/bin/env python3
# plot_benchmark.py — Visualisation des résultats de benchmark
import json  
import matplotlib.pyplot as plt  
import sys  

def load_benchmarks(filepath):
    with open(filepath) as f:
        data = json.load(f)
    results = {}
    for b in data["benchmarks"]:
        if "aggregate_name" in b:
            continue  # Ignorer les agrégats
        name = b["run_name"]
        base = name.split("/")[0]
        param = int(name.split("/")[1]) if "/" in name else 0
        results.setdefault(base, []).append((param, b["real_time"]))
    return results

results = load_benchmarks(sys.argv[1])

plt.figure(figsize=(10, 6))  
for name, data in sorted(results.items()):  
    data.sort()
    params, times = zip(*data)
    plt.plot(params, times, "o-", label=name)

plt.xlabel("N (taille des données)")  
plt.ylabel("Temps (ns)")  
plt.xscale("log")  
plt.yscale("log")  
plt.legend()  
plt.grid(True, alpha=0.3)  
plt.title("Benchmark Results")  
plt.tight_layout()  
plt.savefig("benchmark_plot.png", dpi=150)  
plt.show()  
```

```bash
python3 plot_benchmark.py results.json
```

L'échelle logarithmique sur les deux axes est essentielle pour les benchmarks paramétrés : un algorithme O(N) apparaît comme une droite de pente 1, O(N²) comme une droite de pente 2, O(N log N) comme une courbe légèrement au-dessus de la pente 1. Les déviations par rapport à la droite théorique révèlent les effets de cache (inflexion quand les données dépassent la taille du L3) ou les comportements dégénérés.

### Comparer visuellement deux versions

Le même script peut superposer les résultats de deux runs :

```python
baseline = load_benchmarks("baseline.json")  
feature = load_benchmarks("feature.json")  

fig, ax = plt.subplots(figsize=(10, 6))  
for name in baseline:  
    if name in feature:
        bp, bt = zip(*sorted(baseline[name]))
        fp, ft = zip(*sorted(feature[name]))
        ax.plot(bp, bt, "o--", alpha=0.5, label=f"{name} (baseline)")
        ax.plot(fp, ft, "s-", label=f"{name} (feature)")

ax.set_xlabel("N")  
ax.set_ylabel("Temps (ns)")  
ax.set_xscale("log")  
ax.set_yscale("log")  
ax.legend()  
ax.grid(True, alpha=0.3)  
plt.tight_layout()  
plt.savefig("comparison.png", dpi=150)  
```

La superposition visuelle rend immédiatement apparent si l'amélioration est uniforme (courbes parallèles décalées), si elle apparaît à partir d'une certaine taille (divergence à droite), ou si elle est illusoire (courbes confondues dans le bruit).

## Pièges d'interprétation

### Le piège du micro-benchmark non représentatif

Un benchmark montre que `std::flat_map` (section 14.4) est 3× plus rapide que `std::map` pour les lookups sur 100 éléments. L'équipe remplace tous les `std::map` du projet par des `std::flat_map`. Résultat en production : aucune amélioration mesurable, voire une légère régression.

Que s'est-il passé ? Le micro-benchmark mesure un lookup isolé sur des données chaudes en cache. En production, les maps sont entrecoupées d'autres opérations qui évincent les données du cache, les insertions sont fréquentes (là où `flat_map` est O(N) contre O(log N) pour `map`), et les maps contiennent des milliers d'éléments (pas cent).

Le micro-benchmark n'avait pas tort — `flat_map` est bien plus rapide pour les lookups sur de petites collections chaudes. Mais le scénario mesuré ne correspondait pas au scénario réel. La leçon : un micro-benchmark répond à une question précise et étroite. Extrapoler ses résultats à un contexte différent est une erreur d'interprétation, pas une erreur de mesure.

### Le piège de l'optimisation insignifiante

Un benchmark montre qu'une optimisation de la fonction `validate_input` la rend 40% plus rapide — un résultat impressionnant. Mais `perf` (section 31.1) révèle que `validate_input` représente 0,3% du temps total d'exécution du programme. L'optimisation divise ce 0,3% par 1,4, ce qui produit une amélioration globale de 0,09% — imperceptible et sans valeur pratique.

La loi d'Amdahl formalise cette limitation : le speedup maximal d'un programme est limité par la fraction du temps qui n'est pas améliorée. Si 99,7% du temps est ailleurs, optimiser les 0,3% restants ne changera rien de visible, quelle que soit l'amélioration locale.

La règle pratique : avant d'optimiser, profilez. Identifiez les fonctions qui consomment le plus de temps CPU (les *hotspots*) avec `perf record` et `perf report` (section 31.1). Concentrez les efforts de benchmarking et d'optimisation sur ces fonctions — c'est là que les gains micro se traduisent en gains macro.

### Le piège de la moyenne trompeuse

Un benchmark affiche un temps moyen de 50 microsecondes, ce qui semble acceptable. Mais l'examen des itérations individuelles révèle une distribution bimodale : 95% des itérations à 30 µs et 5% à 430 µs. Le percentile 99 est à 430 µs — presque 10× la moyenne.

Pour les systèmes sensibles à la latence (trading haute fréquence, serveurs temps réel, pipelines audio), les percentiles élevés (P95, P99, P99.9) sont souvent plus importants que la moyenne. Google Benchmark ne rapporte pas directement les percentiles, mais le fichier JSON contient les données brutes nécessaires au calcul.

Si votre domaine est sensible à la latence tail, envisagez de compléter Google Benchmark par des histogrammes de latence construits en post-traitement sur les données JSON, ou par des outils de macro-benchmarking qui rapportent nativement les percentiles.

### Le piège de la régression masquée

Un benchmark de la suite passe de 100 µs à 95 µs — une amélioration de 5%. L'équipe valide le changement. Mais un autre benchmark, non inclus dans la suite, a régressé de 100 µs à 200 µs. Le bilan net est une régression que personne n'a détectée parce que le benchmark concerné n'existait pas.

La couverture de benchmarks — quels chemins critiques sont mesurés — est aussi importante que la couverture de tests. Les benchmarks doivent couvrir les opérations les plus fréquentes et les plus sensibles à la performance, identifiées par le profiling de production.

## Du benchmark à la décision

Un benchmark produit des chiffres. Une décision d'ingénierie nécessite un raisonnement. Voici le cadre d'analyse recommandé pour transformer des résultats de benchmark en action.

**1. Le changement est-il réel ?** Vérifier que le changement dépasse le bruit (> 2× le CV). Si le CV est élevé, stabiliser l'environnement et remesurer avant de conclure.

**2. Le changement est-il pertinent ?** Un speedup de 2% sur une opération de 10 nanosecondes économise 0,2 nanoseconde. Si cette opération est appelée 10 milliards de fois par jour, c'est 2 secondes économisées — possiblement significatif. Si elle est appelée 1000 fois, c'est 200 nanosecondes — insignifiant. Le contexte d'usage détermine la pertinence.

**3. Le changement est-il représentatif ?** Le micro-benchmark reflète-t-il le scénario de production ? Les données sont-elles de la bonne taille ? Le pattern d'accès est-il réaliste ? Le cache est-il dans un état comparable ?

**4. Y a-t-il des compromis ?** Une optimisation qui accélère le cas nominal mais ralentit le cas d'erreur, qui améliore le débit mais dégrade la latence, ou qui gagne en performance mais perd en lisibilité du code, n'est pas nécessairement un gain net. Le benchmark ne mesure qu'une dimension — la décision doit en considérer plusieurs.

**5. Le gain justifie-t-il le coût ?** Trois jours d'optimisation pour gagner 5% sur une fonction qui représente 15% du temps total (soit 0,75% de gain global) doivent être pesés contre trois jours passés sur d'autres priorités — nouvelles fonctionnalités, réduction de la dette technique, tests supplémentaires.

Ce cadre n'interdit aucune optimisation — il exige simplement que chaque décision soit fondée sur des données (le benchmark), un contexte (le profiling, le scénario d'usage) et un raisonnement (le rapport coût/bénéfice). C'est la différence entre l'ingénierie de la performance et l'optimisation aveugle.

---


⏭️ [PARTIE V : DEVOPS ET CLOUD NATIVE](/partie-05-devops-cloud-native.md)
