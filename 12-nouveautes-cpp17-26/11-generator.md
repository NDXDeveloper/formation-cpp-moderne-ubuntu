🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 12.11 std::generator (C++23) : Coroutines simplifiées

## Le chaînon manquant des coroutines

La section 12.6 a présenté les coroutines C++20 — leur puissance et leur complexité. Le constat était clair : écrire un simple générateur nécessitait environ 50 lignes de boilerplate (`promise_type`, `coroutine_handle`, destructeur, sémantique de déplacement). Ce coût d'entrée décourageait l'adoption pour le cas d'usage pourtant le plus naturel et le plus immédiat des coroutines : produire une séquence de valeurs à la demande.

C++23 corrige ce problème avec `std::generator<T>` (header `<generator>`). C'est un type standard prêt à l'emploi qui encapsule toute la machinerie coroutine et s'intègre nativement dans l'écosystème des Ranges. Écrire un générateur devient aussi simple qu'écrire une fonction — il suffit d'utiliser `co_yield`.

> 📎 *Pour les fondamentaux des coroutines (co_yield, co_await, co_return, promise_type, coroutine_handle), voir section 12.6. Cette section se concentre sur l'utilisation pratique de `std::generator`.*

## De l'implémentation manuelle au standard

### Le contraste

Rappelons le générateur Fibonacci implémenté manuellement en section 12.6 — environ 50 lignes de `promise_type`, de gestion du `coroutine_handle`, de destructeur et d'opérateurs move. Voici la version C++23 :

```cpp
#include <generator>
#include <print>
#include <ranges>

std::generator<int> fibonacci() {
    int a = 0, b = 1;
    while (true) {
        co_yield a;
        auto next = a + b;
        a = b;
        b = next;
    }
}

int main() {
    for (int n : fibonacci() | std::views::take(10)) {
        std::print("{} ", n);
    }
    // 0 1 1 2 3 5 8 13 21 34
}
```

Cinq lignes pour le corps du générateur. Pas de `promise_type` à écrire. Pas de `coroutine_handle` à gérer. Pas de destructeur personnalisé. Le type `std::generator<int>` gère tout : l'allocation du coroutine frame, la suspension, la reprise, la destruction, et l'interface d'itération.

### std::generator est un range

La propriété la plus importante de `std::generator` est qu'il satisfait le concept `std::ranges::input_range`. Cela signifie qu'il est directement utilisable dans les range-based for loops, avec l'opérateur pipe `|`, et avec tous les algorithmes et views de la bibliothèque Ranges (section 12.5) :

```cpp
#include <generator>
#include <ranges>
#include <algorithm>
#include <print>

std::generator<int> naturals(int start = 1) {
    while (true) {
        co_yield start++;
    }
}

// Combinaison avec les views — pipeline complet
auto result = naturals()
    | std::views::filter([](int n) { return n % 3 == 0; })   // multiples de 3
    | std::views::transform([](int n) { return n * n; })      // au carré
    | std::views::take(5);                                     // 5 premiers

for (int n : result) {
    std::print("{} ", n);
}
// 9 36 81 144 225
```

Cette intégration avec les Ranges est ce qui rend `std::generator` véritablement puissant. Un générateur n'est pas un objet isolé — c'est une source de données qui s'insère dans des pipelines de transformation paresseux.

## Générateurs courants

### Séquences numériques

```cpp
#include <generator>
#include <cmath>

// Puissances successives
std::generator<double> powers(double base) {
    double value = 1.0;
    while (true) {
        co_yield value;
        value *= base;
    }
}
// powers(2.0) → 1, 2, 4, 8, 16, 32, ...

// Nombres premiers (crible incrémental simplifié)
std::generator<int> primes() {
    co_yield 2;
    for (int n = 3; ; n += 2) {
        bool is_prime = true;
        for (int d = 3; d * d <= n; d += 2) {
            if (n % d == 0) { is_prime = false; break; }
        }
        if (is_prime) co_yield n;
    }
}
// primes() → 2, 3, 5, 7, 11, 13, 17, ...
```

La boucle infinie n'est pas un problème : le générateur ne calcule le prochain nombre premier que lorsqu'un consommateur le demande.

### Lecture paresseuse de fichiers

```cpp
#include <generator>
#include <fstream>
#include <string>

std::generator<std::string> read_lines(const std::string& path) {
    std::ifstream file(path);
    std::string line;
    while (std::getline(file, line)) {
        co_yield std::move(line);
    }
    // Le fichier est fermé automatiquement (RAII) quand le générateur est détruit
}

// Utilisation : traiter un fichier de log ligne par ligne sans tout charger en mémoire
for (const auto& line : read_lines("/var/log/app.log") 
                         | std::views::filter(contains_error)
                         | std::views::take(100)) {
    process(line);
}
```

Le fichier est lu paresseusement — une ligne à la fois. Si `take(100)` coupe le pipeline après 100 lignes d'erreur trouvées, le reste du fichier n'est jamais lu. C'est particulièrement avantageux pour les fichiers volumineux.

### Parcours d'arbres et de graphes

Les générateurs simplifient considérablement les parcours de structures arborescentes qui nécessiteraient autrement une pile explicite ou une récursion avec callbacks :

```cpp
#include <generator>

struct TreeNode {
    int value;
    TreeNode* left = nullptr;
    TreeNode* right = nullptr;
};

// Parcours in-order — aussi simple qu'une récursion classique
std::generator<int> inorder(TreeNode* node) {
    if (!node) co_return;
    for (int v : inorder(node->left))  co_yield v;   // voir section suivante
    co_yield node->value;
    for (int v : inorder(node->right)) co_yield v;
}
```

Ce parcours produit les valeurs dans l'ordre trié. L'appelant itère sur les valeurs comme sur une simple séquence, sans connaître la structure arborescente sous-jacente. Cependant, cette approche crée un générateur imbriqué à chaque nœud. C++23 offre un mécanisme plus efficace pour ce cas : `std::ranges::elements_of`.

## Générateurs récursifs avec elements_of

Le pattern de parcours d'arbre ci-dessus fonctionne mais a un coût : chaque `for (int v : inorder(child))` crée un générateur intermédiaire, itère dessus, et fait un `co_yield` pour chaque élément. Pour un arbre de profondeur d, chaque valeur traverse d générateurs imbriqués — un coût O(d) par élément, soit O(n × d) au total.

`std::ranges::elements_of` résout ce problème en permettant de **déléguer** directement à un sous-générateur, sans l'overhead de l'itération intermédiaire :

```cpp
#include <generator>
#include <ranges>

std::generator<int> inorder(TreeNode* node) {
    if (!node) co_return;
    co_yield std::ranges::elements_of(inorder(node->left));
    co_yield node->value;
    co_yield std::ranges::elements_of(inorder(node->right));
}
```

La syntaxe `co_yield std::ranges::elements_of(range)` signifie « produire tous les éléments de ce range comme si c'étaient mes propres éléments ». Le mécanisme est implémenté au niveau du coroutine frame : plutôt que de créer une boucle d'itération, le contrôle est transféré directement au sous-générateur, qui produit ses valeurs auprès du consommateur final. C'est l'équivalent du `yield from` de Python ou du `yield*` de JavaScript.

Le gain est significatif : chaque valeur ne traverse plus qu'un seul générateur, ramenant le coût total à O(n) quelle que soit la profondeur de l'arbre.

### Exemple : parcours de système de fichiers

```cpp
#include <generator>
#include <filesystem>
#include <ranges>

namespace fs = std::filesystem;

std::generator<fs::directory_entry> walk(const fs::path& dir) {
    for (const auto& entry : fs::directory_iterator(dir)) {
        co_yield entry;
        if (entry.is_directory()) {
            co_yield std::ranges::elements_of(walk(entry.path()));
        }
    }
}

// Utilisation : trouver tous les fichiers .cpp
for (const auto& entry : walk("/home/user/project")
                          | std::views::filter([](const auto& e) {
                              return e.path().extension() == ".cpp";
                          })) {
    std::print("{}\n", entry.path().string());
}
```

Ce parcours récursif du système de fichiers est paresseux — les sous-répertoires ne sont explorés qu'au fur et à mesure de la consommation. La combinaison avec `std::views::filter` permet de ne matérialiser que les entrées qui correspondent au critère, sans construire de liste intermédiaire.

## Le type de référence : generator\<T\> vs generator\<T&\>

Par défaut, `std::generator<T>` produit des valeurs par copie. Pour les types légers (`int`, `double`, petites structs), c'est optimal. Pour les types lourds, on peut produire des références :

```cpp
#include <generator>
#include <vector>
#include <string>

// Produit des références — pas de copie des strings
std::generator<const std::string&> iterate_names(
    const std::vector<std::string>& names) 
{
    for (const auto& name : names) {
        co_yield name;   // Produit une référence, pas une copie
    }
}
```

La déclaration complète de `std::generator` est en fait :

```cpp
template <typename Ref, typename V = void, typename Allocator = void>  
class generator;  
```

Le premier paramètre `Ref` est le type de référence — ce que `co_yield` produit. Le type valeur (`value_type` de l'itérateur) est déduit. Voici les combinaisons courantes :

```cpp
std::generator<int>             // co_yield produit des int (copie)  
std::generator<const int&>      // co_yield produit des const int& (référence constante)  
std::generator<int&&>           // co_yield produit des int&& (référence rvalue)  
std::generator<std::string>     // co_yield produit des string (copie/move)  
std::generator<const std::string&>  // co_yield produit des références constantes  
```

Pour les types coûteux à copier et dont la durée de vie est garantie (par exemple, itération sur un conteneur existant), la variante par référence constante évite les copies inutiles.

## Allocateurs personnalisés

Le troisième paramètre template de `std::generator` permet de spécifier un allocateur pour le coroutine frame. Par défaut, l'allocation utilise `operator new` global, mais dans les contextes sensibles à la performance (serveurs haute fréquence, systèmes embarqués), un allocateur personnalisé peut éliminer la contention sur le heap :

```cpp
#include <generator>
#include <memory_resource>

// Générateur utilisant un allocateur polymorphique
std::generator<int, void, std::pmr::polymorphic_allocator<>>   
counting(std::pmr::memory_resource* mr) {  
    for (int i = 0; ; ++i) {
        co_yield i;
    }
}

// Utilisation avec une arena
std::pmr::monotonic_buffer_resource arena;  
auto gen = counting(&arena);  
```

En pratique, le HARE (Heap Allocation Elision, section 12.6) élimine souvent l'allocation du coroutine frame quand le générateur est consommé localement. Les allocateurs personnalisés sont surtout utiles quand le HARE ne s'applique pas — par exemple quand le générateur est stocké ou transféré entre fonctions.

## Sémantique de déplacement et durée de vie

### Un generator est move-only

`std::generator` n'est pas copiable — seulement déplaçable. C'est cohérent avec le fait qu'il encapsule un coroutine frame unique :

```cpp
auto gen = fibonacci();
// auto copy = gen;          // Erreur : pas copiable
auto moved = std::move(gen); // OK : transféré
// gen est maintenant dans un état moved-from (vide)
```

### Attention aux captures par référence

Le corps d'un générateur est une coroutine — les mises en garde de la section 12.6 sur les durées de vie s'appliquent. Si le générateur capture des variables par référence (y compris ses paramètres), ces références doivent rester valides tant que le générateur est itéré :

```cpp
std::generator<int> dangerous() {
    std::vector<int> local = {1, 2, 3, 4, 5};
    for (int& v : local) {
        co_yield v;   // OK tant que le générateur est actif
    }
    // local est détruit ici — mais le générateur suspend AVANT cette ligne,
    // donc c'est sûr : local survit tant que le générateur produit des valeurs
}

// Le vrai danger :
std::generator<const int&> wrong(const std::vector<int>& data) {
    for (const auto& v : data) {
        co_yield v;   // Référence vers data — data doit survivre !
    }
}

auto make_gen() {
    std::vector<int> temp = {1, 2, 3};
    return wrong(temp);   // DANGER : temp est détruit au retour !
}
// Le générateur retourné référence de la mémoire libérée → UB
```

La règle est la même que pour toute vue non-owning : le générateur ne possède pas les données qu'il référence. Les données doivent survivre à l'itération.

## generator vs les alternatives

### vs implémentation manuelle (promise_type)

L'implémentation manuelle offre un contrôle total (custom awaitables, allocation personnalisée sans le paramètre template, comportement de suspension personnalisé), mais au prix d'un boilerplate massif. Pour les générateurs — le cas d'usage de `co_yield` — il n'y a aucune raison de ne pas utiliser `std::generator`.

L'implémentation manuelle reste nécessaire pour les types `Task<T>` asynchrones avec `co_await`, que `std::generator` ne couvre pas. C'est le domaine de `std::execution` (section 12.14.4) et des bibliothèques comme Asio.

### vs itérateurs personnalisés

Avant les coroutines, produire une séquence paresseuse nécessitait d'écrire un itérateur personnalisé — un exercice notoirement verbeux en C++ (définir `iterator_traits`, `operator++`, `operator*`, `operator==`, les catégories d'itérateur, etc.). Les générateurs rendent ce pattern trivial :

```cpp
// Itérateur personnalisé pour les carrés parfaits : ~40-60 lignes de boilerplate
// Générateur équivalent : 5 lignes
std::generator<int> perfect_squares() {
    for (int n = 1; ; ++n) {
        co_yield n * n;
    }
}
```

Le code exprime directement l'algorithme sans être noyé dans la mécanique d'itération.

### vs std::views::iota et views standard

Pour les séquences simples (compteurs, progressions arithmétiques), les views standard comme `std::views::iota` sont préférables — elles sont plus légères qu'un générateur (pas de coroutine frame) :

```cpp
// Préférer iota pour un simple compteur
auto range = std::views::iota(1, 100);    // Plus léger qu'un générateur

// Utiliser un générateur pour une logique non triviale
std::generator<int> collatz(int n) {       // Logique complexe → générateur
    while (n != 1) {
        co_yield n;
        n = (n % 2 == 0) ? n / 2 : 3 * n + 1;
    }
    co_yield 1;
}
```

La règle de choix : si la séquence peut être exprimée par une combinaison de views standard (`iota | filter | transform`), utiliser les views. Si la logique de génération implique un état complexe, des branchements ou de la récursion, utiliser un générateur.

## Bonnes pratiques

**Utiliser `std::generator` plutôt qu'une implémentation manuelle pour tout générateur.** Le type standard est testé, optimisé, et intégré aux Ranges. Il n'y a pas de raison de réinventer ce boilerplate.

**Combiner avec les views Ranges.** La force de `std::generator` est d'être une source de données dans un pipeline. `generator | filter | transform | take` est l'idiome de base.

**Utiliser `elements_of` pour la récursion.** Pour les parcours d'arbres, de graphes, ou de systèmes de fichiers, `co_yield std::ranges::elements_of(sub_generator)` évite le coût O(profondeur) par élément.

**Produire par référence constante pour les types lourds.** `std::generator<const T&>` évite les copies quand les données sous-jacentes ont une durée de vie garantie.

**Préférer les views standard pour les séquences triviales.** `std::views::iota`, `std::views::repeat` (C++23) et les autres views de génération sont plus légères qu'un générateur pour les cas simples.

**Vérifier les durées de vie.** Un générateur par référence est une vue non-owning. Appliquer les mêmes précautions que pour `std::span` et `std::string_view` : les données référencées doivent survivre au générateur.

---

>  
> 📎 [12.6 Coroutines (C++20) : Fondamentaux](/12-nouveautes-cpp17-26/06-coroutines.md)  
>  
> 📎 [12.5 Ranges (C++20) : Pipelines fonctionnels](/12-nouveautes-cpp17-26/05-ranges.md)

⏭️ [std::stacktrace (C++23) : Traces d'exécution standard](/12-nouveautes-cpp17-26/12-stacktrace.md)
