🔝 Retour au [Sommaire](/SOMMAIRE.md)

# Chapitre 6 : Classes et Encapsulation

## Module 3 — Programmation Orientée Objet *(Niveau Débutant-Intermédiaire)*

---

## Objectifs du chapitre

Ce chapitre pose les bases de la programmation orientée objet en C++. À l'issue de cette étude, vous saurez :

- Définir des classes avec leurs membres (données) et leurs méthodes (fonctions membres).  
- Écrire les différents types de constructeurs : par défaut, paramétré, de copie et de déplacement.  
- Comprendre et appliquer le principe **RAII** (*Resource Acquisition Is Initialization*), pilier central de la gestion des ressources en C++ moderne.  
- Contrôler l'accès aux membres d'une classe grâce aux modificateurs `public`, `private` et `protected`.  
- Implémenter correctement la **Règle des 5** (*Rule of Five*), indispensable dès qu'une classe gère une ressource.

---

## Pourquoi l'encapsulation est fondamentale en C++

En C, les données et les fonctions qui les manipulent vivent séparément. Le développeur doit manuellement garantir la cohérence entre les deux : initialiser une structure avant de l'utiliser, libérer ses ressources au bon moment, s'assurer qu'aucun code extérieur ne place l'objet dans un état invalide. Cette approche fonctionne, mais elle repose entièrement sur la discipline du programmeur — et la discipline, à l'échelle d'un projet de plusieurs milliers de lignes, finit toujours par montrer ses limites.

Le C++ répond à ce problème avec la **classe**. Une classe regroupe des données et les opérations autorisées sur ces données dans une même entité. Elle définit un **invariant** : un ensemble de règles que l'objet respecte à tout moment de son existence. L'encapsulation — le fait de restreindre l'accès direct aux données internes — est le mécanisme qui protège cet invariant.

Prenons un exemple concret. Imaginez une classe `Socket` qui encapsule un descripteur de fichier réseau. Sans encapsulation, n'importe quel code pourrait modifier directement le descripteur, le fermer sans prévenir, ou tenter d'écrire sur un socket déjà fermé. Avec une classe bien conçue, le constructeur ouvre la connexion, le destructeur la ferme, et les méthodes publiques vérifient l'état du socket avant chaque opération. L'utilisateur de la classe n'a pas besoin de connaître ces détails — il utilise une interface propre et fiable.

Ce modèle s'articule autour de trois idées qui traversent tout le chapitre :

**Le cycle de vie est explicite.** Un objet naît (constructeur), vit (méthodes), et meurt (destructeur). Le compilateur garantit que ce cycle est respecté, même en cas d'exception. C'est la base du RAII, que nous étudierons en section 6.3.

**L'interface est séparée de l'implémentation.** Les utilisateurs d'une classe interagissent avec ses méthodes publiques. Les détails internes — structure des données, algorithmes, ressources sous-jacentes — restent cachés. Vous pouvez modifier l'implémentation sans impacter le code client, tant que l'interface publique reste stable.

**La copie et le déplacement sont contrôlés.** En C++, copier ou déplacer un objet a un sens précis. La Règle des 5, abordée en section 6.5, vous apprend à définir (ou à explicitement supprimer) les opérations de copie et de déplacement pour que votre classe se comporte de manière prévisible et sûre.

---

## Prérequis

Avant d'aborder ce chapitre, assurez-vous d'être à l'aise avec :

- Les **types, variables et portée** (chapitre 3) — notamment la différence entre `const`, `constexpr` et les règles de durée de vie des variables.  
- Les **fonctions et le passage de paramètres** (chapitre 4) — en particulier le passage par valeur, par référence et par référence constante.  
- La **gestion de la mémoire** (chapitre 5) — la distinction Stack vs Heap, `new`/`delete`, et les dangers des fuites mémoire et des pointeurs pendants.

Les smart pointers (`std::unique_ptr`, `std::shared_ptr`) seront traités au chapitre 9. Dans ce chapitre, nous utilisons volontairement `new`/`delete` dans certains exemples pour comprendre les mécanismes sous-jacents que les smart pointers automatisent.

---

## Plan du chapitre

| Section | Thème | Contenu clé |  
|---------|-------|-------------|  
| **6.1** | Définition de classes | Membres, méthodes, `struct` vs `class`, mot-clé `this` |  
| **6.2** | Constructeurs | Par défaut, paramétré, de copie, de déplacement, liste d'initialisation |  
| **6.3** | Destructeurs et RAII | Libération automatique, acquisition = initialisation |  
| **6.4** | Modificateurs d'accès | `public`, `private`, `protected`, `friend` |  
| **6.5** | Règle des 5 ⭐ | Les 5 opérations spéciales, `= default`, `= delete` |

---

## Fil conducteur : la classe `DynArray`

Tout au long de ce chapitre, nous construirons pas à pas une classe `DynArray` — un tableau dynamique simplifié. Ce fil conducteur a un objectif pédagogique précis : vous faire toucher du doigt les problèmes que chaque concept résout.

```cpp
// Aperçu de ce que nous construirons au fil du chapitre
class DynArray {  
public:  
    explicit DynArray(std::size_t size);   // 6.2 — Constructeur paramétré
    DynArray(const DynArray& other);       // 6.5 — Copie
    DynArray(DynArray&& other) noexcept;   // 6.5 — Déplacement
    DynArray& operator=(const DynArray& other);      // 6.5
    DynArray& operator=(DynArray&& other) noexcept;  // 6.5
    ~DynArray();                           // 6.3 — Destructeur (RAII)

    int& operator[](std::size_t index);
    std::size_t size() const;

private:                                   // 6.4 — Encapsulation
    int* data_;
    std::size_t size_;
};
```

En section 6.1, nous partirons d'une version minimale. À chaque nouvelle section, nous ajouterons un mécanisme (constructeur de copie, destructeur, etc.) en montrant d'abord le problème qu'il résout, puis la solution. À la fin du chapitre, `DynArray` sera une classe complète, correcte et conforme à la Règle des 5.

> 💡 **Note pédagogique** — `DynArray` est volontairement implémenté avec un pointeur brut (`int*`) pour rendre visibles les problèmes de gestion mémoire. En production, vous utiliseriez `std::vector<int>`, qui gère tout cela pour vous. L'objectif ici est de comprendre *comment* et *pourquoi* `std::vector` fonctionne, pas de le réinventer.

---

## Conventions utilisées dans ce chapitre

Les exemples de code ciblent **C++17** au minimum, avec des mentions explicites lorsqu'une fonctionnalité requiert C++20 ou ultérieur. La compilation peut se faire avec :

```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic -g -o programme source.cpp
```

Les noms de membres privés suivent la convention du suffixe underscore (`data_`, `size_`), conformément au *Google C++ Style Guide* et aux *C++ Core Guidelines*.

---


⏭️ [Définition de classes : Membres et méthodes](/06-classes-encapsulation/01-definition-classes.md)
