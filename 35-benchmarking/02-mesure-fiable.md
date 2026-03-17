🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 35.2 Mesure de performance fiable

## Le problème du bruit

Un micro-benchmark exécuté deux fois de suite sur la même machine, avec le même binaire et les mêmes données, produit rarement le même résultat. L'écart est parfois de 1-2%, parfois de 15-20%. Cette variabilité n'est pas un bug de l'outil de mesure — c'est le reflet de la réalité d'un processeur moderne, d'un système d'exploitation multi-tâches et d'un environnement matériel partagé.

Le problème est qu'un écart de performance entre deux versions du code n'a de sens que s'il dépasse le bruit ambiant. Si la variabilité naturelle de la mesure est de ±5%, un speedup mesuré de 3% est indistinguable du bruit — il pourrait être une amélioration, une dégradation ou rien du tout. Un benchmark fiable est un benchmark dont le bruit est suffisamment faible pour que les différences réelles émergent clairement du signal.

Cette section couvre les sources de bruit, les techniques pour les réduire et les méthodes statistiques pour distinguer le signal du bruit.

## Sources de variabilité

Comprendre les sources de bruit est le préalable à leur réduction. Elles se classent en trois catégories.

### Variabilité du CPU

**Frequency scaling.** Les processeurs modernes ajustent dynamiquement leur fréquence en fonction de la charge et de la température. Un benchmark peut démarrer à 4.2 GHz en turbo boost et descendre à 3.6 GHz après quelques secondes de calcul intensif à cause du throttling thermique. Le même code s'exécute alors 15% plus lentement — non pas parce qu'il a changé, mais parce que le CPU a ralenti.

**État du cache.** Les caches L1/L2/L3 (section 41.1) ont un impact massif sur la performance. Un benchmark précédé d'une opération qui charge le cache avec des données utiles sera plus rapide qu'un benchmark exécuté "à froid". Google Benchmark effectue un warmup automatique, mais les itérations avec `PauseTiming` qui recréent des données à chaque tour réintroduisent un cold start partiel.

**Branch prediction.** Le prédicteur de branches du CPU apprend les patterns au fil des itérations. Les premières exécutions d'une boucle avec des branchements sont plus lentes que les suivantes. Sur des opérations très courtes (quelques nanosecondes), cet effet d'apprentissage peut représenter un biais significatif.

**Hyperthreading.** Sur les processeurs avec SMT (*Simultaneous Multithreading*), deux threads logiques partagent les ressources d'un cœur physique. Un benchmark single-thread qui s'exécute sur un cœur dont l'autre thread est occupé par un processus système sera plus lent qu'un benchmark sur un cœur entièrement libre.

### Variabilité du système d'exploitation

**Scheduling.** Le scheduler Linux peut préempter le processus de benchmark pour exécuter un autre processus, introduisant un délai qui s'ajoute au temps mesuré. Sur un système chargé, ces interruptions peuvent se produire plusieurs fois par milliseconde.

**Interruptions matérielles.** Les IRQ (réseau, disque, timer) interrompent brièvement l'exécution du benchmark. Sur un serveur avec du trafic réseau, ces interruptions sont fréquentes et ajoutent du bruit.

**Gestion mémoire.** Les page faults (accès à une page non encore mappée), le compactage mémoire et le swap introduisent des latences sporadiques qui peuvent multiplier le temps d'une itération par 100 ou plus.

**ASLR.** L'*Address Space Layout Randomization* (section 45.4.3) modifie l'agencement mémoire du programme à chaque exécution. Deux exécutions du même binaire peuvent avoir des performances de cache différentes simplement parce que le code et les données tombent sur des lignes de cache différentes. L'impact est généralement faible (1-2%) mais mesurable.

### Variabilité de l'environnement

**Processus concurrents.** Une compilation en arrière-plan, un navigateur ouvert, un serveur de base de données — tout processus actif consomme du CPU, de la mémoire et de la bande passante cache, ce qui dégrade les mesures.

**Température ambiante.** Sur un ordinateur portable sans ventilation suffisante, le throttling thermique peut dégrader les performances de 20-30% au fil du temps. Ce facteur est souvent sous-estimé.

**Machines virtuelles et conteneurs.** En CI, les benchmarks s'exécutent souvent dans des conteneurs sur des machines partagées. La variabilité est structurellement plus élevée — les voisins consomment des ressources de manière imprévisible.

## Réduire le bruit : préparation de l'environnement

### Stabiliser la fréquence CPU

La première mesure et la plus impactante est de désactiver le frequency scaling et de fixer la fréquence CPU :

```bash
# Vérifier le gouverneur actuel
cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor

# Fixer le gouverneur en mode performance (fréquence maximale constante)
sudo cpupower frequency-set -g performance

# Vérifier que tous les cœurs sont en mode performance
cpupower frequency-info
```

Le gouverneur `performance` verrouille le CPU à sa fréquence maximale non-turbo, éliminant les fluctuations dues au scaling dynamique. Après les benchmarks, restaurez le gouverneur par défaut :

```bash
sudo cpupower frequency-set -g powersave
```

> 📝 **Note.** `cpupower` s'installe via `sudo apt install linux-tools-common linux-tools-$(uname -r)`. Sur certaines machines, le paquet s'appelle `linux-cpupower`.

### Désactiver le turbo boost

Le turbo boost amplifie la fréquence au-delà de la base quand les conditions thermiques le permettent, mais cette amplification est instable — elle fluctue en fonction de la température et du nombre de cœurs actifs :

```bash
# Intel
echo 1 | sudo tee /sys/devices/system/cpu/intel_pstate/no_turbo

# AMD
echo 0 | sudo tee /sys/devices/system/cpu/cpufreq/boost
```

Désactiver le turbo boost réduit la performance absolue mais **améliore la stabilité des mesures** — ce qui est exactement ce qu'on recherche en benchmarking.

### Isoler les cœurs CPU

Pour les benchmarks les plus exigeants, Linux permet de réserver des cœurs CPU exclusivement au benchmark, empêchant le scheduler d'y placer d'autres processus :

```bash
# Réserver les cœurs 2 et 3 au benchmark
sudo cset shield -c 2,3 -k on

# Exécuter le benchmark sur les cœurs isolés
sudo cset shield --exec -- ./mon_projet_benchmarks

# Libérer les cœurs
sudo cset shield --reset
```

`cset` (paquet `cpuset`) crée un *cpuset* protégé. Les processus système et les interruptions sont migés sur les autres cœurs, laissant les cœurs isolés entièrement au benchmark.

Une alternative plus légère utilise `taskset` pour épingler le processus sur un cœur spécifique :

```bash
# Épingler sur le cœur 2
taskset -c 2 ./mon_projet_benchmarks
```

Cela ne protège pas contre les autres processus assignés au même cœur, mais réduit les migrations de cœur à cœur (qui invalident le cache L1/L2).

### Réduire l'activité système

Avant un benchmark critique, minimisez les sources de bruit :

```bash
# Arrêter les services non essentiels
sudo systemctl stop cron  
sudo systemctl stop unattended-upgrades  
sudo systemctl stop snapd  

# Fermer les applications graphiques (si applicable)
# Désactiver le swap (si suffisamment de RAM)
sudo swapoff -a

# Vider les caches du système de fichiers (si I/O benchmarks)
sync && echo 3 | sudo tee /proc/sys/vm/drop_caches
```

Sur une machine de développement, ces mesures sont temporaires. Sur un serveur de benchmarking dédié, elles peuvent être configurées de manière permanente.

## Réduire le bruit : techniques de mesure

### Répétitions et statistiques

La technique la plus accessible est d'exécuter le benchmark plusieurs fois et d'utiliser les statistiques pour qualifier la fiabilité :

```bash
./mon_projet_benchmarks --benchmark_repetitions=10 \
                         --benchmark_report_aggregates_only=true
```

Google Benchmark calcule alors pour chaque benchmark la moyenne (*mean*), la médiane (*median*), l'écart-type (*stddev*) et le coefficient de variation (*cv*) :

```
BM_VectorSort/10000_mean       62.4 us  
BM_VectorSort/10000_median     61.8 us  
BM_VectorSort/10000_stddev      2.1 us  
BM_VectorSort/10000_cv         3.37 %  
```

Le **coefficient de variation** (cv = stddev / mean × 100) est le chiffre clé. Il exprime la dispersion relative des mesures :

| CV | Interprétation | Action |
|----|----------------|--------|
| < 1% | Excellent — mesure très stable | Résultats fiables sans réserve |
| 1-3% | Bon — bruit limité | Résultats exploitables |
| 3-5% | Acceptable — bruit modéré | Seuls les changements > 5% sont significatifs |
| 5-10% | Médiocre — bruit élevé | Stabiliser l'environnement avant de conclure |
| > 10% | Inexploitable | Les résultats ne signifient rien |

Quand le CV dépasse 5%, le problème n'est pas le nombre de répétitions — c'est l'environnement. Ajouter des répétitions réduira l'erreur sur la moyenne mais ne résoudra pas l'instabilité sous-jacente.

### Minimum vs moyenne

Dans un environnement bruité, la **valeur minimale** est souvent plus informative que la moyenne. Le raisonnement est le suivant : le bruit introduit presque toujours des ralentissements (context switch, cache miss, throttling) mais jamais des accélérations inexpliquées. Le minimum représente donc l'exécution la moins perturbée — la plus proche de la performance "vraie" du code.

Google Benchmark ne rapporte pas directement le minimum dans ses agrégats, mais le fichier JSON de sortie contient toutes les itérations individuelles, ce qui permet d'extraire le minimum en post-traitement :

```bash
# Extraire les temps minimaux depuis le JSON
python3 -c "  
import json, sys  
data = json.load(open('results.json'))  
for b in data['benchmarks']:  
    if b.get('aggregate_name') == 'mean':
        continue
    print(f\"{b['name']}: {b['real_time']:.1f} {b['time_unit']}\")
" | sort
```

### Durée minimale de benchmark

Google Benchmark ajuste automatiquement le nombre d'itérations pour atteindre une durée minimale par défaut (500ms). Pour les opérations très rapides où la résolution de l'horloge devient un facteur, augmenter ce minimum améliore la stabilité :

```cpp
BENCHMARK(BM_TinyOperation)->MinTime(2.0);  // Au moins 2 secondes
```

L'option globale `--benchmark_min_time=2s` applique ce minimum à tous les benchmarks. Un temps plus long donne au framework plus d'itérations pour lisser le bruit, au prix d'une exécution totale plus longue.

## Script de benchmarking stabilisé

En combinant les techniques de préparation et de mesure, voici un script complet pour une session de benchmarking fiable :

```bash
#!/bin/bash
# bench_stable.sh — Exécution de benchmarks avec environnement stabilisé
set -euo pipefail

BENCHMARK_BIN="${1:?Usage: bench_stable.sh <benchmark_binary>}"  
OUTPUT_JSON="${2:-results.json}"  
REPS="${3:-10}"  
CPU_CORE="${4:-2}"  

echo "=== Stabilisation de l'environnement ==="

# Vérifier les droits root pour les optimisations système
if [ "$EUID" -ne 0 ]; then
    echo "⚠️  Exécution sans root — certaines optimisations ignorées"
    echo "    Relancez avec sudo pour une stabilisation complète"
else
    # Fixer la fréquence CPU
    cpupower frequency-set -g performance 2>/dev/null || true

    # Désactiver le turbo boost (Intel)
    echo 1 > /sys/devices/system/cpu/intel_pstate/no_turbo 2>/dev/null || true
fi

echo "=== Exécution des benchmarks ==="  
echo "Binaire   : $BENCHMARK_BIN"  
echo "Cœur CPU  : $CPU_CORE"  
echo "Répétitions : $REPS"  
echo "Sortie    : $OUTPUT_JSON"  

taskset -c "$CPU_CORE" "$BENCHMARK_BIN" \
    --benchmark_repetitions="$REPS" \
    --benchmark_out="$OUTPUT_JSON" \
    --benchmark_out_format=json \
    --benchmark_report_aggregates_only=true

echo ""  
echo "=== Vérification de la stabilité ==="  
python3 -c "  
import json  
data = json.load(open('$OUTPUT_JSON'))  
unstable = []  
for b in data['benchmarks']:  
    if b.get('aggregate_name') == 'cv':
        if b['real_time'] > 5.0:
            unstable.append((b['run_name'], b['real_time']))
if unstable:
    print('⚠️  Benchmarks instables (CV > 5%) :')
    for name, cv in unstable:
        print(f'   {name}: CV = {cv:.1f}%')
    print('   → Stabiliser l\'environnement avant d\'interpréter les résultats')
else:
    print('✅ Tous les benchmarks sont stables (CV < 5%)')
"

# Restaurer les paramètres CPU si root
if [ "$EUID" -eq 0 ]; then
    cpupower frequency-set -g powersave 2>/dev/null || true
    echo 0 > /sys/devices/system/cpu/intel_pstate/no_turbo 2>/dev/null || true
fi

echo ""  
echo "Rapport JSON : $OUTPUT_JSON"  
```

Le script stabilise l'environnement, exécute les benchmarks épinglés sur un cœur dédié avec 10 répétitions, et vérifie automatiquement que le coefficient de variation reste sous 5%. C'est le workflow recommandé pour toute mesure de performance sérieuse.

## Cas particulier : benchmarking en CI

Les environnements CI posent un défi structurel pour le benchmarking. Les machines sont partagées, virtualisées, et leur charge varie d'une exécution à l'autre. Les temps absolus sont donc peu fiables. Plusieurs stratégies atténuent ce problème.

### Comparaison relative plutôt qu'absolue

Au lieu de vérifier "cette fonction prend moins de 50 microsecondes" (seuil absolu qui dépend de la machine), comparez deux versions du même benchmark sur la même machine et dans le même pipeline :

```bash
# Checkout de la branche main, build et benchmark
git checkout main  
cmake --build build-bench  
./build-bench/benchmarks --benchmark_out=baseline.json --benchmark_out_format=json

# Checkout de la branche feature, build et benchmark
git checkout feature/optimize-sort  
cmake --build build-bench  
./build-bench/benchmarks --benchmark_out=feature.json --benchmark_out_format=json

# Comparaison relative
python3 tools/compare.py benchmarks baseline.json feature.json
```

La comparaison relative annule une grande partie du bruit environnemental — si la machine est lente, les deux runs sont lents de manière similaire.

### Seuils de régression tolérants

En CI, les seuils de détection de régression doivent être plus larges qu'en local pour absorber la variabilité. Un seuil de 10-15% est raisonnable en environnement partagé, contre 3-5% sur une machine stabilisée.

### Runners dédiés

La solution idéale est de disposer d'un runner CI dédié au benchmarking — une machine bare-metal, non partagée, avec le frequency scaling fixé en permanence. Les grands projets open source (Chromium, LLVM) utilisent cette approche. Pour les équipes plus modestes, un vieux serveur dédié au bench est un investissement modeste aux bénéfices significatifs.

## Checklist du benchmarking fiable

Avant de publier ou d'agir sur des résultats de benchmark, vérifiez les points suivants :

**Compilation.** Le benchmark est compilé en mode Release (`-O2` ou `-O3`), avec le même compilateur et les mêmes flags que la production. Les optimisations link-time (LTO, section 41.5) sont activées si elles le sont en production.

**Environnement.** Le frequency scaling est fixé, le turbo boost est désactivé (ou au minimum documenté), les processus concurrents sont minimisés, le benchmark est épinglé sur un cœur.

**Statistiques.** Le benchmark est exécuté avec au moins 5 répétitions. Le coefficient de variation est inférieur à 5%. Les résultats avec un CV > 5% sont signalés comme instables.

**Anti-optimisation.** Chaque benchmark utilise `DoNotOptimize` et/ou `ClobberMemory` pour empêcher le compilateur d'éliminer le code mesuré. Les temps anormalement bas (< 1ns) sont suspects.

**Isolation.** Le setup (préparation des données) est séparé de la mesure. `PauseTiming/ResumeTiming` est utilisé avec parcimonie et son surcoût est pris en compte.

**Reproductibilité.** Les résultats sont exportés en JSON avec les métadonnées de l'environnement. Un collègue doit pouvoir reproduire la mesure avec les mêmes instructions.

---


⏭️ [Interprétation des résultats](/35-benchmarking/03-interpretation-resultats.md)
