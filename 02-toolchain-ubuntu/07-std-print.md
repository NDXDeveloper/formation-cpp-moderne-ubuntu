🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 2.7 Introduction à `std::print` (C++23) : Le nouveau standard d'affichage ⭐

> **Niveau** : Débutant  
> **Prérequis** : Section 2.5 (Premier programme), Section 2.6.4 (Standard : -std=c++23)  
> **Objectifs** : Comprendre pourquoi `std::print` remplace avantageusement `std::cout` et `printf`, écrire ses premiers affichages avec la nouvelle syntaxe, et savoir compiler du code utilisant `std::print` sur Ubuntu.

---

> 💡 *Cette section est une **prise en main rapide** de `std::print` pour les débutants. La couverture approfondie du système de formatage (`std::format`, formateurs personnalisés, spécifications de format avancées) se trouve en **section 12.7** (std::print et std::format).*

---

Depuis les premières sections de cette formation, nous utilisons `std::cout` pour afficher du texte. C'est l'approche historique du C++, héritée des années 1990 et de la librairie IOStreams. Elle fonctionne, mais elle a des défauts que tout développeur C++ a rencontrés : une syntaxe verbeuse basée sur l'opérateur `<<`, un formatage pénible (manipulateurs `std::setw`, `std::setprecision`, `std::fixed`...), et des performances parfois décevantes.

L'alternative classique est `printf`, héritée du C. Sa syntaxe à chaîne de format est concise et familière, mais elle n'est pas type-safe : rien n'empêche de passer un `int` là où un `double` est attendu, et le compilateur ne peut pas toujours vérifier la cohérence entre la chaîne de format et les arguments.

C++23 résout ce dilemme avec **`std::print`** et **`std::println`** : un système d'affichage qui combine la concision de `printf`, la sécurité de type du C++, et des performances supérieures aux deux.

---

## Première rencontre

Voici à quoi ressemble `std::print` :

```cpp
// hello_print.cpp
#include <print>
#include <string>

int main() {
    std::string nom = "Ubuntu";
    int version = 24;
    double pi = 3.14159265358979;

    std::println("Bienvenue sur {} {} !", nom, version);
    std::println("Pi vaut approximativement {:.4f}", pi);
    std::println("En hexadécimal : {:#x}", 255);

    return 0;
}
```

```bash
g++ -std=c++23 -Wall -Wextra hello_print.cpp -o hello_print
./hello_print
```

```
Bienvenue sur Ubuntu 24 !  
Pi vaut approximativement 3.1416  
En hexadécimal : 0xff  
```

Pas de `<<`, pas de manipulateurs, pas de `std::endl`. Les accolades `{}` servent de marqueurs de remplacement (*placeholders*) dans la chaîne de format, et les arguments sont insérés dans l'ordre. Le type de chaque argument est détecté automatiquement — pas besoin de spécifier `%d`, `%s` ou `%f`.

---

## Pourquoi changer

Pour mesurer le progrès, comparons le même affichage avec les trois approches :

### `printf` (C)

```cpp
#include <cstdio>
#include <string>

int main() {
    std::string nom = "Ubuntu";
    int version = 24;
    double pi = 3.14159265358979;

    printf("Bienvenue sur %s %d !\n", nom.c_str(), version);
    printf("Pi vaut approximativement %.4f\n", pi);
    printf("En hexadécimal : %#x\n", 255);
}
```

`printf` est concis, mais le `nom.c_str()` est obligatoire (printf ne comprend pas `std::string`), et rien ne vous empêche d'écrire `%d` au lieu de `%f` pour `pi` — le programme compilera, mais le résultat sera un comportement indéfini.

### `std::cout` (C++ classique)

```cpp
#include <iostream>
#include <iomanip>
#include <string>

int main() {
    std::string nom = "Ubuntu";
    int version = 24;
    double pi = 3.14159265358979;

    std::cout << "Bienvenue sur " << nom << " " << version << " !" << std::endl;
    std::cout << "Pi vaut approximativement " << std::fixed << std::setprecision(4)
              << pi << std::endl;
    std::cout << "En hexadécimal : 0x" << std::hex << 255 << std::endl;
}
```

Type-safe, mais verbeux. Le formatage de `pi` nécessite deux manipulateurs (`std::fixed` et `std::setprecision`), et la chaîne est fragmentée en de multiples segments reliés par `<<`. L'affichage hexadécimal avec préfixe demande une concaténation manuelle. De plus, les manipulateurs comme `std::hex` sont **persistants** : ils modifient l'état du flux pour tous les affichages suivants, ce qui est une source de bugs subtils.

### `std::println` (C++23)

```cpp
#include <print>
#include <string>

int main() {
    std::string nom = "Ubuntu";
    int version = 24;
    double pi = 3.14159265358979;

    std::println("Bienvenue sur {} {} !", nom, version);
    std::println("Pi vaut approximativement {:.4f}", pi);
    std::println("En hexadécimal : {:#x}", 255);
}
```

Concis comme `printf`, type-safe comme `cout`, sans état global persistant. `std::string` est accepté directement dans les accolades, sans `.c_str()`. La spécification de format (`{:.4f}`, `{:#x}`) est locale à chaque placeholder — elle n'affecte pas les affichages suivants.

---

## `std::print` vs `std::println`

La librairie fournit deux fonctions :

- **`std::print`** affiche le texte formaté **sans** retour à la ligne final.
- **`std::println`** affiche le texte formaté **avec** un retour à la ligne (`\n`) automatique.

```cpp
std::print("Bonjour ");      // Pas de retour à la ligne  
std::print("tout le monde");  // Suite sur la même ligne  
std::println("!");            // Retour à la ligne à la fin  
// Affiche : Bonjour tout le monde!
```

`std::println` est l'équivalent de `std::print` suivi d'un `\n`. C'est la fonction que vous utiliserez le plus souvent, car la grande majorité des affichages se terminent par un retour à la ligne.

> 💡 *Contrairement à `std::endl`, `std::println` ajoute un simple `\n` sans forcer un flush du buffer de sortie. C'est un comportement plus performant — le flush explicite n'est presque jamais nécessaire et coûte cher en performance I/O. Voir section 12.7 pour les détails.*

---

## La syntaxe des placeholders en bref

Les accolades `{}` dans la chaîne de format acceptent une **mini-syntaxe** de spécification de format. Voici les cas les plus courants pour démarrer :

```cpp
// Remplacement simple (type détecté automatiquement)
std::println("Nom : {}", nom);  
std::println("Entier : {}", 42);  
std::println("Flottant : {}", 3.14);  
std::println("Booléen : {}", true);  

// Largeur minimale et alignement
std::println("[{:>10}]", "droite");   // [    droite]  
std::println("[{:<10}]", "gauche");   // [gauche    ]  
std::println("[{:^10}]", "centre");   // [  centre  ]  

// Formatage numérique
std::println("{:.2f}", 3.14159);      // 3.14  
std::println("{:08d}", 42);           // 00000042  
std::println("{:#b}", 42);            // 0b101010  
std::println("{:#x}", 255);           // 0xff  
std::println("{:+d}", 42);            // +42  

// Arguments positionnels (par index)
std::println("{1} précède {0}", "B", "A");  // A précède B
```

La syntaxe générale d'un placeholder est `{[index]:[fill][align][sign][#][0][width][.precision][type]}`. Elle est directement inspirée de Python et de la librairie `{fmt}`. Nous n'avons pas besoin de maîtriser tous ces détails pour l'instant — les cas ci-dessus couvrent 90 % des besoins courants.

> 📎 *La section 12.7 couvre en détail la syntaxe de formatage complète, les formateurs personnalisés pour vos propres types, et l'utilisation de `std::format` pour construire des chaînes sans les afficher.*

---

## Compiler `std::print` sur Ubuntu

`std::print` nécessite C++23 et un compilateur récent. Sur Ubuntu avec GCC 15 :

```bash
g++ -std=c++23 hello_print.cpp -o hello_print
```

C'est tout. Pas de librairie tierce à installer, pas de dépendance externe — `std::print` fait partie de la librairie standard.

Si la compilation échoue avec `fatal error: print: No such file or directory`, c'est que votre version de GCC ou de libstdc++ ne supporte pas encore `<print>`. La sous-section 2.7.3 détaille l'état du support compilateur et les alternatives.

---

## La transition dans cette formation

À partir de cette section, la formation utilise **`std::println` comme méthode d'affichage par défaut** dans tous les exemples de code. Les raisons sont les suivantes : c'est la manière idiomatique d'afficher du texte en C++ moderne, la syntaxe est plus lisible et plus concise, et c'est la compétence que le marché attend en 2026.

Vous rencontrerez encore `std::cout` dans deux contextes : le code existant que vous devrez lire et maintenir (la base installée de code C++ avec `cout` est immense), et les cas rares où l'interaction fine avec le flux de sortie est nécessaire (redirection, manipulation de l'état du flux). Mais pour l'affichage courant — messages, valeurs de débogage, sorties de programme — `std::println` est désormais le choix naturel.

---

## Plan des sous-sections

Les trois sous-sections qui suivent approfondissent cette introduction :

- **Section 2.7.1 — Syntaxe et comparaison avec `std::cout` et `printf`** : une comparaison détaillée des trois approches sur des cas variés (tableaux, nombres, alignement), pour ancrer la syntaxe et comprendre les avantages concrets de `std::print`.

- **Section 2.7.2 — Formatage type-safe et performant** : comment le mécanisme de `std::print` garantit la sécurité de type à la compilation, et pourquoi ses performances sont supérieures à celles de `cout` et souvent comparables à `printf`.

- **Section 2.7.3 — État du support compilateur (GCC 15+, Clang 19+)** : quel compilateur supporte quoi, comment vérifier le support sur votre système, et les alternatives (`{fmt}`, `std::format`) si votre compilateur n'est pas encore prêt.

> **Prochaine étape** : Section 2.7.1 — Syntaxe et comparaison avec `std::cout` et `printf`, où nous mettrons les trois approches côte à côte sur des cas concrets.

⏭️ [Syntaxe et comparaison avec std::cout et printf](/02-toolchain-ubuntu/07.1-syntaxe-comparaison.md)
