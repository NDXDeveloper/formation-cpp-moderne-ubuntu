🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 41.2 — Branch prediction et optimisation des conditions

> **Chapitre 41 : Optimisation CPU et Mémoire** · Section 2  
> **Niveau** : Expert · **Prérequis** : Section 41.1 (cache CPU), Chapitre 31 (profiling avec perf)

---

## Introduction

La section 41.1 a traité le premier grand goulot d'étranglement des processeurs modernes : la latence mémoire. Cette section aborde le second : le **coût des branchements conditionnels** dans un processeur à pipeline.

Chaque `if`, `else`, `switch`, opérateur ternaire `? :`, et même chaque condition de sortie de boucle (`for`, `while`) est un *branchement* au niveau du code machine. Le processeur doit décider quelle instruction exécuter ensuite — mais sur un CPU pipeliné, cette décision doit être prise **avant** que la condition ne soit évaluée. Le processeur *parie* donc sur l'issue du branchement. S'il se trompe, tout le travail spéculatif est jeté, et le pipeline est purgé.

Ce pari porte un nom : la ***branch prediction***. Et quand le pari échoue — un ***branch misprediction*** — le coût est de **10 à 20 cycles** sur les architectures modernes. Cela semble modeste comparé aux 50–100 ns d'un cache miss RAM, mais dans une boucle serrée exécutée des millions de fois, les mispredictions s'accumulent et deviennent un facteur de performance de premier ordre.

---

## Le pipeline du processeur : pourquoi les branchements posent problème

### Le pipeline en bref

Un processeur moderne ne traite pas les instructions une par une de manière séquentielle. Il utilise un ***pipeline*** — une chaîne de traitement où chaque étape s'occupe d'une phase d'exécution différente. Schématiquement, sur un pipeline simplifié :

```
Cycle :     1     2     3     4     5     6     7     8
          ┌─────┬──────┬─────┬─────┐
Instr A : │Fetch│Decode│Exec │Write│
          └─────┴──────┴─────┴─────┘
                ┌─────┬──────┬─────┬─────┐
Instr B :       │Fetch│Decode│Exec │Write│
                └─────┴──────┴─────┴─────┘
                      ┌─────┬──────┬─────┬─────┐
Instr C :             │Fetch│Decode│Exec │Write│
                      └─────┴──────┴─────┴─────┘
                            ┌─────┬──────┬─────┬─────┐
Instr D :                   │Fetch│Decode│Exec │Write│
                            └─────┴──────┴─────┴─────┘
```

À chaque cycle, une nouvelle instruction entre dans le pipeline. En régime de croisière, le processeur achève une instruction par cycle, même si chaque instruction prend 4 cycles individuellement. Les processeurs modernes ont des pipelines de **14 à 20+ étages** et peuvent traiter 4 à 6 instructions par cycle (*superscalar*).

### Le problème du branchement

Quand le pipeline rencontre un `if`, l'instruction de branchement conditionnel est au stade *Fetch* au cycle N. Mais la condition ne sera évaluée qu'au stade *Execute*, soit environ 10–15 cycles plus tard. Pendant ce temps, le pipeline doit continuer à charger des instructions — mais *lesquelles* ? Celles du bloc `then` ? Ou celles du bloc `else` ?

```
Cycle :     1     2     3     ...    12    13
          ┌─────┬──────┬─────  ───┬─────┬─────┐
Branch :  │Fetch│Decode│  ...     │Exec │     │ ← condition évaluée ici
          └─────┴──────┴─────  ───┴─────┴─────┘
                ┌─────┬─────  ───┐
  ???   :       │Fetch│Decode│...│  ← quelle instruction charger ?
                └─────┴─────  ───┘
```

Trois options s'offrent au processeur :

1. **Stall** : arrêter le pipeline et attendre que la condition soit évaluée. Correct, mais on perd 10–15 cycles à chaque branchement — catastrophique pour les performances.
2. **Toujours prédire une direction** (par exemple : toujours *taken*). Simple, mais souvent faux.
3. **Prédire intelligemment** en se basant sur l'historique des branchements passés. C'est l'approche des processeurs modernes.

---

## Le prédicteur de branchement

### Fonctionnement

Le ***Branch Prediction Unit*** (BPU) est un composant matériel dédié qui maintient un historique des branchements récents et prédit leur issue. Lorsqu'un branchement est rencontré :

1. Le BPU consulte son historique pour ce branchement (identifié par son adresse dans le code).
2. Il prédit *taken* (branchement pris, on saute) ou *not taken* (on continue séquentiellement).
3. Le pipeline charge spéculativement les instructions correspondant à la prédiction.
4. Quand la condition est effectivement évaluée :
   - **Prédiction correcte** : le travail spéculatif est validé. Aucun coût. Le programme continue à pleine vitesse.  
   - **Prédiction incorrecte** (*misprediction*) : tout le travail spéculatif est jeté. Le pipeline est purgé (*flush*) et reprend depuis la bonne branche. Coût : **12–20 cycles perdus** selon l'architecture.

### Types de prédicteurs

Les prédicteurs modernes sont sophistiqués :

**Prédicteur à compteur 2 bits.** Le mécanisme de base. Chaque branchement est associé à un compteur de 2 bits (*strongly not taken*, *weakly not taken*, *weakly taken*, *strongly taken*). Il faut deux mauvaises prédictions consécutives pour changer de direction — ce qui évite les oscillations sur les branchements sporadiques.

```
   Strongly     Weakly      Weakly     Strongly
  Not Taken ←→ Not Taken ←→  Taken  ←→  Taken
     00           01           10          11
       predict: NOT TAKEN     predict: TAKEN
```

**Prédicteur corrélé / TAGE.** Les processeurs actuels (Intel depuis Haswell, AMD depuis Zen) utilisent des prédicteurs ***TAGE*** (*TAgged GEometric history length*) qui corrèlent le comportement d'un branchement avec l'historique des N branchements précédents, à plusieurs profondeurs d'historique. Cela permet de prédire des patterns complexes comme « ce branchement est pris une fois sur trois ».

**Prédicteur de boucle.** Un circuit spécialisé détecte les boucles à compteur fixe (`for (int i = 0; i < N; ++i)`) et prédit correctement la dernière itération (sortie de boucle).

### Taux de prédiction

Sur du code « typique » (applications de bureau, serveurs web, compilateurs), les prédicteurs modernes atteignent un taux de **95–99 %** de prédictions correctes. C'est remarquable — mais les 1–5 % restants peuvent représenter des millions de mispredictions par seconde dans une boucle chaude.

---

## Mesurer les mispredictions

### Avec `perf stat`

```bash
perf stat -e branches,branch-misses ./mon_programme
```

Sortie typique :

```
 2 450 000 000  branches
    38 000 000  branch-misses     # 1,55% de tous les branchements
```

Un taux de misprediction de **1,55 %** semble faible, mais sur un CPU à 4 GHz avec une pénalité de 15 cycles par misprediction, cela représente :

```
38 000 000 × 15 cycles = 570 000 000 cycles perdus
÷ 4 000 000 000 cycles/sec ≈ 0,14 seconde
```

Plus d'un dixième de seconde de temps CPU pur gaspillé en mispredictions.

### Identifier les branchements problématiques

`perf record` avec l'événement `branch-misses` localise les branchements les plus coûteux :

```bash
perf record -e branch-misses ./mon_programme  
perf report  
```

La sortie montre les fonctions et les lignes de code responsables de la majorité des mispredictions. C'est le point de départ pour toute optimisation.

### Avec `perf annotate`

Pour voir le détail instruction par instruction :

```bash
perf annotate -s my_function
```

Cette commande affiche le code assembleur de la fonction, avec le nombre de mispredictions par instruction de branchement. Elle révèle précisément *quel* `if` ou *quelle* condition de boucle est mal prédite.

---

## Patterns de branchement et leur prédictibilité

Tous les branchements ne se valent pas du point de vue du prédicteur. Le facteur clé est la **prévisibilité du pattern**.

### Pattern parfaitement prévisible

```cpp
// Le branchement est TOUJOURS taken (ou presque) → 100% prédictible
for (int i = 0; i < 1'000'000; ++i) {
    sum += data[i];  // la condition de boucle est prise 999 999 fois sur 1 000 000
}
```

Le prédicteur de boucle gère ce cas parfaitement. Seule la dernière itération (sortie de boucle) génère une misprediction — soit 1 misprediction pour 1 000 000 itérations.

### Pattern biaisé mais prévisible

```cpp
// 99% des valeurs satisfont la condition → le prédicteur prédit "taken"
for (auto val : data) {
    if (val > 0)      // vrai dans 99% des cas
        sum += val;
}
```

Le prédicteur apprend rapidement que ce branchement est presque toujours pris. Taux de misprediction : ~1 %. Très bon.

### Pattern aléatoire — le pire cas

```cpp
// 50/50 aléatoire → impossible à prédire
std::mt19937 gen(42);  
std::uniform_int_distribution<int> dist(0, 1);  

for (int i = 0; i < N; ++i) {
    if (dist(gen))    // vrai 50% du temps, de manière aléatoire
        sum += data[i];
}
```

Le prédicteur ne peut pas faire mieux que le hasard : taux de misprediction **~50 %**. Chaque misprediction coûte 15 cycles. Si la boucle interne fait normalement 5 cycles, la misprediction *triple* le temps d'exécution moyen.

### Le benchmark célèbre : tri et prédiction

L'un des exemples les plus célèbres de l'impact de la branch prediction est le tri préalable des données avant un filtrage conditionnel :

```cpp
#include <algorithm>
#include <cstdint>
#include <random>
#include <vector>

volatile std::int64_t sink;

void conditional_sum(std::vector<int>& data) {
    std::int64_t sum = 0;
    for (auto val : data) {
        if (val >= 128)    // filtre : ne garde que les valeurs >= 128
            sum += val;
    }
    sink = sum;
}

int main() {
    constexpr int N = 10'000'000;
    std::vector<int> data(N);
    
    std::mt19937 gen(42);
    std::uniform_int_distribution<int> dist(0, 255);
    for (auto& v : data) v = dist(gen);
    
    // Test 1 : données NON triées → branchement imprévisible
    conditional_sum(data);
    
    // Test 2 : données TRIÉES → branchement parfaitement prédictible
    std::sort(data.begin(), data.end());
    conditional_sum(data);
}
```

Résultats typiques (GCC 15, `-O2`, AMD Zen 4) :

| Données | Temps | Branch misses |
|---------|-------|--------------|
| Non triées | 58 ms | ~25 % |
| Triées | 15 ms | ~0,01 % |

Le même code, les mêmes données, le même résultat final — mais **3,9× plus rapide** après un tri. Sur les données non triées, les valeurs autour de 128 provoquent un branchement imprévisible (~50/50 localement). Après tri, les valeurs sont dans l'ordre croissant : la condition est d'abord toujours fausse, puis toujours vraie. Le prédicteur suit sans effort.

> **Note pédagogique** : trier les données uniquement pour améliorer la prédiction de branchement n'est évidemment pas toujours pratique. Cet exemple illustre le *mécanisme* — les solutions réelles sont présentées dans les sections suivantes.

---

## Technique 1 : `[[likely]]` et `[[unlikely]]` (C++20)

C++20 a introduit les attributs `[[likely]]` et `[[unlikely]]` pour indiquer au compilateur quelle branche est la plus probable. Le compilateur peut alors organiser le code machine de manière à favoriser la branche *likely* — en la plaçant dans le chemin séquentiel (*fall-through*), ce qui améliore à la fois la prédiction et la localité du cache d'instructions.

```cpp
int process(int value) {
    if (value > 0) [[likely]] {
        // Chemin normal — le compilateur l'optimise comme fall-through
        return compute_normal(value);
    } else [[unlikely]] {
        // Chemin d'erreur — placé plus loin dans le code, hors du hot path
        return handle_error(value);
    }
}
```

### Effet sur le code généré

Sans `[[likely]]` :

```asm
    cmp     edi, 0
    jle     .Lelse          ; saut conditionnel vers else
    ; ... code du then (fall-through)
    ret
.Lelse:
    ; ... code du else
    ret
```

Avec `[[likely]]` sur le `then` (même code, le compilateur confirme le layout) :

```asm
    cmp     edi, 0
    jle     .Lelse          ; saut vers else (unlikely → loin)
    ; ... code du then (fall-through, dans le hot path)
    ret
    
    ; Section cold — le else est placé plus loin, hors du hot path
    .section .text.unlikely
.Lelse:
    ; ... code du else
    ret
```

Le code `unlikely` peut être déplacé dans une section froide du binaire (`.text.unlikely`), libérant de l'espace dans le cache L1i pour le hot path.

### Impact réel

`[[likely]]` / `[[unlikely]]` n'affecte pas directement le prédicteur matériel — celui-ci apprend de l'exécution réelle, pas des hints du compilateur. L'impact est plutôt sur :

- Le **layout du code** (branche froide déplacée hors du hot path → meilleure localité L1i).  
- Les **heuristiques d'inlining** (le compilateur est plus enclin à inliner le chemin likely).  
- La **vectorisation** (certaines passes d'optimisation utilisent ces hints).

Le gain est typiquement modeste (2–5 %) mais gratuit en termes de lisibilité. À utiliser systématiquement sur les chemins d'erreur et les cas rares.

### Alternatives pré-C++20

Avant C++20, GCC et Clang offrent `__builtin_expect` :

```cpp
// Équivalent de [[likely]] / [[unlikely]] pour GCC/Clang
#define LIKELY(x)   __builtin_expect(!!(x), 1)
#define UNLIKELY(x) __builtin_expect(!!(x), 0)

if (LIKELY(value > 0)) {
    return compute_normal(value);
} else {
    return handle_error(value);
}
```

En C++20+, préférer les attributs standard `[[likely]]` et `[[unlikely]]`.

---

## Technique 2 : programmation branchless

### Principe

L'approche la plus radicale pour éliminer les mispredictions est de **supprimer le branchement**. Au lieu d'un `if` / `else`, on calcule le résultat des deux branches et on sélectionne le bon résultat avec une opération arithmétique ou un `cmov` (*conditional move*) — une instruction qui ne provoque jamais de pipeline flush.

### Somme conditionnelle — version branchless

Reprenons l'exemple du filtre `>= 128` :

```cpp
// Version avec branchement (vulnérable aux mispredictions)
void sum_branched(const std::vector<int>& data) {
    std::int64_t sum = 0;
    for (auto val : data) {
        if (val >= 128)
            sum += val;
    }
    sink = sum;
}

// Version branchless — pas de if
void sum_branchless(const std::vector<int>& data) {
    std::int64_t sum = 0;
    for (auto val : data) {
        // Le masque vaut 0xFFFFFFFF si val >= 128, 0 sinon
        // Pas de branchement — juste de l'arithmétique
        int mask = -(val >= 128);  // conversion bool → 0 ou -1 (all bits set)
        sum += (val & mask);
    }
    sink = sum;
}
```

Le compilateur traduit `-(val >= 128)` en une comparaison suivie d'un `sbb` (*subtract with borrow*) ou un `cmov`, sans branchement conditionnel. Le pipeline n'est jamais interrompu.

Résultats sur les données non triées :

| Version | Temps | Branch misses |
|---------|-------|--------------|
| Branched (non trié) | 58 ms | ~25 % |
| Branchless | 22 ms | ~0 % |
| Branched (trié) | 15 ms | ~0,01 % |

La version branchless est **2,6× plus rapide** que la version branchée sur des données aléatoires, sans nécessiter de tri préalable. Elle est toutefois un peu plus lente que la version branchée sur données triées, car le `cmov` a un coût fixe (il évalue toujours les deux opérandes), alors que le branchement bien prédit a un coût quasi nul.

### L'opérateur ternaire comme hint branchless

Le compilateur peut parfois transformer un opérateur ternaire en `cmov` au lieu d'un branchement :

```cpp
// Le compilateur peut émettre un cmov ici (pas garanti)
sum += (val >= 128) ? val : 0;
```

Mais ce n'est pas garanti — le compilateur décide selon ses heuristiques. Pour forcer un code branchless, la technique du masque arithmétique est plus fiable.

### Min/max branchless

Les fonctions `std::min` et `std::max` sont des candidats classiques pour le branchless. Le compilateur les transforme souvent en `cmov`, mais on peut le vérifier sur Compiler Explorer :

```cpp
// Ces deux appels sont typiquement compilés en cmov (pas de branchement)
int a = std::min(x, y);  
int b = std::max(x, y);  

// Vérifier dans l'assembleur :
//   cmp   edi, esi
//   cmovl eax, esi    ← pas de jmp → branchless ✓
```

### Sélection branchless avec multiplication

Pour sélectionner entre deux valeurs sans branchement :

```cpp
// Sélectionne a si condition est vrai, b sinon — sans if
int select_branchless(bool condition, int a, int b) {
    return a * condition + b * (!condition);
}
```

Le compilateur optimise souvent mieux que cela (via `cmov`), mais le pattern est utile pour les types plus complexes ou les expressions où le compilateur ne peut pas inférer le branchless par lui-même.

---

## Technique 3 : réorganisation des conditions

### Ordre des tests dans un `if` / `else if`

Lorsqu'une chaîne de `if` / `else if` est évaluée, le premier test vrai court-circuite les suivants. Placer le cas le plus fréquent en premier réduit le nombre moyen de branchements évalués — et chaque branchement supplémentaire est une opportunité de misprediction.

```cpp
// Si 90% des requêtes sont GET, 8% POST, 2% autres :

// BON — cas fréquent en premier
if (method == Method::GET) [[likely]] {
    handle_get();
} else if (method == Method::POST) {
    handle_post();
} else {
    handle_other();
}

// MAUVAIS — cas rare en premier
if (method == Method::DELETE) [[unlikely]] {
    handle_delete();
} else if (method == Method::PUT) {
    handle_put();
} else if (method == Method::GET) {    // 90% des cas, mais 2 branchements déjà évalués
    handle_get();
}
```

### Fusion de conditions

Plusieurs conditions imbriquées peuvent parfois être fusionnées en une seule expression, réduisant le nombre total de branchements :

```cpp
// 3 branchements
if (x > 0) {
    if (x < 100) {
        if (y > 0) {
            process(x, y);
        }
    }
}

// 1 seul branchement (le compilateur peut souvent le faire seul avec -O2)
if (x > 0 && x < 100 && y > 0) {
    process(x, y);
}
```

En pratique, les compilateurs fusionnent souvent les conditions simples automatiquement à `-O2`. Mais pour des conditions complexes impliquant des appels de fonctions non-inline, la fusion manuelle reste bénéfique.

---

## Technique 4 : remplacement de `switch` par une table de lookup

Un `switch` avec de nombreux cas génère soit une cascade de comparaisons (si les valeurs sont éparses), soit une table de sauts (si les valeurs sont denses). Dans les deux cas, le prédicteur peut avoir du mal si les cas sont équiprobables.

Une alternative branchless est la **table de lookup** :

```cpp
// Version switch — potentiellement mal prédite si les cas sont équiprobables
int classify_switch(int val) {
    switch (val % 4) {
        case 0: return 10;
        case 1: return 20;
        case 2: return 30;
        case 3: return 40;
        default: return 0;  // impossible, mais le compilateur ne le sait pas
    }
}

// Version lookup table — zéro branchement
int classify_lut(int val) {
    static constexpr int table[] = {10, 20, 30, 40};
    return table[val % 4];   // un seul accès mémoire, pas de branchement
}
```

La lookup table transforme un branchement en un **accès mémoire indexé**. Si la table tient dans le L1d (ce qui est presque toujours le cas pour des tables de quelques dizaines d'entrées), l'accès coûte ~1 ns — bien moins que le coût moyen d'un branchement mal prédit.

Ce pattern s'applique aussi aux machines à états, aux décodeurs de protocoles, et aux dispatchers de commandes.

### Lookup table pour des fonctions (function dispatch)

```cpp
using Handler = void(*)(int);

void handle_read(int fd);  
void handle_write(int fd);  
void handle_close(int fd);  
void handle_error(int fd);  

// Dispatch sans branchement
constexpr Handler dispatch_table[] = {
    handle_read,    // EventType::READ  = 0
    handle_write,   // EventType::WRITE = 1
    handle_close,   // EventType::CLOSE = 2
    handle_error,   // EventType::ERROR = 3
};

void dispatch_event(int event_type, int fd) {
    dispatch_table[event_type](fd);  // appel indirect, pas de branchement
}
```

L'appel indirect via pointeur de fonction a son propre coût (prédiction de cible indirecte), mais il est généralement inférieur à une cascade de `if` / `else if` pour plus de 3–4 cas.

---

## Technique 5 : partitionnement des données (tri partiel)

Le tri complet des données (vu plus haut) est souvent impraticable en production. Une alternative plus légère est le **partitionnement** : séparer les données en deux groupes (celles qui satisfont la condition et celles qui ne la satisfont pas) avant la boucle de traitement.

```cpp
#include <algorithm>

// Partitionner : tous les val >= 128 en premier
auto pivot = std::partition(data.begin(), data.end(),
                            [](int val) { return val >= 128; });

// Traiter uniquement les éléments qui satisfont la condition
// → aucun branchement dans la boucle
std::int64_t sum = 0;  
for (auto it = data.begin(); it != pivot; ++it) {  
    sum += *it;
}
```

`std::partition` est O(n) et transforme un problème de branchement imprévisible en un parcours séquentiel sans condition. C'est un compromis entre le tri O(n log n) et le code branchless parfois complexe.

---

## Technique 6 : déroulement de boucle et réduction des branchements de contrôle

Chaque itération de boucle contient un branchement implicite : le test de fin de boucle. Pour les boucles très courtes (corps de 1–3 instructions), ce branchement de contrôle représente une fraction significative du coût total.

Le **déroulement de boucle** (*loop unrolling*) réduit le nombre de tests de fin de boucle en traitant plusieurs éléments par itération :

```cpp
// Boucle standard : 1 branchement par élément
for (std::size_t i = 0; i < n; ++i) {
    sum += data[i];
}

// Déroulée ×4 : 1 branchement pour 4 éléments
std::size_t i = 0;  
for (; i + 3 < n; i += 4) {  
    sum += data[i];
    sum += data[i + 1];
    sum += data[i + 2];
    sum += data[i + 3];
}
// Traiter le reste
for (; i < n; ++i) {
    sum += data[i];
}
```

Le compilateur effectue souvent ce déroulement automatiquement à `-O2` ou `-O3`, mais il peut être insuffisant pour les cas critiques. On peut influencer le comportement avec des pragmas :

```cpp
// GCC / Clang : suggérer un facteur de déroulement
#pragma GCC unroll 8
for (std::size_t i = 0; i < n; ++i) {
    sum += data[i];
}

// Clang spécifique
#pragma clang loop unroll_count(8)
for (std::size_t i = 0; i < n; ++i) {
    sum += data[i];
}
```

> ⚠️ **Précaution** : un déroulement excessif augmente la taille du code, ce qui peut provoquer des misses dans le cache L1i. Le gain sur les branchements peut être annulé par la perte de localité du code. Comme toujours : mesurer.

---

## Quand optimiser les branchements — et quand ne pas le faire

### Optimiser quand :

- `perf stat` montre un taux de `branch-misses` supérieur à **2–3 %** et que le programme passe une part significative de son temps dans les fonctions concernées.  
- `perf report -e branch-misses` identifie un hotspot clair — un `if` spécifique responsable de la majorité des mispredictions.  
- Le branchement est dans une **boucle serrée** exécutée des millions de fois avec des données imprévisibles.

### Ne PAS optimiser quand :

- Le taux de misprediction est faible (< 1 %). Le prédicteur gère correctement — toute réécriture branchless sera plus complexe sans gain mesurable.  
- Le branchement est **hors du hot path** — code d'initialisation, gestion d'erreurs rare, parsing de configuration.  
- Le corps du branchement est **lourd** (appels de fonctions, I/O, allocations mémoire). Le coût du misprediction (~15 cycles) est noyé dans le coût du corps (~100–10 000 cycles).  
- La réécriture branchless rend le code **significativement moins lisible** sans gain mesuré. La lisibilité est une forme de performance à long terme — celle de l'équipe qui maintient le code.

### Matrice de décision

| Situation | Action recommandée |
|-----------|-------------------|
| Boucle chaude, données aléatoires, taux de miss > 5 % | Branchless ou partitionnement |
| Boucle chaude, données biaisées (95%+ un côté) | `[[likely]]` / `[[unlikely]]` suffit |
| `switch` à 5+ cas équiprobables, hot path | Lookup table |
| Chaîne `if/else if` avec cas dominant clair | Réordonner, mettre le cas fréquent en premier |
| Code froid (erreurs, init, config) | Ne rien faire |
| Taux de miss < 1 % | Ne rien faire |

---

## Résumé des techniques

| Technique | Mécanisme | Gain typique | Complexité |
|-----------|-----------|-------------|------------|
| `[[likely]]` / `[[unlikely]]` | Layout du code machine | 2–5 % | Triviale |
| Réordonnancement des conditions | Réduit les tests évalués | 5–15 % | Faible |
| Code branchless (masque, `cmov`) | Élimine le branchement | 30–70 % sur données aléatoires | Moyenne |
| Lookup table | Remplace switch par accès mémoire | 20–50 % | Faible |
| Partitionnement des données | Sépare les cas avant la boucle | 50–80 % | Faible |
| Déroulement de boucle | Réduit les tests de fin de boucle | 5–20 % | Faible (pragma) |
| Tri préalable des données | Rend le pattern prévisible | 200–400 % | Élevée (O(n log n)) |

La section suivante (41.3) explore un levier d'optimisation complémentaire : les instructions SIMD, qui permettent de traiter plusieurs données par instruction — et qui sont souvent le prolongement naturel des techniques branchless vues ici.

---


⏭️ [SIMD et vectorisation (SSE, AVX)](/41-optimisation-cpu-memoire/03-simd-vectorisation.md)
