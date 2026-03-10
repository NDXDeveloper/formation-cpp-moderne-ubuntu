🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 4.1 — Conditionnelles et boucles

## Chapitre 4 · Structures de Contrôle et Fonctions · Niveau Débutant

---

## Introduction

Un programme qui ne fait qu'exécuter des instructions les unes après les autres, de haut en bas, sans jamais bifurquer ni revenir en arrière, n'a qu'un intérêt très limité. La puissance d'un langage de programmation réside dans sa capacité à **modifier le flux d'exécution** en fonction de conditions et à **répéter des blocs d'instructions** tant qu'un critère est satisfait.

C++ hérite de C l'ensemble classique des structures de contrôle — `if`, `else`, `switch`, `for`, `while`, `do-while` — mais les versions modernes du langage y ajoutent des variantes qui améliorent significativement la lisibilité, la sécurité et parfois même la performance du code.

---

## Ce que couvre cette section

Cette section présente les mécanismes de contrôle de flux disponibles en C++ moderne. Elle est découpée en trois sous-sections, chacune centrée sur une famille de constructions :

| Sous-section | Sujet | Ce que vous y trouverez |  
|--------------|-------|------------------------|  
| **4.1.1** | `if`, `else if`, `else` et `if constexpr` | La conditionnelle de base, l'initialisation dans le `if` (C++17), et l'évaluation de branches à la compilation avec `if constexpr` |  
| **4.1.2** | `switch` et `switch` avec initialisation (C++17) | L'aiguillage multi-cas, l'attribut `[[fallthrough]]`, et la forme avec initialisation introduite en C++17 |  
| **4.1.3** | Range-based `for` loop | La boucle `for` moderne pour itérer sur les conteneurs et les plages, comparée aux boucles classiques |

---

## Les structures de contrôle en C++ : vue d'ensemble

### Conditionnelles

Les conditionnelles permettent d'exécuter un bloc de code **uniquement si une condition est vraie**. C++ en propose deux formes principales.

La première est le bloc `if` / `else if` / `else`, adapté aux branchements à deux ou quelques chemins, surtout quand les conditions sont des expressions booléennes arbitraires. La seconde est le `switch`, plus approprié lorsqu'on compare une même valeur à un ensemble fini de constantes — typiquement un entier ou un `enum`.

C++17 a enrichi ces deux constructions d'une syntaxe avec **initialisation** qui permet de déclarer une variable locale dans la portée de la condition, évitant ainsi des variables temporaires qui polluent la portée englobante. C++17 a aussi introduit `if constexpr`, qui permet au compilateur de **résoudre une branche à la compilation** — un outil indispensable en programmation générique avec les templates.

### Boucles

Les boucles permettent de **répéter un bloc d'instructions**. C++ offre quatre formes de boucles :

La boucle `for` classique, héritée du C, se compose de trois parties : initialisation, condition et incrément. Elle reste la forme la plus flexible mais aussi la plus verbeuse. La boucle `while` évalue sa condition **avant** chaque itération et convient quand le nombre d'itérations n'est pas connu à l'avance. La boucle `do-while` évalue sa condition **après** chaque itération, garantissant au moins une exécution du corps. Enfin, le **range-based `for`**, introduit en C++11, permet d'itérer directement sur les éléments d'un conteneur, d'un tableau C, ou de toute plage compatible, sans manipuler d'index ni d'itérateur explicite.

En pratique, dans du code C++ moderne, le range-based `for` est la forme privilégiée dès que l'on parcourt un conteneur. Les boucles `for` classiques restent utilisées pour les cas où l'on a besoin d'un index explicite ou d'un pas d'itération non standard. Les boucles `while` et `do-while` apparaissent principalement dans les boucles événementielles, les lectures de flux, ou les attentes actives.

---

## Bonnes pratiques générales

Avant d'entrer dans le détail de chaque construction, voici quelques principes transversaux qui s'appliquent à toutes les structures de contrôle en C++ moderne.

**Toujours utiliser des accolades**, même pour les blocs d'une seule ligne. Omettre les accolades après un `if` ou un `for` est une source classique de bugs lors de l'ajout ultérieur d'instructions. La majorité des guides de style professionnels (Google, LLVM, C++ Core Guidelines) imposent les accolades systématiques.

```cpp
// ❌ Fragile — une modification future peut introduire un bug silencieux
if (x > 0)
    std::cout << "positif\n";

// ✅ Robuste
if (x > 0) {
    std::cout << "positif\n";
}
```

**Préférer la portée la plus étroite possible.** Les variables déclarées dans un `if` avec initialisation (C++17) ou dans l'en-tête d'un `for` n'existent que dans le bloc concerné. C'est exactement ce qu'on souhaite : limiter la visibilité d'une variable à sa durée d'utilité réelle.

```cpp
// C++17 — la variable 'it' n'existe que dans le bloc if/else
if (auto it = cache.find(key); it != cache.end()) {
    process(it->second);
} else {
    load_from_disk(key);
}
// 'it' n'est plus accessible ici — aucun risque de réutilisation accidentelle
```

**Éviter les niveaux d'imbrication profonds.** Un code avec quatre ou cinq niveaux de `if` imbriqués est difficile à lire et à maintenir. Deux techniques aident à aplatir la structure : les **retours anticipés** (*early returns*) dans les fonctions, et l'extraction de blocs complexes dans des fonctions dédiées.

```cpp
// ❌ Imbrication profonde
void process(const Request& req) {
    if (req.is_valid()) {
        if (req.has_auth()) {
            if (req.payload_size() < MAX_SIZE) {
                // logique métier...
            }
        }
    }
}

// ✅ Retours anticipés
void process(const Request& req) {
    if (!req.is_valid()) return;
    if (!req.has_auth()) return;
    if (req.payload_size() >= MAX_SIZE) return;

    // logique métier — un seul niveau d'indentation
}
```

---

## Récapitulatif des constructions

Le tableau ci-dessous synthétise les structures de contrôle disponibles en C++, leur version d'introduction et leur cas d'usage typique.

| Construction | Depuis | Usage typique |  
|-------------|--------|---------------|  
| `if` / `else` | C | Branchement conditionnel général |  
| `if` avec initialisation | C++17 | Limiter la portée d'une variable à la condition |  
| `if constexpr` | C++17 | Branchement résolu à la compilation (templates) |  
| `switch` | C | Aiguillage sur une valeur discrète (entier, enum) |  
| `switch` avec initialisation | C++17 | Idem, avec variable locale à la portée du switch |  
| `for` classique | C | Itération avec index ou pas personnalisé |  
| `while` | C | Boucle à condition pré-évaluée |  
| `do-while` | C | Boucle à condition post-évaluée (au moins 1 itération) |  
| Range-based `for` | C++11 | Parcours d'un conteneur ou d'une plage |

---

## Ce qui suit

Les trois sous-sections suivantes détaillent chacune de ces constructions avec des exemples compilables, les pièges à éviter et les idiomes modernes recommandés. On commence par la conditionnelle la plus fondamentale : le bloc `if` / `else` et sa variante compile-time `if constexpr`.

---

> 💡 **Conseil** — Si vous venez du C ou d'un langage comme Java, la plupart de ces constructions vous seront familières dans leur forme de base. Concentrez votre attention sur les ajouts C++17 (initialisation dans `if`/`switch`, `if constexpr`) et sur le range-based `for` : ce sont eux qui distinguent le code C++ moderne du code « C avec des classes ».

⏭️ [if, else if, else et if constexpr](/04-structures-controle-fonctions/01.1-if-else.md)
