🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 5.3 — Arithmétique des pointeurs et accès bas niveau

## Le pointeur : une adresse typée

Un pointeur en C++ est une variable qui contient l'**adresse mémoire** d'un objet. Mais un pointeur n'est pas qu'une simple adresse — c'est une adresse **typée**. Le type du pointeur indique au compilateur quelle taille de données se trouve à cette adresse et comment les interpréter.

```cpp
int valeur = 42;  
int* ptr = &valeur;   // ptr contient l'adresse de 'valeur'  

std::cout << ptr << "\n";    // affiche l'adresse, ex: 0x7ffd3b2a1e4c  
std::cout << *ptr << "\n";   // déréférencement — affiche 42  
```

L'opérateur `&` (adresse de) produit un pointeur à partir d'une variable. L'opérateur `*` (déréférencement) accède à la valeur stockée à l'adresse pointée. Ces deux opérateurs sont inverses l'un de l'autre : `*(&valeur)` est équivalent à `valeur`.

Sur un système 64 bits, un pointeur occupe toujours **8 octets**, quel que soit le type pointé. Un `int*`, un `double*`, un `Connexion*` ont tous la même taille — seule leur interprétation diffère.

---

## Arithmétique de base : addition et soustraction

C'est ici que le type du pointeur entre en jeu. Quand vous ajoutez `1` à un pointeur, vous n'avancez pas d'un **octet** — vous avancez d'un **élément**, c'est-à-dire de `sizeof(type_pointé)` octets :

```cpp
int tableau[] = {10, 20, 30, 40, 50};  
int* p = tableau;         // p pointe vers tableau[0]  

std::cout << *p << "\n";       // 10  
std::cout << *(p + 1) << "\n"; // 20 — avance de sizeof(int) = 4 octets  
std::cout << *(p + 2) << "\n"; // 30 — avance de 2 × 4 = 8 octets  
std::cout << *(p + 4) << "\n"; // 50 — avance de 4 × 4 = 16 octets  
```

Voici ce qui se passe en mémoire, en supposant que `tableau` commence à l'adresse `0x1000` :

```
Adresse :   0x1000   0x1004   0x1008   0x100C   0x1010  
Valeur  :   [ 10 ]   [ 20 ]   [ 30 ]   [ 40 ]   [ 50 ]  
Pointeur:    p        p+1      p+2      p+3      p+4  
```

Le compilateur traduit `p + n` en `adresse_de_p + n × sizeof(int)`. C'est cette mise à l'échelle automatique qui rend l'arithmétique des pointeurs cohérente avec les types — et c'est pourquoi le type du pointeur est si important.

Avec un type plus grand, l'incrément physique change :

```cpp
double valeurs[] = {1.1, 2.2, 3.3};  
double* d = valeurs;  

// d + 1 avance de sizeof(double) = 8 octets, pas 4
std::cout << *d << "\n";       // 1.1  
std::cout << *(d + 1) << "\n"; // 2.2  
std::cout << *(d + 2) << "\n"; // 3.3  
```

La soustraction fonctionne symétriquement : `p - 1` recule d'un élément. Et la soustraction de deux pointeurs du même type retourne le **nombre d'éléments** entre eux, pas le nombre d'octets :

```cpp
int tableau[] = {10, 20, 30, 40, 50};  
int* debut = &tableau[0];  
int* fin   = &tableau[4];  

std::ptrdiff_t distance = fin - debut;  // 4 (éléments), pas 16 (octets)  
std::cout << distance << "\n";          // affiche 4  
```

Le type retourné par la soustraction de deux pointeurs est `std::ptrdiff_t`, un entier signé défini dans `<cstddef>`.

---

## Équivalence pointeur-tableau

En C++, le nom d'un tableau se décompose (*decay*) implicitement en un pointeur vers son premier élément dans la plupart des contextes. Cette propriété crée une **équivalence syntaxique** entre l'accès par index et l'arithmétique de pointeurs :

```cpp
int tab[] = {100, 200, 300};

// Ces quatre écritures sont strictement équivalentes :
tab[2];          // accès par index — le plus lisible
*(tab + 2);      // arithmétique de pointeurs explicite
*(2 + tab);      // l'addition est commutative
2[tab];          // conséquence bizarre de la commutativité — à éviter absolument
```

Le compilateur traduit `tab[i]` en `*(tab + i)` dans tous les cas. L'opérateur `[]` n'est qu'un raccourci syntaxique (*syntactic sugar*) pour le déréférencement avec offset.

Cette équivalence signifie qu'un pointeur peut être utilisé exactement comme un tableau :

```cpp
void afficher(const int* donnees, int taille) {
    for (int i = 0; i < taille; ++i) {
        std::cout << donnees[i] << " ";   // [] fonctionne sur un pointeur
    }
    std::cout << "\n";
}

int main() {
    int valeurs[] = {1, 2, 3, 4, 5};
    afficher(valeurs, 5);    // le tableau decay en pointeur

    int* heap_valeurs = new int[5]{10, 20, 30, 40, 50};
    afficher(heap_valeurs, 5);  // un pointeur heap fonctionne aussi
    delete[] heap_valeurs;
}
```

> **Attention :** l'équivalence a ses limites. `sizeof(tableau)` retourne la taille totale du tableau en octets, tandis que `sizeof(pointeur)` retourne toujours 8 (sur 64 bits). Une fois qu'un tableau a *decayed* en pointeur, l'information sur sa taille est **perdue**. C'est pourquoi les fonctions C recevant un tableau ont toujours besoin d'un paramètre `taille` séparé — et c'est un des problèmes que `std::span` (C++20, section 13.5) résout élégamment.

---

## Incrémentation et parcours idiomatique

Les opérateurs `++` et `--` s'appliquent aux pointeurs et avancent ou reculent d'un élément :

```cpp
int donnees[] = {5, 10, 15, 20, 25};  
int* p = donnees;  
int* fin = donnees + 5;   // pointe un élément APRÈS le dernier (past-the-end)  

// Parcours avec pointeur — style C classique
while (p != fin) {
    std::cout << *p << " ";
    ++p;    // avance au prochain élément
}
// Sortie : 5 10 15 20 25
```

Ce pattern — un pointeur de début et un pointeur *past-the-end* — est l'ancêtre direct du concept d'**itérateurs** dans la STL. Quand vous écrivez une boucle avec `begin()` et `end()` sur un `std::vector`, le mécanisme sous-jacent est exactement le même : deux pointeurs (ou objets qui se comportent comme des pointeurs) définissant une plage semi-ouverte `[début, fin)`.

```cpp
#include <vector>

std::vector<int> v = {5, 10, 15, 20, 25};

// Le vector::iterator est conceptuellement un pointeur
for (auto it = v.begin(); it != v.end(); ++it) {
    std::cout << *it << " ";
}
```

---

## Pointeurs vers des structures et l'opérateur ->

Quand un pointeur pointe vers un objet de type structure ou classe, on accède aux membres via l'opérateur `->`, qui combine déréférencement et accès membre :

```cpp
struct Point {
    double x;
    double y;
};

Point* p = new Point{3.0, 4.0};

// Ces deux écritures sont équivalentes :
std::cout << (*p).x << "\n";   // déréférencement puis accès membre  
std::cout << p->x << "\n";     // syntaxe raccourcie — à préférer  

// Arithmétique sur un tableau de structures
Point points[] = {{1.0, 2.0}, {3.0, 4.0}, {5.0, 6.0}};  
Point* curseur = points;  

std::cout << curseur->x << "\n";        // 1.0  
std::cout << (curseur + 1)->y << "\n";  // 4.0  
std::cout << (curseur + 2)->x << "\n";  // 5.0  

delete p;
```

L'arithmétique fonctionne identiquement : `curseur + 1` avance de `sizeof(Point)` octets, ce qui correspond bien à l'élément suivant du tableau.

---

## Pointeur void* : le pointeur générique

Le type `void*` est un pointeur **sans type associé**. Il peut contenir l'adresse de n'importe quel objet, mais il ne peut pas être déréférencé directement ni faire l'objet d'arithmétique — le compilateur ne sait pas de combien d'octets avancer :

```cpp
int entier = 42;  
double flottant = 3.14;  

void* generique = &entier;       // ✅ toute adresse peut être stockée
// *generique;                   // ❌ erreur — impossible de déréférencer void*
// generique + 1;                // ❌ erreur — arithmétique impossible sur void*

// Pour utiliser un void*, il faut le caster vers un type concret
int* p = static_cast<int*>(generique);  
std::cout << *p << "\n";         // 42  

generique = &flottant;           // ✅ changement de type pointé  
double* d = static_cast<double*>(generique);  
std::cout << *d << "\n";         // 3.14  
```

Le `void*` est omniprésent dans les **API C** (`malloc` retourne un `void*`, les callbacks POSIX utilisent `void*` comme paramètre générique). En C++ moderne, on lui préfère les templates et `std::any`, mais il reste incontournable pour l'interopérabilité avec le C.

> ⚠️ **Danger :** caster un `void*` vers le mauvais type est un comportement indéfini. Si vous stockez l'adresse d'un `int` et que vous castez en `double*`, le déréférencement lit 8 octets là où seuls 4 sont valides — le résultat est incohérent voire un crash.

---

## nullptr : le pointeur nul moderne

Avant C++11, on utilisait `NULL` (une macro définie comme `0`) ou le littéral `0` pour représenter un pointeur nul. C++11 a introduit `nullptr`, un littéral de type `std::nullptr_t` qui ne peut être converti **qu'en pointeur** :

```cpp
int* p1 = nullptr;     // ✅ C++ moderne — sans ambiguïté  
int* p2 = NULL;        // ⚠️ legacy — NULL est souvent défini comme 0  
int* p3 = 0;           // ⚠️ compile mais ambigu — est-ce un entier ou un pointeur ?  
```

La différence n'est pas cosmétique. Elle résout une ambiguïté réelle lors de la surcharge de fonctions :

```cpp
void traiter(int valeur)    { std::cout << "entier\n"; }  
void traiter(int* pointeur) { std::cout << "pointeur\n"; }  

traiter(NULL);      // ❓ ambigu — NULL est 0, qui est un int → appelle traiter(int)  
traiter(nullptr);   // ✅ sans ambiguïté → appelle traiter(int*)  
```

Déréférencer `nullptr` est un **comportement indéfini** qui provoque généralement un `Segmentation fault` :

```cpp
int* p = nullptr;
// *p = 10;          // 💥 Segmentation fault (ou pire : corruption silencieuse)
```

La bonne pratique est de toujours vérifier qu'un pointeur n'est pas nul avant de le déréférencer, quand sa valeur n'est pas garantie par le contexte :

```cpp
void utiliser(int* p) {
    if (p != nullptr) {   // ou simplement : if (p)
        std::cout << *p << "\n";
    }
}
```

---

## Pointeurs et const : quatre combinaisons

L'interaction entre pointeurs et `const` est une source de confusion fréquente. Il existe quatre combinaisons, chacune avec une sémantique distincte :

```cpp
int valeur = 10;  
int autre = 20;  

// 1. Pointeur modifiable vers donnée modifiable
int* p1 = &valeur;
*p1 = 30;            // ✅ modification de la donnée
p1 = &autre;         // ✅ réaffectation du pointeur

// 2. Pointeur modifiable vers donnée constante
const int* p2 = &valeur;    // ou : int const* p2
// *p2 = 30;         // ❌ ne peut pas modifier la donnée via p2
p2 = &autre;         // ✅ peut réaffecter le pointeur

// 3. Pointeur constant vers donnée modifiable
int* const p3 = &valeur;
*p3 = 30;            // ✅ peut modifier la donnée
// p3 = &autre;      // ❌ ne peut pas réaffecter le pointeur

// 4. Pointeur constant vers donnée constante
const int* const p4 = &valeur;
// *p4 = 30;         // ❌
// p4 = &autre;      // ❌
```

La règle de lecture est : **`const` s'applique à ce qui est immédiatement à sa gauche** (sauf s'il est tout à gauche, auquel cas il s'applique à ce qui est à sa droite). Lisez la déclaration de droite à gauche pour la décrypter :

```
const int* p       →  p est un pointeur vers un int constant  
int const* p       →  p est un pointeur vers un const int    (identique)  
int* const p       →  p est un pointeur constant vers un int  
const int* const p →  p est un pointeur constant vers un int constant  
```

La forme `const int*` (pointeur vers donnée constante) est de loin la plus utilisée. C'est celle que vous rencontrez quand une fonction promet de ne pas modifier les données qu'on lui passe :

```cpp
// Le const garantit que la fonction ne modifiera pas le contenu du tableau
double moyenne(const double* donnees, int taille) {
    double somme = 0.0;
    for (int i = 0; i < taille; ++i) {
        somme += donnees[i];
    }
    return somme / taille;
}
```

---

## Limites de l'arithmétique : comportement indéfini

L'arithmétique des pointeurs n'est valide que **dans les bornes d'un tableau** (plus un élément *past-the-end*). Tout accès en dehors de ces bornes est un comportement indéfini, même si l'adresse calculée correspond à de la mémoire valide :

```cpp
int tableau[5] = {10, 20, 30, 40, 50};  
int* p = tableau;  

*(p + 4);    // ✅ dernier élément (tableau[4])
*(p + 5);    // ❌ comportement indéfini — un au-delà du past-the-end
*(p - 1);    // ❌ comportement indéfini — avant le début du tableau
```

Le pointeur *past-the-end* (`p + 5` dans cet exemple) est valide pour la **comparaison** mais pas pour le **déréférencement** :

```cpp
int* fin = tableau + 5;

// ✅ Comparaison avec past-the-end — base de tout parcours
for (int* it = tableau; it != fin; ++it) {
    std::cout << *it << " ";
}

// ❌ Déréférencement du past-the-end — comportement indéfini
// std::cout << *fin;
```

De même, l'arithmétique entre pointeurs qui ne pointent pas dans le même tableau est un comportement indéfini :

```cpp
int a[10];  
int b[10];  

int* pa = a + 5;  
int* pb = b + 3;  

// ptrdiff_t diff = pa - pb;  // ❌ comportement indéfini — tableaux différents
```

Ces règles peuvent sembler restrictives, mais elles permettent au compilateur d'effectuer des optimisations agressives. En pratique, quand vous utilisez des conteneurs STL et des itérateurs, ces limites sont gérées pour vous.

---

## Pourquoi comprendre tout cela en C++ moderne ?

Vous pourriez vous demander pourquoi consacrer autant d'espace à l'arithmétique des pointeurs alors que le C++ moderne offre `std::vector`, `std::span`, les itérateurs et les ranges. La réponse tient en trois points.

**L'interopérabilité C.** De très nombreuses API système, bibliothèques réseau, drivers et composants legacy exposent des interfaces en pointeurs bruts. Chaque appel à `read()`, `write()`, `memcpy()`, chaque callback POSIX manipule des pointeurs. Comprendre l'arithmétique des pointeurs est indispensable pour utiliser ces interfaces correctement.

**Le débogage.** Quand un programme plante avec un `Segmentation fault` ou qu'un sanitizer signale un *buffer overflow*, le diagnostic implique de comprendre quelles adresses sont valides, à quelle distance se trouve un pointeur de sa zone légitime, et comment la mémoire est organisée. Sans cette compréhension, les messages d'erreur de Valgrind ou d'AddressSanitizer restent opaques.

**Les fondations.** Les itérateurs de la STL **sont** de l'arithmétique de pointeurs abstraite. `std::span` **est** un pointeur plus une taille. Les smart pointers **encapsulent** un pointeur brut. Comprendre le mécanisme de base rend toutes ces abstractions transparentes plutôt que magiques.

Le C++ moderne ne supprime pas les pointeurs — il les enrobe dans des abstractions sûres. Mais quand l'abstraction fuit (et elle finit toujours par fuir), c'est la compréhension du niveau bas qui fait la différence.

⏭️ [Dangers : Memory leaks, dangling pointers, double free](/05-gestion-memoire/04-dangers-memoire.md)
