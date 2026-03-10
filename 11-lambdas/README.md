🔝 Retour au [Sommaire](/SOMMAIRE.md)

# Chapitre 11 — Programmation Fonctionnelle et Lambdas

## Introduction

Les lambdas, introduites en C++11, ont profondément transformé la façon d'écrire du C++ moderne. Avant leur apparition, exprimer une logique locale — un critère de tri, un filtre, un callback — nécessitait de définir un **foncteur** (une classe avec `operator()` surchargé) ou de recourir à un pointeur de fonction. Le code résultant était verbeux, souvent éloigné de son contexte d'utilisation, et difficile à maintenir.

Avec les lambdas, le C++ a adopté un paradigme bien connu dans d'autres langages : la **programmation fonctionnelle**. L'idée centrale est simple mais puissante — traiter les fonctions comme des **valeurs**. On peut les créer à la volée, les stocker dans des variables, les passer en argument à d'autres fonctions, et les retourner comme résultat.

---

## Ce que ce chapitre couvre

Ce chapitre explore les lambdas et les techniques de programmation fonctionnelle en C++ selon quatre axes complémentaires.

La **section 11.1** pose les fondations : la syntaxe d'une lambda, le mécanisme de capture, et les différentes stratégies pour capturer le contexte environnant (par valeur, par référence, captures mixtes, init captures). Comprendre les captures est essentiel — c'est ce qui distingue une lambda d'un simple pointeur de fonction et ce qui en fait un outil aussi flexible.

La **section 11.2** introduit les lambdas génériques apparues en C++14, puis les lambdas templatées de C++20. Ces évolutions permettent d'écrire des lambdas qui fonctionnent avec n'importe quel type, à la manière des templates, tout en conservant la concision d'une expression inline.

La **section 11.3** met les lambdas en pratique avec les algorithmes de la STL. C'est leur terrain de jeu naturel : `std::sort`, `std::transform`, `std::find_if`, `std::remove_if` et bien d'autres prennent tout leur sens lorsqu'on peut exprimer le comportement souhaité directement au point d'appel.

Enfin, la **section 11.4** aborde `std::function` et la notion plus large de *callable objects*. On y verra comment stocker une lambda dans une variable typée de façon uniforme, les implications en termes de performance (type erasure, allocation), et les alternatives comme les templates pour éviter ce surcoût lorsqu'il n'est pas nécessaire.

---

## Pourquoi les lambdas sont incontournables en C++ moderne

### Avant les lambdas : les foncteurs

Pour mesurer l'apport des lambdas, il faut d'abord comprendre ce qu'elles remplacent. Imaginons un tri de vecteur par ordre décroissant avant C++11 :

```cpp
// Avant C++11 : il faut définir un foncteur dédié
struct DescendingOrder {
    bool operator()(int a, int b) const {
        return a > b;
    }
};

void sort_data(std::vector<int>& data) {
    std::sort(data.begin(), data.end(), DescendingOrder{});
}
```

Le foncteur `DescendingOrder` est une classe entière pour exprimer une comparaison triviale. Le code métier est pollué par du boilerplate, et la logique de tri est physiquement séparée de son point d'utilisation.

### Avec une lambda : concision et localité

```cpp
// C++11 et au-delà : la logique reste au point d'appel
void sort_data(std::vector<int>& data) {
    std::sort(data.begin(), data.end(), [](int a, int b) {
        return a > b;
    });
}
```

La différence est immédiate : la logique de comparaison est définie **là où elle est utilisée**. Pas de classe auxiliaire, pas de nom à inventer, pas de fichier à ouvrir pour comprendre ce que fait le tri. Le code se lit de haut en bas, naturellement.

### Les lambdas capturent leur contexte

La véritable puissance des lambdas réside dans leur capacité à **capturer** des variables de leur portée englobante. C'est ce qui les rend bien plus expressives qu'un simple pointeur de fonction :

```cpp
void filter_above_threshold(const std::vector<int>& values, int threshold) {
    // La lambda capture 'threshold' depuis le contexte englobant
    auto it = std::find_if(values.begin(), values.end(), [threshold](int v) {
        return v > threshold;
    });

    if (it != values.end()) {
        std::print("Première valeur au-dessus de {} : {}\n", threshold, *it);
    }
}
```

Un pointeur de fonction ne pourrait pas accéder à `threshold` sans recourir à une variable globale ou à un mécanisme indirect. La lambda le fait naturellement, de façon type-safe et sans coût superflu.

---

## Ce que le compilateur fait réellement

Il est important de comprendre que les lambdas ne sont pas de la magie — elles sont du **sucre syntaxique** pour des foncteurs. Lorsque le compilateur rencontre une lambda, il génère une classe anonyme avec un `operator()` correspondant au corps de la lambda. Les variables capturées deviennent des membres de cette classe.

Ainsi, cette lambda :

```cpp
int factor = 3;  
auto multiply = [factor](int x) { return x * factor; };  
```

est transformée par le compilateur en quelque chose d'équivalent à :

```cpp
// Classe anonyme générée par le compilateur (simplifié)
class __lambda_unique_name {
    int factor;
public:
    __lambda_unique_name(int f) : factor(f) {}
    int operator()(int x) const { return x * factor; }
};

int factor = 3;  
auto multiply = __lambda_unique_name{factor};  
```

Cette équivalence a deux conséquences majeures. Premièrement, les lambdas n'introduisent **aucun surcoût** par rapport à un foncteur écrit manuellement — le compilateur applique les mêmes optimisations, et l'inlining est courant. Deuxièmement, chaque lambda a un **type unique** connu du compilateur seul, ce qui explique pourquoi `auto` est le moyen naturel de stocker une lambda.

---

## Évolution des lambdas à travers les standards

Les lambdas ont considérablement évolué depuis C++11, chaque standard apportant plus de flexibilité.

**C++11** a introduit les lambdas avec la syntaxe de base : captures par valeur et par référence, type de retour déduit pour les corps simples, et spécification `mutable` pour modifier les captures par valeur.

**C++14** a apporté deux améliorations majeures : les **lambdas génériques** (paramètres `auto`) qui permettent d'écrire des lambdas fonctionnant avec n'importe quel type, et les **init captures** (ou captures généralisées) qui permettent de créer de nouvelles variables dans la clause de capture, rendant possible le déplacement de ressources dans une lambda via `std::move`.

**C++17** a ajouté la possibilité de capturer `*this` par valeur (copie de l'objet englobant), utile lorsque la lambda survit à l'objet qui l'a créée.

**C++20** a introduit les **lambdas templatées** avec une syntaxe `<typename T>` explicite, les lambdas dans des contextes non évalués (utilisables avec `decltype`), et les lambdas `consteval` pour le calcul à la compilation.

**C++23** a affiné encore les cas d'usage avec des simplifications syntaxiques supplémentaires et un support accru dans les contextes `constexpr`.

Ce chapitre couvre l'ensemble de ces évolutions, en mettant l'accent sur les usages les plus courants et les pièges à éviter.

---

## Principes de programmation fonctionnelle en C++

Le C++ n'est pas un langage fonctionnel pur — c'est un langage **multiparadigme** qui intègre des éléments fonctionnels comme outils complémentaires. Les lambdas en sont le pilier, mais les principes qu'elles permettent d'appliquer vont au-delà de la simple syntaxe.

**Les fonctions comme valeurs.** Stocker un comportement dans une variable, le passer en paramètre, le retourner depuis une fonction — c'est le cœur de la programmation fonctionnelle. En C++, les lambdas, `std::function`, et les templates permettent tous cela, avec des compromis différents en termes de flexibilité et de performance.

**La composition.** Enchaîner des transformations sur des données plutôt que muter un état partagé. La bibliothèque Ranges (C++20, couverte en section 15.6) pousse ce principe encore plus loin avec les pipelines et l'opérateur `|`.

**L'immutabilité comme défaut.** Les lambdas capturent par valeur par défaut avec un `operator()` **const** — les captures ne sont pas modifiables sauf demande explicite via `mutable`. Ce choix de design encourage naturellement l'écriture de code sans effets de bord.

**Les fonctions d'ordre supérieur.** Les algorithmes de la STL sont fondamentalement des fonctions d'ordre supérieur — ils prennent un comportement (une lambda, un foncteur, un pointeur de fonction) en paramètre et l'appliquent à une collection. Maîtriser les lambdas, c'est débloquer toute la puissance de la STL.

---

## Prérequis

Ce chapitre s'appuie sur plusieurs notions abordées dans les chapitres précédents :

- **Références et passage de paramètres** (section 4.3) — comprendre la différence entre copie et référence est indispensable pour maîtriser les modes de capture.
- **Sémantique de mouvement** (chapitre 10) — les init captures avec `std::move` et le déplacement de ressources dans les lambdas supposent une bonne compréhension de `std::move` et des rvalue references.
- **Smart pointers** (chapitre 9) — de nombreux exemples de captures impliquent `std::unique_ptr` et `std::shared_ptr`, notamment pour les questions de durée de vie.
- **const et constexpr** (section 3.5) — le comportement `const` par défaut de l'`operator()` d'une lambda et l'utilisation de lambdas dans des contextes `constexpr` s'appuient sur ces concepts.

---

## Liens avec les autres chapitres

Les lambdas sont un outil transversal du C++ moderne. Leur maîtrise se prolonge naturellement dans plusieurs autres parties de la formation :

- **Chapitre 15 — Algorithmes de la STL** : les lambdas sont le compagnon naturel de `std::sort`, `std::transform`, `std::accumulate` et de l'ensemble des algorithmes standards.
- **Section 15.6 — Ranges (C++20)** : les pipelines de Ranges s'appuient massivement sur les lambdas pour définir les transformations, filtres et projections.
- **Chapitre 21 — Threads et concurrence** : les lambdas sont omniprésentes dans la programmation concurrente — comme corps de `std::thread`, `std::async`, ou callbacks dans les patterns producteur/consommateur.
- **Section 12.6 — Coroutines (C++20)** : les coroutines et les lambdas partagent des similitudes conceptuelles dans la capture de contexte et la suspension d'exécution.

---

*Ce chapitre fait partie du **Module 4 : C++ Moderne (C++11 → C++26)**, niveau intermédiaire.*

| Sous-section | Thème | Niveau |
|---|---|---|
| 11.1 | Syntaxe des lambdas et types de captures | Intermédiaire |
| 11.2 | Lambdas génériques et templatées | Intermédiaire |
| 11.3 | Utilisation avec les algorithmes STL | Intermédiaire |
| 11.4 | std::function et callable objects | Intermédiaire-Avancé |

⏭️ [Syntaxe des lambdas et types de captures](/11-lambdas/01-syntaxe-captures.md)
