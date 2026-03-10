🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 2.5 Premier programme : Compilation manuelle et analyse

> **Niveau** : Débutant  
> **Prérequis** : Sections 1.3 (Cycle de compilation), 2.1 (Installation des compilateurs), 2.2 (Outils essentiels)  
> **Objectifs** : Compiler un programme C++ manuellement, comprendre chaque artefact produit, et acquérir les réflexes de diagnostic sur les binaires.

---

Jusqu'ici, nous avons abordé le cycle de compilation de manière théorique (section 1.3) et installé notre toolchain (sections 2.1 à 2.4). Il est temps de mettre les mains dans le terminal.

Dans la plupart des tutoriels, le premier programme C++ se résume à une commande `g++ main.cpp -o main` suivie de `./main`. Le programme affiche quelque chose, on passe au chapitre suivant. Cette approche masque l'essentiel : ce qui se passe **entre** votre code source et le binaire que vous exécutez. Or, cette compréhension vous sera indispensable le jour où vous devrez diagnostiquer une erreur de linkage, optimiser un temps de compilation, ou déboguer un crash dans une librairie partagée.

L'objectif de cette section est de parcourir le chemin complet du code source au binaire en exécution, en passant par chaque étape intermédiaire et chaque fichier produit.

---

## Ce que nous allons construire

### Un programme mono-fichier pour commencer

Créez un fichier `main.cpp` :

```cpp
// main.cpp
#include <iostream>
#include <string>
#include <cmath>

#define APP_VERSION "1.0.0"

constexpr double PI = 3.14159265358979323846;

double aire_cercle(double rayon) {
    return PI * rayon * rayon;
}

int main() {
    std::string nom = "Ubuntu";
    double rayon = 5.0;
    double aire = aire_cercle(rayon);

    std::cout << "Bienvenue sur " << nom << " !" << std::endl;
    std::cout << "Version : " << APP_VERSION << std::endl;
    std::cout << "Aire d'un cercle de rayon " << rayon
              << " = " << aire << std::endl;
    std::cout << "Vérification avec cmath : sqrt(144) = "
              << std::sqrt(144.0) << std::endl;

    return 0;
}
```

Ce programme est volontairement simple, mais il contient suffisamment d'éléments pour rendre chaque étape de la compilation observable et intéressante :

- **`#include <iostream>` et `<string>`** génèrent une expansion massive au préprocesseur — nous verrons comment un fichier de 25 lignes devient une unité de traduction de plusieurs dizaines de milliers de lignes.
- **`#define APP_VERSION`** est une macro que nous pourrons traquer dans le fichier préprocessé pour constater sa disparition.
- **`constexpr double PI`** nous permettra de voir comment le compilateur traite les constantes compile-time.
- **`aire_cercle(double)`** est une fonction suffisamment simple pour que son assembleur soit lisible, même pour un débutant.
- **`std::sqrt`** introduit une dépendance vers `libm.so`, que nous pourrons observer dans les dépendances dynamiques.
- **`std::string`** utilise l'allocation dynamique en coulisses, ce qui enrichit la table des symboles.

> 💡 *Ce programme utilise `std::cout` car c'est la sortie standard historique et universellement supportée. La section 2.7 introduit `std::print` (C++23), l'alternative moderne que nous privilégierons par la suite dans la formation.*

### Une version multi-fichiers pour aller plus loin

Un programme réel n'est jamais contenu dans un seul fichier. Pour illustrer les enjeux du linkage et de la séparation déclaration/définition, les sous-sections utiliseront aussi une architecture à plusieurs fichiers :

```cpp
// mathutils.hpp
#ifndef MATHUTILS_HPP
#define MATHUTILS_HPP

constexpr double PI = 3.14159265358979323846;

double aire_cercle(double rayon);  
double perimetre_cercle(double rayon);  

#endif // MATHUTILS_HPP
```

```cpp
// mathutils.cpp
#include "mathutils.hpp"

double aire_cercle(double rayon) {
    return PI * rayon * rayon;
}

double perimetre_cercle(double rayon) {
    return 2.0 * PI * rayon;
}
```

```cpp
// main.cpp (version multi-fichiers)
#include <iostream>
#include <string>
#include <cmath>
#include "mathutils.hpp"

#define APP_VERSION "1.0.0"

int main() {
    std::string nom = "Ubuntu";
    double rayon = 5.0;

    std::cout << "Bienvenue sur " << nom << " !" << std::endl;
    std::cout << "Version : " << APP_VERSION << std::endl;
    std::cout << "Aire d'un cercle de rayon " << rayon
              << " = " << aire_cercle(rayon) << std::endl;
    std::cout << "Périmètre : " << perimetre_cercle(rayon) << std::endl;
    std::cout << "sqrt(144) = " << std::sqrt(144.0) << std::endl;

    return 0;
}
```

Cette séparation en header (`.hpp`) et implémentation (`.cpp`) fait apparaître les concepts d'include guard, de compilation séparée, et de résolution de symboles au linkage — des sujets que la version mono-fichier masque complètement.

---

## La compilation en une commande vs. la compilation décomposée

Compilons d'abord de la manière la plus directe :

```bash
g++ main.cpp mathutils.cpp -o main
./main
```

```
Bienvenue sur Ubuntu !  
Version : 1.0.0  
Aire d'un cercle de rayon 5 = 78.5398  
Périmètre : 31.4159  
sqrt(144) = 12  
```

Le programme fonctionne. Mais cette commande unique a réalisé en coulisses **quatre étapes distinctes**, chacune produisant un type de fichier différent et pouvant échouer de manière distincte :

| Étape | Outil | Entrée | Sortie | Type d'erreur |
|-------|-------|--------|--------|---------------|
| 1. Préprocesseur | `cpp` | `.cpp` + `.hpp` | `.ii` | `#include` introuvable, macro mal formée |
| 2. Compilation | `cc1plus` | `.ii` | `.s` | Erreur de syntaxe, erreur de type |
| 3. Assemblage | `as` | `.s` | `.o` | Très rare en pratique |
| 4. Édition de liens | `ld` | `.o` + libs | exécutable | `undefined reference`, librairie manquante |

Savoir **à quelle étape** une erreur se produit est la première compétence de diagnostic. Un `#include` introuvable échoue à l'étape 1. Une erreur de syntaxe, à l'étape 2. Un `undefined reference to 'aire_cercle(double)'`, à l'étape 4 — ce qui signifie que la déclaration a été trouvée (étapes 1-2 réussies) mais que la définition compilée est absente au moment du linkage.

L'option `-save-temps` de GCC permet de conserver tous les fichiers intermédiaires en une seule commande :

```bash
g++ -save-temps main.cpp mathutils.cpp -o main  
ls -la *.ii *.s *.o main  
```

Vous obtenez tous les artefacts intermédiaires tout en produisant l'exécutable final. C'est un raccourci précieux pour l'analyse.

---

## Premier coup d'œil sur le binaire

Avant de plonger dans les détails de chaque étape, prenons la mesure de ce que nous avons produit :

```bash
file main
```

```
main: ELF 64-bit LSB pie executable, x86-64, version 1 (SYSV),  
dynamically linked, interpreter /lib64/ld-linux-x86-64.so.2,  
BuildID[sha1]=..., for GNU/Linux 3.2.0, not stripped  
```

Chaque mot de cette sortie raconte quelque chose. **ELF** est le format binaire standard de Linux. **64-bit** et **x86-64** confirment l'architecture cible. **dynamically linked** signifie que notre exécutable dépend de librairies partagées chargées au lancement — il ne contient pas lui-même le code de `std::cout` ni de `std::sqrt`. **not stripped** signifie que les symboles de débogage sont encore présents, ce qui rend l'inspection possible avec les outils que nous allons utiliser.

```bash
ls -lh main
```

L'exécutable ne pèse que quelques dizaines de Ko. C'est étonnamment petit pour un programme qui utilise `std::string`, `std::cout` et `std::sqrt`. L'explication tient en un mot : linkage dynamique. Le code de la librairie standard n'est pas dans notre binaire — il sera chargé depuis les fichiers `.so` du système au moment de l'exécution.

À titre de comparaison :

```bash
g++ -static main.cpp mathutils.cpp -o main_static  
ls -lh main main_static  
```

Le binaire statique, qui embarque tout, pèse plusieurs Mo. Nous reviendrons sur ce choix en section 2.5.3.

---

## Plan des sous-sections

Les trois sous-sections qui suivent décomposent en profondeur chaque aspect de cette chaîne :

- **Section 2.5.1 — Compilation étape par étape (`g++ -E`, `-S`, `-c`)** : nous exécuterons chaque phase isolément, examinerons en détail les fichiers intermédiaires produits (unité de traduction, assembleur, code objet), et montrerons comment l'impact des optimisations se lit directement dans l'assembleur généré.

- **Section 2.5.2 — Inspection des binaires (`nm`, `objdump`, `ldd`)** : nous apprendrons à interroger un fichier objet ou un exécutable — lire sa table de symboles, désassembler une fonction, examiner les sections ELF, et identifier les dépendances dynamiques.

- **Section 2.5.3 — Dépendances dynamiques et résolution** : nous plongerons dans le mécanisme par lequel Linux trouve et charge les librairies partagées au lancement, avec l'algorithme de recherche complet, le versioning SONAME, `LD_DEBUG` pour le diagnostic, et les stratégies de résolution des problèmes courants.

> **Prochaine étape** : Section 2.5.1 — Compilation étape par étape, où nous démonterons la commande `g++` pour comprendre chaque phase de la transformation de votre code source en code machine.

⏭️ [Compilation étape par étape (g++ -E, -S, -c)](/02-toolchain-ubuntu/05.1-compilation-etapes.md)
