🔝 Retour au [Sommaire](/SOMMAIRE.md)

# Chapitre 14 : Conteneurs Associatifs

## Module 5 — La Bibliothèque Standard (STL) | Niveau Intermédiaire

---

## Vue d'ensemble

Les conteneurs séquentiels — `std::vector`, `std::array`, `std::list` — organisent leurs éléments dans un ordre linéaire déterminé par l'insertion. C'est un modèle naturel et efficace tant que l'on parcourt, empile ou accède par indice. Mais dès que la question centrale devient *« cet élément existe-t-il ? »* ou *« quelle valeur est associée à cette clé ? »*, le modèle séquentiel montre ses limites : une recherche dans un `std::vector` non trié est en **O(n)**, et même un `std::binary_search` sur un vecteur trié impose de maintenir l'ordre à chaque insertion.

Les **conteneurs associatifs** de la STL répondent à ce besoin. Ils structurent leurs données autour d'une **clé** plutôt que d'une position, et garantissent des complexités de recherche, d'insertion et de suppression nettement meilleures pour ce type d'usage.

---

## Les deux familles de conteneurs associatifs

La STL propose deux grandes familles, fondées sur des structures de données très différentes.

### Conteneurs ordonnés (basés sur des arbres)

Les conteneurs `std::map`, `std::set`, `std::multimap` et `std::multiset` reposent en interne sur des **arbres binaires de recherche équilibrés** (typiquement des Red-Black Trees). Ils maintiennent en permanence un ordre de tri sur les clés, ce qui permet non seulement la recherche en **O(log n)**, mais aussi le parcours ordonné et les requêtes par plage (`lower_bound`, `upper_bound`).

Le prix à payer est structurel : chaque nœud de l'arbre est alloué individuellement sur le heap, avec des pointeurs vers ses enfants et son parent. Cette dispersion en mémoire nuit à la localité spatiale et donc à l'efficacité du cache CPU.

### Conteneurs non ordonnés (basés sur des tables de hachage)

Introduits en C++11, `std::unordered_map`, `std::unordered_set`, `std::unordered_multimap` et `std::unordered_multiset` reposent sur des **tables de hachage**. Ils offrent une recherche en **O(1) amorti** — considérablement plus rapide que les variantes ordonnées — mais ne maintiennent aucun ordre sur les clés.

Leur performance dépend de la qualité de la fonction de hachage et du taux de remplissage (*load factor*). Dans le pire cas (collisions massives), la complexité peut dégénérer en O(n), mais en pratique, avec une fonction de hachage correcte, ce scénario est rare.

### Conteneurs *flat* (C++23) — cache-friendly ⭐

C++23 introduit `std::flat_map` et `std::flat_set`, qui combinent l'interface des conteneurs ordonnés avec un **stockage contigu en mémoire** (basé sur des `std::vector` triés en interne). Le résultat : un conteneur ordonné dont les performances en lecture rivalisent avec celles d'un vecteur trié, grâce à une bien meilleure utilisation du cache.

Ces conteneurs constituent une alternative sérieuse à `std::map` et `std::set` pour les cas d'usage où les lectures dominent largement les écritures.

---

## Tableau comparatif synthétique

| Conteneur | Structure interne | Recherche | Insertion | Ordonné | Mémoire |
|---|---|---|---|---|---|
| `std::map` | Arbre RB | O(log n) | O(log n) | Oui | Nœuds dispersés |
| `std::unordered_map` | Table de hachage | O(1) amorti | O(1) amorti | Non | Buckets + nœuds |
| `std::flat_map` (C++23) | Vecteurs triés | O(log n) | O(n) | Oui | Contiguë |
| `std::set` | Arbre RB | O(log n) | O(log n) | Oui | Nœuds dispersés |
| `std::unordered_set` | Table de hachage | O(1) amorti | O(1) amorti | Non | Buckets + nœuds |
| `std::flat_set` (C++23) | Vecteur trié | O(log n) | O(n) | Oui | Contiguë |

> Le coût d'insertion en O(n) de `std::flat_map` / `std::flat_set` provient du décalage nécessaire pour maintenir le vecteur trié. En contrepartie, les lectures bénéficient pleinement de la localité mémoire.

---

## Choisir le bon conteneur associatif

Le choix entre ces conteneurs n'est pas anodin et dépend du profil d'utilisation. Quelques repères pratiques :

**Privilégier `std::unordered_map` / `std::unordered_set` quand :**
- la recherche rapide est la priorité absolue,
- l'ordre des clés n'a aucune importance,
- la clé dispose d'une bonne fonction de hachage (types primitifs, `std::string`).

**Privilégier `std::map` / `std::set` quand :**
- le parcours ordonné des clés est nécessaire,
- les requêtes par plage (`lower_bound`, `upper_bound`) sont fréquentes,
- le profil d'accès mélange lectures et écritures de manière équilibrée.

**Privilégier `std::flat_map` / `std::flat_set` (C++23) quand :**
- le jeu de données est construit une fois puis lu de nombreuses fois (*read-heavy workload*),
- la taille du conteneur est petite à moyenne (quelques milliers d'éléments),
- la performance cache est critique (boucles serrées, traitement en batch).

---

## Map vs Set : stocker une association ou une simple appartenance

La distinction entre *map* et *set* est transversale aux trois familles. Un **set** stocke uniquement des clés — il modélise un **ensemble** au sens mathématique et répond à la question *« cet élément fait-il partie de la collection ? »*. Un **map** associe chaque clé à une **valeur** — il modélise un **dictionnaire** et répond à *« quelle valeur correspond à cette clé ? »*.

Les variantes **multi** (`std::multimap`, `std::multiset`, `std::unordered_multimap`, `std::unordered_multiset`) autorisent plusieurs éléments à partager la même clé. Leur usage est plus spécialisé — indexation d'événements par date, regroupement d'enregistrements par catégorie — mais il est important de savoir qu'ils existent.

---

## Ce que couvre ce chapitre

Ce chapitre explore chacune de ces familles en profondeur :

- **14.1** — `std::map` et `std::multimap` : fonctionnement interne basé sur les arbres Red-Black, API, itération ordonnée, et cas d'usage.
- **14.2** — `std::unordered_map` : principes des tables de hachage, gestion des collisions, écriture de fonctions de hachage personnalisées.
- **14.3** — `std::set` et `std::unordered_set` : ensembles ordonnés et non ordonnés, opérations ensemblistes.
- **14.4** — `std::flat_map` et `std::flat_set` (C++23) : le compromis cache-friendly, quand et comment les utiliser.
- **14.5** — Comparaison de performances : benchmarks concrets entre conteneurs ordonnés, non ordonnés et flat, pour guider le choix en situation réelle.

---

## Prérequis

Avant d'aborder ce chapitre, assurez-vous d'être à l'aise avec :

- les **conteneurs séquentiels** de la STL, en particulier `std::vector` (chapitre 13),
- la **complexité algorithmique** (Big O) et le choix de conteneur (section 13.6),
- les **templates** et la notion de type paramétré (chapitre 16 — au minimum les bases),
- les **itérateurs** et leur rôle dans les algorithmes STL (section 15.5),
- la **sémantique de mouvement** pour comprendre les insertions efficaces (chapitre 10).

---


⏭️ [std::map et std::multimap : Arbres binaires ordonnés](/14-conteneurs-associatifs/01-map-multimap.md)
