🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 12.9 std::flat_map et std::flat_set (C++23) : Conteneurs ordonnés à mémoire contiguë ⭐

## Quand l'arbre binaire devient l'ennemi du cache

`std::map` et `std::set` sont des conteneurs associatifs ordonnés présents dans la STL depuis C++98. Implémentés comme des arbres rouge-noir, ils offrent des garanties algorithmiques solides : insertion, suppression et recherche en O(log n). Sur le papier, c'est excellent. En pratique, sur le matériel moderne, c'est souvent bien pire que prévu.

Le problème est la **localité mémoire**. Un arbre rouge-noir alloue chaque nœud indépendamment sur le heap. Dans un `std::map` de 10 000 éléments, les nœuds sont dispersés à travers l'espace d'adressage, séparés par des dizaines ou des centaines d'octets d'autres allocations. Chaque accès à un nœud — en suivant un pointeur gauche ou droit dans l'arbre — est potentiellement un *cache miss* : le processeur doit aller chercher des données en mémoire principale, une opération 50 à 200 fois plus lente qu'un accès au cache L1.

C++23 introduit `std::flat_map` et `std::flat_set` (header `<flat_map>` et `<flat_set>`), des conteneurs associatifs ordonnés qui stockent leurs éléments dans des **séquences contiguës triées** plutôt que dans des arbres. Le compromis est explicite : les caractéristiques algorithmiques changent (l'insertion et la suppression passent de O(log n) à O(n) dans le pire cas), mais les opérations de lecture — recherche, itération, accès — deviennent drastiquement plus rapides grâce à la localité mémoire et à l'utilisation efficace du cache CPU.

## Le principe : des vecteurs triés sous le capot

Un `std::flat_map<K, V>` stocke ses clés et ses valeurs dans deux conteneurs séquentiels séparés (par défaut, deux `std::vector`) maintenus en ordre trié :

```
std::map<string, int>                    std::flat_map<string, int>
(arbre rouge-noir)                       (vecteurs triés)

        ┌───────┐                        clés (vector<string>):
        │"Clara"│                        ┌───────┬─────┬───────┐
        │  92   │                        │"Alice"│"Bob"│"Clara"│
        └──┬──┬─┘                        └───────┴─────┴───────┘
          ╱    ╲
    ┌─────┐    ┌─────┐                   valeurs (vector<int>):
    │"Bob"│    │     │                   ┌────┬────┬────┐
    │ 87  │    │     │                   │ 95 │ 87 │ 92 │
    └──┬──┘    └─────┘                   └────┴────┴────┘
      ╱
┌───────┐                                Index 0 ↔ ("Alice", 95)
│"Alice"│                                Index 1 ↔ ("Bob", 87)
│  95   │                                Index 2 ↔ ("Clara", 92)
└───────┘

Nœuds dispersés en mémoire              Données contiguës en mémoire
→ cache misses fréquents                 → cache-friendly
```

La recherche s'effectue par **recherche binaire** sur le vecteur de clés — O(log n) comme `std::map`, mais avec des accès mémoire séquentiels et prédictibles qui exploitent pleinement les lignes de cache du processeur.

## Utilisation de base

L'API de `std::flat_map` est volontairement identique à celle de `std::map`. Dans la grande majorité des cas, le remplacement est un changement de type, rien de plus :

```cpp
#include <flat_map>
#include <string>
#include <print>

// Remplacement direct de std::map
std::flat_map<std::string, int> scores;

scores["Alice"] = 95;
scores["Bob"] = 87;
scores["Clara"] = 92;
scores.insert({"Dave", 88});
scores.emplace("Eve", 91);

// Recherche — identique à std::map
if (auto it = scores.find("Bob"); it != scores.end()) {
    std::print("Bob : {}\n", it->second);
}

// Itération — en ordre trié, comme std::map
for (const auto& [name, score] : scores) {
    std::print("{} : {}\n", name, score);
}
// Alice : 95
// Bob : 87
// Clara : 92
// Dave : 88
// Eve : 91

// contains (C++20) — fonctionne aussi
if (scores.contains("Alice")) {
    std::print("Alice trouvée\n");
}
```

`std::flat_set` suit le même principe pour les ensembles ordonnés :

```cpp
#include <flat_set>
#include <string>
#include <print>

std::flat_set<std::string> tags;
tags.insert("cpp");
tags.insert("linux");
tags.insert("devops");
tags.insert("cpp");    // Ignoré — déjà présent

for (const auto& tag : tags) {
    std::print("{} ", tag);
}
// cpp devops linux (ordre trié)
```

## Construction efficace : sorted_unique

Quand les données sont déjà triées et sans doublons (par exemple après un tri préalable ou une lecture depuis une source ordonnée), on peut construire le `flat_map` sans le coût de tri interne en utilisant le tag `std::sorted_unique` :

```cpp
#include <flat_map>
#include <vector>
#include <string>

// Données déjà triées par clé
std::vector<std::string> keys = {"Alice", "Bob", "Clara"};
std::vector<int> values = {95, 87, 92};

// Construction O(1) — pas de tri nécessaire
std::flat_map<std::string, int> scores(std::sorted_unique, 
                                        std::move(keys), 
                                        std::move(values));
```

Ce constructeur déplace les vecteurs existants sans copie ni tri. C'est l'approche la plus efficace quand on construit un `flat_map` à partir de données pré-triées — typiquement lors de la désérialisation ou de la construction en batch.

Pour `std::flat_set`, le tag équivalent est le même :

```cpp
std::vector<int> sorted_data = {1, 3, 5, 7, 9};
std::flat_set<int> s(std::sorted_unique, std::move(sorted_data));
```

## Conteneurs sous-jacents personnalisés

Par défaut, `std::flat_map<K, V>` utilise `std::vector<K>` pour les clés et `std::vector<V>` pour les valeurs. Mais le standard permet de spécifier d'autres conteneurs séquentiels :

```cpp
#include <flat_map>
#include <deque>
#include <vector>

// Utiliser un deque pour les clés et un vector pour les valeurs
std::flat_map<std::string, int, std::less<>, 
              std::deque<std::string>, 
              std::vector<int>> custom_map;

// Utiliser des small_vector, des pmr::vector, etc.
```

Cette flexibilité est utile dans les cas avancés : allocateurs personnalisés, conteneurs à taille fixe pour l'embarqué, ou conteneurs polymorphiques (`pmr::vector`) pour le contrôle fin de l'allocation.

## Différences d'API avec std::map

Bien que l'API soit largement compatible, quelques différences existent en raison de la structure interne différente.

### Invalidation des itérateurs

C'est la différence la plus importante au quotidien. Comme les données sont stockées dans des vecteurs, **toute insertion ou suppression peut invalider tous les itérateurs** — exactement comme pour `std::vector`. Avec `std::map`, seul l'itérateur de l'élément supprimé est invalidé :

```cpp
std::flat_map<std::string, int> fm = {{"a", 1}, {"b", 2}, {"c", 3}};

auto it = fm.find("b");
fm.insert({"d", 4});    // ATTENTION : it est potentiellement invalide !

// Avec std::map, it resterait valide après cette insertion
```

Cette invalidation est la contrepartie directe de la mémoire contiguë : quand le vecteur interne se réalloue, tous les pointeurs et itérateurs existants deviennent invalides.

### Stabilité des références

De même, les références et pointeurs vers les éléments ne sont pas stables après une modification du conteneur. Si du code externe conserve un `const std::string&` vers une clé, cette référence peut devenir invalide après une insertion :

```cpp
const auto& key_ref = fm.begin()->first;
fm.insert({"new_key", 42});
// key_ref est potentiellement un dangling reference
```

### Accès direct aux conteneurs sous-jacents

`std::flat_map` offre un accès direct aux conteneurs internes via `keys()` et `values()`, ce que `std::map` ne permet pas :

```cpp
std::flat_map<std::string, int> scores = {{"Alice", 95}, {"Bob", 87}, {"Clara", 92}};

// Accès en lecture aux conteneurs internes
const auto& k = scores.keys();      // const vector<string>&
const auto& v = scores.values();     // const vector<int>&

std::print("Clés : ");
for (const auto& key : k) std::print("{} ", key);
// Clés : Alice Bob Clara

// Extraction des conteneurs (transfert de propriété)
auto [extracted_keys, extracted_values] = std::move(scores).extract();
// scores est maintenant vide
// extracted_keys et extracted_values possèdent les données
```

La méthode `extract()` est particulièrement intéressante : elle permet de récupérer les vecteurs internes sans copie pour les traiter avec des algorithmes classiques, puis éventuellement de les réinjecter avec `replace()`.

### Pas de node handles

`std::map` supporte les *node handles* (C++17) qui permettent de déplacer des éléments entre conteneurs sans copie ni réallocation. Ce mécanisme n'existe pas pour les flat containers — les éléments sont stockés dans des vecteurs, pas dans des nœuds indépendants.

## Profil d'usage typique

Les flat containers excellent dans un profil d'usage bien défini — et sont moins adaptés dans d'autres. Voici le résumé :

**Idéal quand :**
- Le conteneur est construit une fois puis consulté fréquemment (profil *read-heavy*).
- La taille du conteneur est modérée (quelques centaines à quelques dizaines de milliers d'éléments).
- L'itération ordonnée est fréquente.
- La consommation mémoire doit être minimale (pas d'overhead de nœuds d'arbre).
- La sérialisation/désérialisation est importante (les vecteurs se sérialisent trivialement).

**Moins adapté quand :**
- Les insertions et suppressions sont fréquentes et intercalées avec les lectures.
- Le conteneur contient des millions d'éléments (le coût O(n) des insertions domine).
- La stabilité des itérateurs et des références est requise.
- Les éléments sont très coûteux à déplacer (types non-movable ou à move coûteux).

Ce profil correspond en pratique à un grand nombre de cas d'usage réels : tables de configuration, caches en lecture seule, index construits au démarrage, dictionnaires de traduction, registres de services. C'est pourquoi les flat containers étaient déjà populaires sous forme de bibliothèques tierces (Boost.Container, Abseil `btree_map`, Folly `sorted_vector_map`) bien avant leur standardisation.

> 📎 *La section 12.9.1 quantifie les avantages de performance avec des benchmarks comparatifs. La section 12.9.2 détaille les cas d'usage et les limites en profondeur. Voir aussi section 14.4 pour l'intégration dans le chapitre des conteneurs associatifs, et section 41.6 pour l'analyse sous l'angle de l'optimisation cache.*

## Bonnes pratiques

**Considérer `std::flat_map` comme le choix par défaut pour les conteneurs associatifs ordonnés de taille modérée et profil lecture-dominant.** Pour beaucoup de cas d'usage réels, les flat containers sont plus performants que `std::map` grâce à la localité mémoire, même si la complexité algorithmique théorique est moins favorable.

**Utiliser `sorted_unique` pour la construction en batch.** Quand les données sont disponibles en une fois (chargement de configuration, désérialisation), trier les données en amont et construire le flat_map avec `sorted_unique` élimine le surcoût de tri interne.

**Ne pas conserver d'itérateurs ou de références à travers des modifications.** C'est le piège principal. Traiter un `flat_map` comme un `vector` du point de vue de l'invalidation : après toute insertion ou suppression, considérer que tous les itérateurs et références sont invalides.

**Réserver la capacité si la taille est connue à l'avance.** Comme les conteneurs sous-jacents sont des vecteurs, un `reserve` préalable évite les réallocations lors de l'insertion :

```cpp
std::flat_map<std::string, int> fm;
// Pas d'API reserve directe, mais on peut pré-construire les vecteurs
std::vector<std::string> keys;
std::vector<int> values;
keys.reserve(1000);
values.reserve(1000);
// ... remplir et trier ...
fm = std::flat_map(std::sorted_unique, std::move(keys), std::move(values));
```

**Pour les conteneurs non-ordonnés, `std::unordered_map` reste le meilleur choix.** Les flat containers sont ordonnés. Si l'ordre n'est pas nécessaire et que la performance de lookup est prioritaire, `std::unordered_map` avec ses recherches O(1) amorti reste préférable — au prix d'une consommation mémoire plus élevée.

---

>  
> 📎 [14.4 std::flat_map et std::flat_set — Conteneurs associatifs](/14-conteneurs-associatifs/04-flat-map-flat-set.md)  
>  
> 📎 [41.6 Flat containers et performance cache](/41-optimisation-cpu-memoire/06-flat-containers-perf.md)

⏭️ [Avantages performance vs std::map](/12-nouveautes-cpp17-26/09.1-avantages-performance.md)
