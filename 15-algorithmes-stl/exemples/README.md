# Chapitre 15 — Algorithmes de la STL : Exemples

## Compilation

Chaque fichier est un programme autonome compilable avec :

```bash
g++-15 -std=c++23 -Wall -Wextra -Wpedantic -g fichier.cpp -o fichier
```

Pour `07_algorithmes_paralleles.cpp`, ajouter `-O2` et optionnellement `-ltbb` pour activer la parallélisation réelle (GCC/libstdc++). Sans `-ltbb`, les algorithmes s'exécutent en fallback séquentiel :

```bash
g++-15 -std=c++23 -Wall -Wextra -Wpedantic -O2 -g 07_algorithmes_paralleles.cpp -o 07_algorithmes_paralleles -ltbb
```

Compilation de tous les exemples (sauf parallèle) :

```bash
for f in 0[0-6]*.cpp; do
    echo "Compilation de $f..."
    g++-15 -std=c++23 -Wall -Wextra -Wpedantic -g "$f" -o "${f%.cpp}" || echo "ERREUR: $f"
done  
echo "Compilation de 07_algorithmes_paralleles.cpp..."  
g++-15 -std=c++23 -Wall -Wextra -Wpedantic -O2 -g 07_algorithmes_paralleles.cpp -o 07_algorithmes_paralleles  
```

---

## Liste des exemples

### 00_introduction.cpp

- **Section** : 15.0 — Introduction
- **Fichier source** : README.md
- **Description** : Exemples d'introduction du chapitre couvrant `count_if`, `transform`, `find_if`, remove-erase idiom, C++20 `std::erase`, conventions classiques vs Ranges.
- **Sortie attendue** :
```
1 2 3 5 8 9
find(8) OK  
ranges::find(8) OK  
Nombres pairs : 5  
1 4 9 16 25 36 49 64 81 100
Premier > 7 : 8
1 3 5 7
1 3 5 7
1 3 5
```

---

### 01_recherche.cpp

- **Section** : 15.1 — Recherche : std::find, std::binary_search
- **Fichier source** : 01-recherche.md
- **Description** : Exemples complets couvrant `find`, `find_if`, `find_if_not`, `ranges::find` avec projections, `count`, `count_if`, `all_of`/`any_of`/`none_of` (dont séquence vide), `binary_search`, `lower_bound`, `upper_bound`, `equal_range`, recherche avec comparateur personnalisé, `search`, `boyer_moore_searcher`, `adjacent_find`, `min_element`, `max_element`, `minmax_element`, méthodes membres des conteneurs associatifs.
- **Sortie attendue** :
```
Trouvé : 42 à l'index 2  
Premier impair : 25  
Trouvé : Carol  
Premier non-pair : 7  
ranges::find(42) à l'index 2  
ranges::find_if(>30) : 42  
Département de Carol : 42  
Employé dept 17 : Bob  
Nombre de 2 : 4  
Éléments > 3 : 2
ranges: count(2)=4, count_if(>3)=2  
all_even=true, has_big=true, no_neg=true  
all_of sur vide : true  
binary_search(7)=true, binary_search(8)=false  
lower_bound(5): *it=5, index 2  
lower_bound(6): *it=7, index 5  
Trouvé à l'index 3  
upper_bound(5): *it=7, index 5  
Nombre de 5 : 3  
5 5 5
Occurrences de 5 : 3  
binary_search(9, greater)=true  
lower_bound(9, greater): *it=9  
Événement : ready
ranges: Événement : ready  
Sous-séquence trouvée à l'index 3  
Trouvé à la position 10  
Premier doublon consécutif : 3 à l'index 2  
Rupture d'ordre : 4 > 3 à l'index 2  
Min : 5 (index 3)  
Max : 93 (index 2)  
Min : 5, Max : 93  
Salaire le plus bas : Bob (62000)  
Salaire le plus haut : Carol (88000)  
ranges: Bob (lo), Carol (hi)  
Alice trouvée  
Bob a 25 ans  
```

---

### 02_tri.cpp

- **Section** : 15.2 — Tri : std::sort, std::stable_sort
- **Fichier source** : 02-tri.md
- **Description** : Exemples couvrant `sort` basique, `sort` avec `std::greater`, `sort` de structs avec lambda, `sort` de sous-range, `ranges::sort` avec projection, `stable_sort` préservant l'ordre relatif, `partial_sort`, `partial_sort_copy`, `nth_element`, `is_sorted`, `is_sorted_until`, spaceship operator (`<=>`), sort de tableaux C et `std::array`, `list::sort`.
- **Sortie attendue** :
```
5 7 17 31 42 68 93
93 68 42 31 17 7 5
cache-01(23), web-02(45), web-03(45), web-01(72), db-01(91),
1 3 5 7 9 8 6 4 2 0
cache-01(23), web-02(45), web-03(45), web-01(72), db-01(91),  
deploy(1, #2), monitor(1, #4), backup(2, #1), cleanup(2, #3), archive(2, #5),  
Top 3: 5 7 12  
top3: 5 7 12  
Médiane (v[4]): 31  
v1 sorted: true, v2 sorted: false  
Trié jusqu'à l'index 4 (valeur 2)  
{1,9,3} {1,9,3} {2,0,1} {2,1,0} {3,0,0}
5 17 42 68 93
5 17 42 68 93
5 17 42 68 93
```

---

### 03_transformation.cpp

- **Section** : 15.3 — Transformation et accumulation
- **Fichier source** : 03-transformation.md
- **Description** : Exemples couvrant `transform` (unaire, binaire, in-place, changement de type), `ranges::transform` avec projection, `for_each`, `for_each_n`, `accumulate` (somme, produit, piège du type initial, concaténation de strings, Stats custom), `reduce`, `transform_reduce` (somme des carrés, produit scalaire, MAE), `inner_product`, `partial_sum` (cumul, budget), `inclusive_scan`/`exclusive_scan`, `iota` (séquence, tri indirect), `generate`, `generate_n`.
- **Sortie attendue** :
```
1 4 9 16 25
1 4 9 16 25
2 4 6 8 10
11 22 33 44 55
10 -5 10 10
OK, Not Found, Server Error, Redirect, OK,  
Alice Bob Carol  
1 2 3 4 5
3 6 9 12 15
10 20 30 40 5 6 7 8
sum=15  
product=120  
bad=6, good=7.5  
sentence=Hello World!  
Somme: 11.22, Min: 1.41, Max: 3.14, Moyenne: 2.24  
reduce sum=15  
reduce product=120  
sum_sq=55  
total=130  
mae=0.7  
dot=32  
1 3 6 10 15
Budget dépassé au mois 4 (cumul : 5030)  
inc: 1 3 6 10 15  
exc: 0 1 3 6 10  
0 1 2 3 4 5 6 7 8 9
Alice  
Bob  
Charlie  
Dave  
1 2 4 8 16 32 64 128 256 512
0 1 4 9 16
```

---

### 04_manipulation.cpp

- **Section** : 15.4 — Manipulation de séquences
- **Fichier source** : 04-manipulation.md
- **Description** : Exemples couvrant `copy`, `copy_if`, `copy_n`, `copy_backward`, `move`, remove-erase idiom, C++20 `std::erase`/`std::erase_if`, `unique`, `sort`+`unique`+`erase`, `reverse`, `rotate`, `shuffle`, `partition`, `stable_partition`, `partition_copy`, `fill`, `fill_n`, `swap_ranges`, `replace`, `replace_if`, opérations ensemblistes (`set_union`, `set_intersection`, `set_difference`, `set_symmetric_difference`), `merge`, `inplace_merge`, `includes`.
- **Comportement attendu** : Le shuffle produit un résultat déterministe grâce à la seed fixe (42). Toutes les opérations ensemblistes requièrent des séquences triées.
- **Sortie attendue** :
```
1 2 3 4 5
30 40 50
2 4 6 8 10
10 20 30
1 1 2 3 4 5 0
99 1 2 3 4 5 0
alpha bravo charlie delta
1 3 5 7 (size=4)
1 2 4 5 7 8 10
1 3 5 7 (removed=3)
1 2 4 5 7 8 10 (removed=3)
1 2 3 4 5
1 2 3 4
5 4 3 2 1
3 4 5 1 2
Home Shop About Blog Contact  
shuffled: 2 7 8 1 6 10 9 3 4 5  
Nombre de pairs : 4  
8 2 4 6 3 5 1 9 7
evens: 2 4 6 8 10  
odds: 1 3 5 7 9  
0 0 0 0 0 42 42 42 42 42
0 0 0 99 99 99 99 0 0 0
a: 10 20 30 40 50  
b: 1 2 3 4 5  
1 99 3 99 5 99 7
1 0 3 0 5 0 7
Union: 1 2 3 4 5 6 7  
Intersection: 3 4 5  
Difference: 1 2  
Sym diff: 1 2 6 7  
1 2 3 4 5 6 7 8
1 2 3 4 5 6 7 8
includes: true
```

---

### 05_iterateurs.cpp

- **Section** : 15.5 — Itérateurs : input, output, forward, bidirectional, random_access
- **Fichier source** : 05-iterateurs.md
- **Description** : Exemples couvrant `istream_iterator` (lecture de flux vers vector), `ostream_iterator`, `back_inserter`, `front_inserter`, `inserter` (avec set, vector, deque), `forward_list` multi-pass, navigation bidirectionnelle sur `list`, arithmétique random access sur `vector`, itérateurs inversés (`rbegin`/`rend`), `find_if` avec reverse iterator et `.base()`, `std::next`/`std::prev`, concepts C++20 (`static_assert`).
- **Sortie attendue** :
```
10 20 30 40 50
n=5
1 2 3 4 5
1 2 3
3 2 1
1 2 3
*it=30, *saved=10
*it=50
*it=40, *it2=20, d=2
5 4 3 2 1
Dernier > 5 : 6  
Index : 7  
*third=30, *it=10
*before_last=50
10 20 1 2 3 4 5
3 2 1 10 20
10 20 1 2 3 30 40
10 20 30 40 50 60
Concept checks passed
```

---

### 06_ranges.cpp

- **Section** : 15.6 — Ranges (C++20) : Simplification des algorithmes
- **Fichier source** : 06-ranges.md
- **Description** : Exemples couvrant `ranges::sort`, projections multiples (sort par champ, sort décroissant, sort par score composite), `ranges::find` avec projection, `ranges::count_if` avec projection, `ranges::minmax_element` avec projection, `ranges::find_if` avec projection, valeur de retour enrichie (`ranges::copy`), `ranges::remove`, `subrange` sort, sentinelle `NullTerminator`, views `filter`+`transform`, dangling protection, composition filter+transform+accumulate.
- **Sortie attendue** :
```
1 2 3 5 7 8 9
cache-01(23), web-02(45), web-01(72), db-01(91),  
web-02(45), web-01(72), db-01(91), cache-01(23),  
cache-01(23/2048), web-01(72/4096), web-02(45/8192), db-01(91/16384),  
Trouvé: db-01 (cpu=91)  
Serveurs > 50%: 2  
Moins de RAM : cache-01 (2048MB)  
Plus de RAM : db-01 (16384MB)  
Serveur > 10000MB: db-01 (16384MB)  
in_it==end: true, out_it==end: true  
1 3 5 7
1 3 5 7 9 8 6 4 2 0
1 3 5 7 9 8 6 4 2 0
Nombre de 'l' : 2
4 16 36 64 100
Trouvé 42 à l'index 3  
total=220  
```

---

### 06_1_views_lazy.cpp

- **Section** : 15.6.1 — Views et lazy evaluation
- **Fichier source** : 06.1-views-lazy.md
- **Description** : Exemples couvrant `views::filter`, `views::transform` (int, struct avec pointeur vers membre), `views::take`, `views::take_while`, `views::drop`, `views::drop_while`, `views::reverse`, `views::keys`/`views::values`, `views::elements<N>`, `views::iota` (infini et borné), `views::enumerate` (C++23), `views::zip` (C++23), `views::split`, `views::join`, `views::common` (avec `std::accumulate`), matérialisation `std::ranges::to` (C++23), views sur données mutables.
- **Sortie attendue** :
```
1 3 5 7 9
1 4 9 16 25
Alice  
Bob  
Carol  
10 20 30
2 4 6
40 50 60 70
6 7 8
5 4 3 2 1
Alice Bob Carol
95 82 91
web-01 db-01 cache-01
0 1 2 3 4
1 2 3 4 5 6 7 8 9 10
[0] Alice
[1] Bob
[2] Carol
Alice: 95  
Bob: 82  
Carol: 91  
Alice  
Bob  
Carol  
Dave  
1 2 3 4 5 6 7 8 9
sum=20
1 4 9 16 25
1 2 3 4 5
1 4 3 8 5 12 7 16 9 20
```

---

### 06_2_pipelines.cpp

- **Section** : 15.6.2 — Pipelines avec l'opérateur `|`
- **Fichier source** : 06.2-pipelines.md
- **Description** : Exemples couvrant pipeline `filter`+`transform`+`take`, anatomie d'un pipeline, séquence infinie Fizz, `map` prices en majuscules, `zip`+`filter`+`enumerate` (C++23), matérialisation `ranges::to` (C++23), `count_if` sur view, adaptateur personnalisé `take_evens`, `max_element` sur view filtrée, pipeline de logs avec `reverse`.
- **Sortie attendue** :
```
4 16 36
30 44 94 20
3 6 9 12 18 21 24 27 33 36
CHERRY  
DATE  
ELDERBERRY  
FIG  
#1 Alice (score: 95)
#2 Carol (score: 88)
#3 Eve (score: 91)
4 16 36 64 100
Carrés de pairs > 20 : 2
2 4 6
Top eng : Carol (95000€)
2024-01-01 10:05 [Rate limit exceeded]
2024-01-01 10:03 [Out of memory]
2024-01-01 10:01 [Connection timeout]
```

---

### 07_algorithmes_paralleles.cpp

- **Section** : 15.7 — Algorithmes parallèles : std::execution policies
- **Fichiers source** : 07-algorithmes-paralleles.md, 07.1-politiques-execution.md, 07.2-parallelisation-algorithmes.md, 07.3-precautions-limitations.md
- **Description** : Exemples couvrant les quatre politiques d'exécution (`seq`, `par`, `par_unseq`, `unseq`), sort parallèle avec benchmark, `reduce` parallèle, `transform` parallèle, `transform_reduce` (somme des carrés, produit scalaire), `reduce` max parallèle, `count_if` parallèle, `find_if` parallèle, `for_each` parallèle (normalisation), `inclusive_scan` parallèle, politique comme paramètre template, seuil adaptatif, validation préalable (éviter les exceptions), patterns corrects vs data races (`reduce` au lieu de `for_each`+accumulation, `copy_if` au lieu de `for_each`+`push_back`, `minmax_element`).
- **Compilation** : `g++-15 -std=c++23 -Wall -Wextra -Wpedantic -O2 -g 07_algorithmes_paralleles.cpp -o 07_algorithmes_paralleles` (ajouter `-ltbb` pour la parallélisation réelle)
- **Comportement attendu** : Sans `-ltbb`, les algorithmes s'exécutent en fallback séquentiel (les temps seq et par seront similaires). Avec `-ltbb`, le sort parallèle devrait être 2-6x plus rapide selon le nombre de coeurs. Les valeurs numériques (sommes, comptages) sont déterministes. Les temps de benchmark varient selon le matériel.
- **Sortie attendue** (valeurs fixes, temps variables) :
```
seq:       1 3 5 8 9  
par:       1 3 5 8 9  
par_unseq: 1 3 5 8 9  
unseq:     7.0 3.0 9.0 4.0 5.0  
Sort 1M : seq=...ms, par=...ms  
Résultats identiques : true  
accumulate=500000500000, reduce(par)=500000500000  
transform par: result[0]=0.0000, result[999999]=13815.5106  
sum_sq=55  
dot=32  
max parallèle=1000000  
count_if > 500000 : ~500000  
find_if(42) : trouvé à l'index 42  
0.20 0.40 0.60 0.80 1.00
1 3 6 10 15
RMS small=4.0825, RMS large=57735.4599  
small sorted: true  
large sorted: true  
1.0 2.0 3.0 4.0 5.0
reduce sum=4999950000  
copy_if > 50000 : 49999 éléments  
min=0, max=99999  
```
