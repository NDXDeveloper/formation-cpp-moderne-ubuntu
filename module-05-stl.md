🔝 Retour au [Sommaire](/SOMMAIRE.md)

# Module 5 — La Librairie Standard (STL)

> 🎯 Niveau : Intermédiaire

Ce module couvre les structures de données et les algorithmes de la STL — le code que vous n'avez pas à écrire vous-même. Conteneurs séquentiels (`std::vector`, `std::array`, `std::span`), conteneurs associatifs (`std::map`, `std::unordered_map`, `std::flat_map`), algorithmes classiques et parallèles, ranges (C++20), et enfin templates avec concepts (C++20) pour écrire du code générique contraint. Le choix du bon conteneur et du bon algorithme a plus d'impact sur la performance que la plupart des micro-optimisations.

---

## Objectifs pédagogiques

1. **Choisir** le conteneur STL adapté à un problème en fonction de la complexité algorithmique, de la localité mémoire et des patterns d'accès.
2. **Utiliser** `std::span` (C++20) comme vue zero-copy sur des données contiguës, en remplacement des paramètres pointeur+taille.
3. **Appliquer** les algorithmes STL (recherche, tri, transformation, manipulation) avec des lambdas et des itérateurs appropriés.
4. **Composer** des pipelines de traitement avec les ranges (C++20) et l'opérateur `|`.
5. **Paralléliser** des algorithmes STL avec les politiques d'exécution (`std::execution::par`, `par_unseq`).
6. **Implémenter** des templates de fonctions et de classes, avec spécialisation partielle/totale, variadic templates, et fold expressions.
7. **Contraindre** des templates avec des concepts (C++20), y compris la création de concepts personnalisés avec la syntaxe `requires`.

---

## Prérequis

- **Module 4, chapitre 9** : smart pointers — les conteneurs STL stockent souvent des `unique_ptr` ou `shared_ptr`, et le transfert de propriété via `std::move` est courant.
- **Module 4, chapitre 10** : move semantics — `emplace_back`, `push_back(std::move(...))`, et le comportement des conteneurs lors de la réallocation reposent sur la move semantics.
- **Module 4, chapitre 11** : lambdas — les algorithmes STL s'utilisent principalement avec des lambdas comme prédicats ou transformations.
- **Module 4, chapitre 12** : introduction aux concepts et aux ranges — ce module les approfondit en contexte.

---

## Chapitres

### Chapitre 13 — Conteneurs Séquentiels

Les conteneurs qui stockent des éléments dans un ordre déterminé par l'insertion. `std::vector` est le conteneur par défaut — les autres existent pour des cas d'usage spécifiques où `vector` est sous-optimal.

- `std::vector` : fonctionnement interne (buffer contigu, capacité vs taille, facteur de croissance), méthodes essentielles (`push_back`, `emplace_back`, `reserve`, `shrink_to_fit`), invalidation des itérateurs après réallocation.
- `std::array` : tableau de taille fixe, stocké sur la stack, zéro overhead par rapport à un C-array.
- `std::list` et `std::forward_list` : listes doublement/simplement chaînées — itérateurs jamais invalidés, mais mauvaise localité mémoire.
- `std::deque` : file à double entrée — insertion O(1) aux deux bouts, accès aléatoire O(1), mais mémoire non contiguë.
- `std::span` (C++20) : vue non-owning sur des données contiguës — span statique (`std::span<int, 5>`) vs dynamique (`std::span<int>`), remplacement du pattern `(T* ptr, size_t len)`, interopérabilité avec `vector`, `array` et C-arrays.
- Complexité algorithmique (Big O) et guide de choix du conteneur selon le pattern d'accès.

### Chapitre 14 — Conteneurs Associatifs

Les conteneurs qui associent des clés à des valeurs ou qui maintiennent un ensemble unique. Le choix entre ordered, unordered et flat dépend du volume de données, du pattern d'accès et des contraintes de localité mémoire.

- `std::map` et `std::multimap` : arbres rouge-noir, accès O(log n), clés triées — adapté aux parcours ordonnés.
- `std::unordered_map` : table de hachage, accès O(1) amorti — dépend de la qualité de la fonction de hash. Custom hash functions pour les types personnalisés.
- `std::set` et `std::unordered_set` : mêmes structures sous-jacentes que `map`/`unordered_map`, sans valeur associée.
- `std::flat_map` et `std::flat_set` (C++23) : conteneurs ordonnés à mémoire contiguë — meilleures performances que `std::map` pour les lectures fréquentes sur de petits à moyens ensembles de données grâce à la localité cache.
- Comparaison de performances : ordered vs unordered vs flat — benchmarks et critères de décision.

### Chapitre 15 — Algorithmes de la STL

Les algorithmes opèrent sur des ranges d'itérateurs et ne connaissent pas les conteneurs qui les hébergent. Ce chapitre couvre les algorithmes classiques, l'introduction des ranges (C++20), et la parallélisation.

- Recherche : `std::find`, `std::binary_search`, `std::lower_bound` — pré-conditions de tri pour les recherches binaires.
- Tri : `std::sort` (introsort), `std::stable_sort` (mergesort), `std::partial_sort`.
- Transformation : `std::transform` (map), `std::accumulate` / `std::reduce` (fold).
- Manipulation : `std::copy`, `std::move`, `std::remove_if` — attention : `std::remove_if` ne supprime pas, il déplace les éléments non-matchés vers le début et retourne un itérateur de fin.
- Itérateurs : les cinq catégories (input, output, forward, bidirectional, random_access) et leur relation avec les algorithmes.
- Ranges (C++20) : views et lazy evaluation, composition de pipelines avec `|` (`views::filter`, `views::transform`, `views::take`).
- Algorithmes parallèles : politiques d'exécution `std::execution::seq`, `par`, `par_unseq`, `unseq` — parallélisation de `std::sort`, `std::transform`, `std::reduce`. Précautions : data races, overhead de threading, limitations selon l'implémentation.

### Chapitre 16 — Templates et Métaprogrammation

Les templates sont le mécanisme de généricité de C++. Ce chapitre couvre les templates classiques, les techniques avancées (SFINAE, variadic templates), et leur remplacement moderne par les concepts (C++20).

- Templates de fonctions et de classes : syntaxe, déduction de type, instanciation explicite.
- Spécialisation partielle et totale : personnaliser le comportement d'un template pour des types spécifiques.
- SFINAE (Substitution Failure Is Not An Error) : mécanisme historique pour activer/désactiver des overloads selon les propriétés d'un type — messages d'erreur souvent cryptiques.
- Variadic templates (C++11) : templates avec un nombre variable de paramètres (`template<typename... Args>`), expansion de pack.
- Concepts (C++20) : syntaxe `requires`, concepts standard de la STL (`std::integral`, `std::floating_point`, `std::sortable`, `std::ranges::range`), création de concepts personnalisés — remplacement de SFINAE avec des messages d'erreur lisibles.
- Fold expressions (C++17) : réduction d'un parameter pack en une seule expression (`(args + ...)`).

---

## Points de vigilance

- **Invalidation d'itérateurs après modification d'un `std::vector`.** Un `push_back` qui déclenche une réallocation invalide tous les itérateurs, pointeurs et références vers les éléments du vector. C'est un des bugs les plus fréquents en C++. Si vous itérez sur un vector, n'insérez pas dedans. Si vous devez insérer pendant un parcours, utilisez l'idiome `erase-remove` ou travaillez par indices. Règle pratique : appelez `reserve()` si vous connaissez la taille finale.

- **`std::unordered_map` avec une fonction de hash mal distribuée.** Si trop de clés tombent dans le même bucket, les opérations passent de O(1) amorti à O(n) par bucket. Avec une hash triviale (par exemple, hasher un entier modulo une petite constante), les performances peuvent être pires qu'un `std::map`. Utilisez les hash functions de la STL pour les types standard et implémentez des hash de qualité (FNV-1a, combinaison avec `std::hash`) pour les types personnalisés.

- **Confondre `std::remove` / `std::remove_if` avec une suppression réelle.** `std::remove_if` ne réduit pas la taille du conteneur — il déplace les éléments non-matchés vers le début et retourne un itérateur vers la nouvelle fin logique. Sans appel à `erase` ensuite, les éléments "supprimés" restent dans le conteneur. L'idiome correct est `v.erase(std::remove_if(v.begin(), v.end(), pred), v.end());` — ou avec C++20, `std::erase_if(v, pred)` qui fait les deux en un.

- **SFINAE vs Concepts : savoir quand utiliser lequel.** SFINAE reste nécessaire si vous devez supporter du code pre-C++20 ou si vous maintenez une librairie qui cible C++17. Pour tout code nouveau ciblant C++20 ou ultérieur, les concepts sont supérieurs : messages d'erreur clairs, syntaxe lisible, composition avec `&&` et `||`. Ne mélangez pas SFINAE et concepts dans la même overload set — choisissez l'un ou l'autre.

---

## Compétences acquises

À l'issue de ce module, vous savez :
- Choisir le bon conteneur STL selon la complexité algorithmique et la localité mémoire, y compris les nouveaux `std::flat_map`/`std::flat_set` (C++23).
- Utiliser `std::span` comme interface de fonction pour les données contiguës, en remplacement des pointeurs bruts.
- Appliquer les algorithmes STL avec lambdas, et composer des pipelines de traitement avec les ranges (C++20).
- Paralléliser des algorithmes via `std::execution::par` en tenant compte des contraintes de thread-safety.
- Écrire des templates génériques contraints par des concepts (C++20) avec des messages d'erreur exploitables.
- Implémenter des variadic templates et des fold expressions pour du code générique à nombre variable d'arguments.

---


⏭️ [Conteneurs Séquentiels](/13-conteneurs-sequentiels/README.md)
