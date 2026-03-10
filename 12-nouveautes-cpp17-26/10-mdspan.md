🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 12.10 std::mdspan (C++23) : Vues multidimensionnelles ⭐

## Voir un tableau 1D comme une matrice

Le calcul scientifique, le traitement d'images, les simulations physiques, le machine learning — tous ces domaines manipulent des données multidimensionnelles : matrices, tenseurs, grilles 3D. Pourtant, la mémoire d'un ordinateur est fondamentalement linéaire : un bloc contigu d'octets adressés séquentiellement. Le fossé entre la vision multidimensionnelle du programmeur et la réalité linéaire de la mémoire a toujours été une source de complexité, d'erreurs et de code verbeux en C++.

Avant C++23, il n'existait aucune abstraction standard pour interpréter un bloc de mémoire linéaire comme une structure multidimensionnelle. Chaque projet réinventait ses propres classes `Matrix`, `Tensor` ou `Grid`, ou recourait à des bibliothèques tierces (Eigen, Blaze, xtensor). Les approches manuelles — tableaux de tableaux (`vector<vector<T>>`), arithmétique d'index brute — étaient soit inefficaces, soit illisibles, soit les deux.

`std::mdspan` (header `<mdspan>`) résout ce problème en fournissant une **vue multidimensionnelle non-owning** sur des données contiguës en mémoire. C'est l'extension naturelle de `std::span` (section 12.3) au cas multidimensionnel : léger, sans allocation, et d'une flexibilité remarquable grâce à un système de layouts et d'accessors personnalisables.

## Le problème : accès multidimensionnel sans abstraction

### Le piège du vector de vectors

L'approche la plus intuitive pour créer une matrice en C++ est un `vector<vector<T>>` :

```cpp
// Matrice 3×4 — approche naïve
std::vector<std::vector<double>> matrix(3, std::vector<double>(4, 0.0));  
matrix[1][2] = 3.14;  
```

Cette approche est problématique à plusieurs niveaux. Chaque ligne est un `vector` indépendant, alloué séparément sur le heap. Les lignes ne sont pas contiguës en mémoire — parcourir la matrice provoque des cache misses à chaque changement de ligne. Rien ne garantit que toutes les lignes ont la même taille. Et le double déréférencement (`operator[]` sur le vector externe, puis sur le vector interne) a un coût.

```
vector<vector<double>> — Mémoire fragmentée :

vector externe : [ptr_ligne0] [ptr_ligne1] [ptr_ligne2]
                      │            │            │
                      ▼            ▼            ▼
              ┌──────────┐  ┌───────────┐  ┌──────────┐
              │ 0 0 0 0  │  │ 0 0 3.14 0│  │ 0 0 0 0  │  ← allocations séparées
              └──────────┘  └───────────┘  └──────────┘
              (quelque part)  (ailleurs)   (encore ailleurs)
```

### L'arithmétique d'index manuelle

L'alternative performante est de stocker les données dans un vecteur unique et de calculer les indices manuellement :

```cpp
// Matrice 3×4 stockée dans un vecteur linéaire
std::vector<double> data(3 * 4, 0.0);

// Accès à l'élément (ligne, colonne) — row-major
auto index = [cols = 4](int row, int col) { return row * cols + col; };  
data[index(1, 2)] = 3.14;  
```

C'est performant — la mémoire est contiguë — mais le code est fragile et peu lisible. Le calcul d'index est sujet aux erreurs (confusion ligne/colonne, oubli de la largeur), il ne se généralise pas facilement à plus de deux dimensions, et il n'y a aucune vérification des bornes.

## La solution : std::mdspan

`std::mdspan` prend un pointeur vers des données contiguës et les expose à travers une interface multidimensionnelle. Aucune copie, aucune allocation — c'est une vue pure :

```cpp
#include <mdspan>
#include <vector>
#include <print>

// Données contiguës en mémoire
std::vector<double> data(3 * 4, 0.0);

// Vue multidimensionnelle 3×4 sur ces données
std::mdspan matrix(data.data(), 3, 4);

// Accès multidimensionnel — syntaxe naturelle avec operator[]
matrix[1, 2] = 3.14;

std::print("matrix[1,2] = {}\n", matrix[1, 2]);  // 3.14
```

L'opérateur `[]` multidimensionnel (C++23) accepte plusieurs indices séparés par des virgules. La conversion vers l'index linéaire est effectuée automatiquement selon le *layout* choisi.

```
std::mdspan — Vue sur mémoire contiguë :

data (vector<double>) :
┌────┬────┬────┬────┬────┬────┬────┬────┬────┬────┬────┬────┐
│ 0  │ 0  │ 0  │ 0  │ 0  │ 0  │3.14│ 0  │ 0  │ 0  │ 0  │ 0  │
└────┴────┴────┴────┴────┴────┴────┴────┴────┴────┴────┴────┘
  [0,0] [0,1] [0,2] [0,3] [1,0] [1,1] [1,2] [1,3] [2,0] [2,1] [2,2] [2,3]

mdspan matrix(data.data(), 3, 4) :
  ┌───────────────────┐
  │ rows=3, cols=4    │
  │ layout=row_major  │   → matrix[1,2] = data[1*4 + 2] = data[6]
  │ ptr → data[0]     │
  └───────────────────┘
```

## Anatomie de std::mdspan

La déclaration complète de `std::mdspan` est :

```cpp
template <
    class ElementType,
    class Extents,
    class LayoutPolicy = std::layout_right,
    class AccessorPolicy = std::default_accessor<ElementType>
>
class mdspan;
```

Quatre paramètres de template, dont deux ont des valeurs par défaut. Examinons chacun.

### ElementType : le type des éléments

Le type des données pointées — `double`, `float`, `int`, un type utilisateur, etc. Comme pour `std::span`, la constance se gère au niveau du type :

```cpp
std::mdspan<const double, ...>  // Vue en lecture seule  
std::mdspan<double, ...>        // Vue en lecture-écriture  
```

### Extents : les dimensions

Les *extents* décrivent la forme multidimensionnelle — le nombre de dimensions et la taille de chaque dimension. Elles peuvent être statiques (connues à la compilation) ou dynamiques (connues à l'exécution) :

```cpp
#include <mdspan>

// Dimensions entièrement dynamiques
std::mdspan<double, std::dextents<size_t, 2>> matrix(ptr, 3, 4);
// 2 = nombre de dimensions, 3 et 4 fournis à l'exécution

// Dimensions entièrement statiques
std::mdspan<double, std::extents<size_t, 3, 4>> matrix(ptr);
// 3×4 connu à la compilation — pas besoin de passer les tailles

// Dimensions mixtes
std::mdspan<double, std::extents<size_t, std::dynamic_extent, 4>> matrix(ptr, 3);
// Nombre de lignes dynamique, 4 colonnes fixes
```

Les extents statiques permettent au compilateur d'optimiser le calcul d'index (multiplication par une constante, voire remplacement par des décalages de bits). Les extents dynamiques offrent la flexibilité nécessaire quand les dimensions ne sont connues qu'à l'exécution.

En pratique, le CTAD (Class Template Argument Deduction) simplifie la syntaxe. Quand les tailles sont passées au constructeur, le compilateur déduit des extents dynamiques :

```cpp
std::vector<double> data(12);  
std::mdspan matrix(data.data(), 3, 4);  // Déduit mdspan<double, dextents<size_t, 2>>  
```

### LayoutPolicy : l'organisation en mémoire

Le layout détermine comment les indices multidimensionnels sont convertis en un offset linéaire. Le standard fournit trois layouts :

**`std::layout_right` (par défaut) — Row-major :**

Le dernier indice varie le plus rapidement. C'est la convention du C, du C++, de NumPy (par défaut), et de la plupart des bibliothèques modernes :

```
Matrice 3×4, layout_right (row-major) :

Mémoire : [0,0] [0,1] [0,2] [0,3] | [1,0] [1,1] [1,2] [1,3] | [2,0] [2,1] [2,2] [2,3]
           ────── ligne 0 ────────   ────── ligne 1 ────────   ────── ligne 2 ────────

offset(i, j) = i * 4 + j
```

Itérer sur les colonnes d'une même ligne est un accès séquentiel optimal pour le cache.

**`std::layout_left` — Column-major :**

Le premier indice varie le plus rapidement. C'est la convention de Fortran, MATLAB, et des bibliothèques LAPACK/BLAS :

```
Matrice 3×4, layout_left (column-major) :

Mémoire : [0,0] [1,0] [2,0] | [0,1] [1,1] [2,1] | [0,2] [1,2] [2,2] | [0,3] [1,3] [2,3]
           ── col 0 ───────   ── col 1 ───────   ── col 2 ───────   ── col 3 ───────

offset(i, j) = i + j * 3
```

Itérer sur les lignes d'une même colonne est optimal. Ce layout est essentiel pour l'interopérabilité avec les bibliothèques d'algèbre linéaire en Fortran.

**`std::layout_stride` — Strides personnalisés :**

Chaque dimension a un pas (*stride*) arbitraire, ce qui permet de représenter des sous-matrices, des tranches, ou des données avec du padding :

```cpp
// Matrice 3×4 avec un stride de 5 sur les lignes (padding de 1 élément par ligne)
std::array<size_t, 2> strides = {5, 1};  
std::layout_stride::mapping<std::dextents<size_t, 2>> map(  
    std::dextents<size_t, 2>(3, 4), strides);
std::mdspan<double, std::dextents<size_t, 2>, std::layout_stride> 
    matrix(ptr, map);

// offset(i, j) = i * 5 + j * 1
```

Le layout stride est indispensable pour les sous-vues et l'interopérabilité avec des bibliothèques qui imposent un alignement ou un padding spécifique.

### AccessorPolicy : contrôle de l'accès

L'accessor par défaut (`std::default_accessor<T>`) effectue un simple déréférencement de pointeur. Les accessors personnalisés permettent d'intercepter chaque accès aux données — pour la vérification de bornes, l'accès atomique, la conversion de type, ou l'accès à des mémoires spéciales (GPU, mémoire mappée) :

```cpp
// Concept : un accessor avec vérification de bornes
template <typename T>  
struct checked_accessor {  
    using element_type = T;
    using reference = T&;
    using data_handle_type = T*;
    using offset_policy = checked_accessor;

    reference access(data_handle_type p, size_t i) const {
        // Ici on pourrait vérifier les bornes
        return p[i];
    }

    data_handle_type offset(data_handle_type p, size_t i) const {
        return p + i;
    }
};
```

En pratique, l'accessor par défaut convient à la grande majorité des cas. Les accessors personnalisés relèvent de l'usage avancé (bibliothèques HPC, frameworks GPU).

## Cas d'usage

### Traitement d'images

Une image RGB peut être vue comme un tenseur 3D (hauteur × largeur × canaux) :

```cpp
#include <mdspan>
#include <vector>
#include <cstdint>

// Image 1920×1080, 3 canaux (RGB)
std::vector<uint8_t> pixels(1920 * 1080 * 3);

// Vue 3D : hauteur × largeur × canal
std::mdspan image(pixels.data(), 1080, 1920, 3);

// Accès naturel : image[ligne, colonne, canal]
image[540, 960, 0] = 255;   // Rouge au centre de l'image  
image[540, 960, 1] = 128;   // Vert  
image[540, 960, 2] = 0;     // Bleu  

// Conversion en niveaux de gris
for (size_t y = 0; y < image.extent(0); ++y) {
    for (size_t x = 0; x < image.extent(1); ++x) {
        auto r = image[y, x, 0];
        auto g = image[y, x, 1];
        auto b = image[y, x, 2];
        uint8_t gray = static_cast<uint8_t>(0.299 * r + 0.587 * g + 0.114 * b);
        image[y, x, 0] = gray;
        image[y, x, 1] = gray;
        image[y, x, 2] = gray;
    }
}
```

Le code est lisible, performant (mémoire contiguë, pas d'allocation supplémentaire), et la structure dimensionnelle est explicite dans le type.

### Algèbre linéaire et matrices

```cpp
#include <mdspan>
#include <vector>
#include <print>

using matrix_t = std::mdspan<double, std::dextents<size_t, 2>>;

// Multiplication matricielle C = A × B
void mat_mul(matrix_t A, matrix_t B, matrix_t C) {
    assert(A.extent(1) == B.extent(0));
    assert(C.extent(0) == A.extent(0));
    assert(C.extent(1) == B.extent(1));

    for (size_t i = 0; i < A.extent(0); ++i) {
        for (size_t j = 0; j < B.extent(1); ++j) {
            double sum = 0.0;
            for (size_t k = 0; k < A.extent(1); ++k) {
                sum += A[i, k] * B[k, j];
            }
            C[i, j] = sum;
        }
    }
}

int main() {
    std::vector<double> a_data = {1, 2, 3, 4, 5, 6};     // 2×3
    std::vector<double> b_data = {7, 8, 9, 10, 11, 12};   // 3×2
    std::vector<double> c_data(4, 0.0);                    // 2×2

    matrix_t A(a_data.data(), 2, 3);
    matrix_t B(b_data.data(), 3, 2);
    matrix_t C(c_data.data(), 2, 2);

    mat_mul(A, B, C);
    std::print("C[0,0]={}, C[0,1]={}, C[1,0]={}, C[1,1]={}\n",
               C[0, 0], C[0, 1], C[1, 0], C[1, 1]);
    // C[0,0]=58, C[0,1]=64, C[1,0]=139, C[1,1]=154
}
```

La fonction `mat_mul` accepte des `mdspan` de dimensions quelconques — elle fonctionne avec des matrices de toute taille sans être un template sur le type de conteneur. C'est l'avantage d'une vue non-owning : la fonction ne se soucie pas de la provenance des données (vector, array, allocation C, mémoire mappée).

### Interopérabilité avec du code C et Fortran

`std::mdspan` est idéal pour envelopper des buffers provenant d'API C ou de bibliothèques Fortran (BLAS, LAPACK) :

```cpp
// Appel à une fonction C qui retourne un buffer brut
extern "C" double* compute_grid(int nx, int ny, int nz);

double* raw = compute_grid(100, 200, 50);

// Envelopper le buffer C dans une vue 3D C++ — zéro copie
std::mdspan grid(raw, 100, 200, 50);

// Accès multidimensionnel sûr et lisible
double center_value = grid[50, 100, 25];

// Pour une bibliothèque Fortran (column-major) :
std::mdspan<double, std::dextents<size_t, 2>, std::layout_left> 
    fortran_matrix(raw, 100, 200);
```

Le choix du layout (`layout_right` pour C/C++, `layout_left` pour Fortran) garantit que les indices correspondent à la convention de la bibliothèque sous-jacente.

### Simulations et grilles multidimensionnelles

Les simulations physiques (mécanique des fluides, éléments finis, automates cellulaires) manipulent des grilles 2D ou 3D :

```cpp
#include <mdspan>
#include <vector>

// Grille 3D pour une simulation thermique
constexpr size_t NX = 100, NY = 100, NZ = 100;  
std::vector<double> temperature(NX * NY * NZ, 20.0);  // 20°C partout  

std::mdspan grid(temperature.data(), NX, NY, NZ);

// Source de chaleur au centre
grid[50, 50, 50] = 500.0;

// Un pas de diffusion thermique (schéma aux différences finies simplifié)
std::vector<double> next_temp(NX * NY * NZ);  
std::mdspan next(next_temp.data(), NX, NY, NZ);  

double alpha = 0.1;  
for (size_t x = 1; x + 1 < NX; ++x) {  
    for (size_t y = 1; y + 1 < NY; ++y) {
        for (size_t z = 1; z + 1 < NZ; ++z) {
            next[x, y, z] = grid[x, y, z] + alpha * (
                grid[x-1, y, z] + grid[x+1, y, z] +
                grid[x, y-1, z] + grid[x, y+1, z] +
                grid[x, y, z-1] + grid[x, y, z+1] -
                6.0 * grid[x, y, z]);
        }
    }
}
```

Sans `mdspan`, ce code utiliserait une macro ou une lambda `index(x, y, z)` avec un calcul `x * NY * NZ + y * NZ + z` à chaque accès — moins lisible et plus sujet aux erreurs.

## Propriétés essentielles

### Non-owning et léger

Comme `std::span`, `std::mdspan` ne possède pas les données. Sa taille en mémoire est celle d'un pointeur plus les métadonnées d'extents et de layout — typiquement 16 à 40 octets selon la configuration. Le copier est trivial.

### Zéro coût d'abstraction

Le calcul d'index est effectué inline par le compilateur. Avec des extents statiques et le layout par défaut, `matrix[i, j]` génère exactement le même assembleur que `ptr[i * cols + j]`. Le compilateur optimise les multiplications par des constantes connues à la compilation en shifts et additions. L'abstraction multidimensionnelle n'a littéralement aucun coût runtime.

### Nombre de dimensions arbitraire

`mdspan` supporte un nombre quelconque de dimensions — 1D, 2D, 3D, ou plus. Le nombre de dimensions est fixé à la compilation via les extents :

```cpp
std::mdspan<double, std::dextents<size_t, 1>> vector_1d(ptr, 100);  
std::mdspan<double, std::dextents<size_t, 2>> matrix_2d(ptr, 10, 10);  
std::mdspan<double, std::dextents<size_t, 3>> tensor_3d(ptr, 10, 10, 10);  
std::mdspan<double, std::dextents<size_t, 4>> hyper_4d(ptr, 5, 5, 5, 5);  
```

### Méthodes d'inspection

```cpp
std::mdspan matrix(data.data(), 3, 4);

matrix.rank();           // 2 — nombre de dimensions  
matrix.extent(0);        // 3 — taille de la première dimension  
matrix.extent(1);        // 4 — taille de la seconde dimension  
matrix.size();           // 12 — nombre total d'éléments  
matrix.empty();          // false  
matrix.data_handle();    // Pointeur brut sous-jacent  
```

## Sous-vues avec std::submdspan

C++26 standardise `std::submdspan` pour extraire des sous-vues d'un `mdspan` — lignes, colonnes, blocs, tranches — sans copie :

```cpp
#include <mdspan>

std::vector<double> data(12);  
std::mdspan matrix(data.data(), 3, 4);  

// Extraire la ligne 1 (vue 1D)
auto row_1 = std::submdspan(matrix, 1, std::full_extent);
// row_1 est un mdspan 1D de 4 éléments

// Extraire la colonne 2 (vue 1D avec stride)
auto col_2 = std::submdspan(matrix, std::full_extent, 2);
// col_2 est un mdspan 1D de 3 éléments (avec stride)

// Extraire un bloc 2×2 (sous-matrice)
auto block = std::submdspan(matrix, 
    std::pair{0, 2},    // lignes 0 à 1
    std::pair{1, 3});   // colonnes 1 à 2
// block est un mdspan 2×2
```

`std::submdspan` retourne un `mdspan` avec un layout stride qui encode le pas nécessaire pour « sauter » les éléments non inclus dans la sous-vue. Aucune donnée n'est copiée — c'est une nouvelle fenêtre sur les mêmes données.

> ⚠️ *`std::submdspan` est standardisé en C++26. Certains compilateurs le fournissent déjà comme extension ou dans une implémentation expérimentale de `<mdspan>`. Vérifier le support de votre compilateur.*

## mdspan vs les alternatives

| Critère | `vector<vector<T>>` | Index manuel | `std::mdspan` |
|---------|---------------------|-------------|----------------|
| Mémoire contiguë | Non | Oui | Oui (vue) |
| Syntaxe d'accès | `m[i][j]` | `m[i*cols+j]` | `m[i, j]` |
| Vérification dimensions | Non | Non | Compile-time possible |
| Ownership | Oui | Dépend | Non (vue) |
| Multi-layout | Non | Manuel | `layout_right/left/stride` |
| N dimensions | Imbrication | Manuel | Natif |
| Performance | Médiocre | Optimale | Optimale |
| Interopérabilité C/Fortran | Difficile | Manuelle | Naturelle |

## Bonnes pratiques

**Utiliser `mdspan` pour toute interface de fonction travaillant sur des données multidimensionnelles.** Comme `std::span` pour le 1D, `std::mdspan` découple la fonction du type de conteneur et rend la dimensionnalité explicite dans la signature.

**Préférer les extents statiques quand les dimensions sont connues à la compilation.** Le compilateur génère un code significativement plus efficace quand les tailles sont des constantes (élimination des multiplications, déroulement de boucles).

**Choisir le layout en fonction de l'usage.** `layout_right` (row-major) pour le code C++ natif et l'itération ligne par ligne. `layout_left` (column-major) pour l'interopérabilité Fortran/BLAS et l'itération colonne par colonne. `layout_stride` pour les sous-vues et les cas spéciaux.

**Ne pas stocker un mdspan au-delà de la durée de vie des données.** Comme pour `span` et `string_view`, un `mdspan` est une vue temporaire. Les données sous-jacentes doivent survivre au mdspan.

**Combiner mdspan avec les algorithmes parallèles.** Les boucles sur un `mdspan` se parallélisent naturellement — les accès sont indépendants et la mémoire est contiguë. C'est un terrain idéal pour `std::execution::par` (section 15.7) ou les futures primitives de `std::execution` (section 12.14.4).

---


⏭️ [std::generator (C++23) : Coroutines simplifiées](/12-nouveautes-cpp17-26/11-generator.md)
