🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 2.6 Options de compilation critiques

> **Niveau** : Débutant  
> **Prérequis** : Sections 2.1 (Installation des compilateurs), 2.5 (Premier programme)  
> **Objectifs** : Comprendre les catégories d'options de compilation, savoir choisir les bonnes combinaisons selon le contexte (développement, débogage, release), et éviter les pièges classiques de la configuration par défaut.

---

Dans la section 2.5, nous avons compilé notre programme avec la commande la plus simple possible : `g++ main.cpp -o main`. Cette commande fonctionne, mais elle utilise les **réglages par défaut** du compilateur — et ces réglages par défaut ne sont adaptés à aucun scénario réel.

Sans options explicites, GCC compile en C++17 (depuis GCC 11), sans aucun avertissement au-delà du minimum vital, sans aucune optimisation (`-O0`), et sans informations de débogage. Autrement dit, le compilateur produit un binaire qui ne vous prévient pas de vos erreurs, qui s'exécute lentement, et qui est impossible à déboguer correctement. C'est le pire des trois mondes.

Les options de compilation ne sont pas un détail technique qu'on règle une fois et qu'on oublie. Elles constituent une **politique de qualité** qui influence directement la fiabilité de votre code, sa performance en production, et votre efficacité lors du débogage. Choisir les bonnes options au bon moment est une compétence qui distingue le développeur qui subit les bugs de celui qui les prévient.

---

## Les quatre familles d'options

Les options de compilation se regroupent en quatre catégories, chacune répondant à un besoin distinct :

**Les warnings** (section 2.6.1) transforment le compilateur en relecteur de code. GCC et Clang sont capables de détecter des centaines de constructions suspectes — variables non utilisées, conversions implicites dangereuses, comparaisons toujours vraies, comportements indéfinis probables — mais la plupart de ces avertissements sont **désactivés par défaut**. Activer les bons warnings revient à brancher un filet de sécurité gratuit.

**Les niveaux d'optimisation** (section 2.6.2) contrôlent la manière dont le compilateur transforme votre code en instructions machine. Entre `-O0` (aucune optimisation, le plus fidèle à votre code source) et `-O3` (optimisation agressive, réorganisation profonde du code), la différence de performance peut atteindre un facteur 10 ou plus sur du code numérique. Mais une optimisation plus agressive rend aussi le débogage plus difficile, car le code machine ne correspond plus ligne à ligne au code source.

**Les options de débogage** (section 2.6.3) contrôlent la quantité d'informations que le compilateur inscrit dans le binaire pour permettre à un débogueur comme GDB de faire la correspondance entre l'exécution et votre code source. Sans `-g`, le débogueur travaille à l'aveugle — il voit des adresses mémoire et des registres, mais pas vos variables ni vos lignes de code.

**La sélection du standard** (section 2.6.4) détermine quelle version du langage C++ est acceptée par le compilateur. C'est un choix qui a des implications concrètes sur les fonctionnalités disponibles : `constexpr` étendu, `std::optional`, concepts, ranges, `std::print`… Chaque version du standard apporte des outils qui simplifient le code et réduisent les erreurs.

---

## Le problème des options par défaut

Pour mesurer l'écart entre les options par défaut et une configuration raisonnable, compilons un exemple volontairement imparfait :

```cpp
// suspect.cpp
#include <iostream>

int calculer(int x) {
    int resultat;        // Non initialisé
    if (x > 0)
        resultat = x * 2;
    // Chemin manquant : que vaut resultat si x <= 0 ?
    return resultat;
}

int main() {
    unsigned int a = -1;            // Conversion signée → non signée
    int b = 3.14;                   // Troncature float → int
    
    if (a = 42) {                   // Affectation au lieu de comparaison
        std::cout << calculer(0) << std::endl;
    }

    int tableau[5];
    for (int i = 0; i <= 5; ++i) {  // Débordement : i va jusqu'à 5
        tableau[i] = i;
    }

    return 0;
}
```

Ce code contient au moins six bugs ou mauvaises pratiques : une variable non initialisée, un chemin de retour manquant, une conversion signée vers non signée, une troncature flottant vers entier, une affectation dans un `if` au lieu d'une comparaison, et un débordement de tableau. Pourtant :

```bash
g++ suspect.cpp -o suspect
```

Aucune erreur, aucun avertissement. Le compilateur produit silencieusement un binaire qui contient du **comportement indéfini** (*undefined behavior*). Ce binaire peut sembler fonctionner aujourd'hui, planter demain, ou produire des résultats subtillement faux sans aucun signe visible. C'est le scénario le plus dangereux qui soit : un code incorrect qui a l'air correct.

Maintenant, compilons avec des options raisonnables :

```bash
g++ -Wall -Wextra -Wpedantic -std=c++23 suspect.cpp -o suspect
```

Le compilateur signale immédiatement plusieurs problèmes :

```
suspect.cpp: In function 'int main()':  
suspect.cpp:12:24: warning: unsigned conversion from 'int' to 'unsigned int'  
changes value from '-1' to '4294967295' [-Wsign-conversion]  
suspect.cpp:13:19: warning: implicit conversion from 'double' to 'int'  
changes value from '3.14' to '3' [-Wfloat-conversion]  
suspect.cpp:15:11: warning: suggest parentheses around assignment used as  
truth value [-Wparentheses]  
...
```

> 💡 **Note :** Le warning `-Wmaybe-uninitialized` (pour la variable `resultat` non initialisée dans `calculer()`) ne se déclenche qu'avec l'optimisation activée (`-O1` ou plus), car il repose sur l'analyse de flux de données de l'optimiseur. Ajoutez `-O1` pour le voir apparaître.

Chaque warning pointe vers un bug réel ou potentiel. Le compilateur fait le travail de relecture que même un développeur expérimenté pourrait manquer lors d'une revue de code rapide.

---

## Profils de compilation typiques

En pratique, on ne choisit pas les options une par une à chaque compilation. On définit des **profils** adaptés à chaque phase du cycle de développement. Voici les trois profils les plus courants, que nous détaillerons dans les sous-sections :

### Profil développement

```bash
g++ -std=c++23 -Wall -Wextra -Wpedantic -Werror -g -O0 main.cpp -o main
```

Ce profil privilégie la **détection d'erreurs** et la **facilité de débogage**. Les warnings sont maximaux et traités comme des erreurs (`-Werror`), l'optimisation est désactivée pour que le code machine corresponde fidèlement au code source, et les informations de débogage sont incluses. C'est le profil à utiliser au quotidien pendant le développement.

### Profil débogage

```bash
g++ -std=c++23 -Wall -Wextra -g -ggdb3 -O0 \
    -fsanitize=address,undefined main.cpp -o main
```

Ce profil va plus loin en ajoutant les **sanitizers** : AddressSanitizer détecte les accès mémoire invalides à l'exécution, et UndefinedBehaviorSanitizer intercepte les comportements indéfinis. Le niveau de débogage maximal (`-ggdb3`) inclut les informations sur les macros. Ce profil est plus lent à l'exécution, mais il attrape des bugs invisibles autrement.

> 📎 *Les sanitizers sont traités en détail en section 5.5 (outils de détection) et section 29.4 (sanitizers avancés).*

### Profil release (production)

```bash
g++ -std=c++23 -O2 -DNDEBUG -s main.cpp -o main
```

Ce profil privilégie la **performance**. L'optimisation `-O2` (ou `-O3` selon le contexte) réorganise le code pour la vitesse. La macro `NDEBUG` désactive les `assert`. L'option `-s` strippe les symboles pour réduire la taille du binaire. Les warnings restent utiles en release, mais `-Werror` est parfois retiré pour ne pas bloquer un build en production sur un warning non critique d'un compilateur récent.

---

## Compiler avec GCC vs Clang : Les mêmes options, des diagnostics différents

Un avantage majeur de l'écosystème Linux est de disposer de deux compilateurs de premier plan. GCC et Clang acceptent la grande majorité des mêmes options (`-Wall`, `-O2`, `-std=c++23`, `-g`, etc.), mais leurs diagnostics diffèrent — et cette différence est une force :

```bash
# Compiler avec GCC
g++ -Wall -Wextra -std=c++23 suspect.cpp -o suspect_gcc 2> gcc_warnings.txt

# Compiler avec Clang
clang++ -Wall -Wextra -std=c++23 suspect.cpp -o suspect_clang 2> clang_warnings.txt
```

Clang est souvent salué pour la clarté de ses messages d'erreur : il souligne la portion de code incriminée, suggère des corrections, et produit des notes de contexte précises. GCC a considérablement progressé sur ce terrain dans les versions récentes (GCC 14+), mais les deux compilateurs détectent parfois des problèmes différents. Compiler avec les deux est une pratique recommandée, facilement automatisable en CI/CD.

> 📎 *La section 2.1.2 compare GCC et Clang en détail. La section 38.7 (matrix builds) montre comment tester automatiquement avec plusieurs compilateurs dans un pipeline CI.*

---

## Options passées au linker : le préfixe `-Wl,`

Certaines options ne s'adressent pas au compilateur mais à l'éditeur de liens. Elles sont transmises via le préfixe `-Wl,` (*pass to linker*), suivi de l'option et de sa valeur séparées par des virgules :

```bash
# Activer le RELRO complet (sécurité)
g++ main.cpp -o main -Wl,-z,relro,-z,now

# Inscrire un RUNPATH dans le binaire
g++ main.cpp -o main -Wl,-rpath,/opt/libs

# Afficher la trace du linker
g++ main.cpp -o main -Wl,--verbose
```

Ce n'est pas le cœur de cette section — les options du linker sont spécialisées — mais il est important de comprendre le mécanisme. Si vous voyez `-Wl,` dans un Makefile ou un `CMakeLists.txt`, vous saurez que l'option est destinée à `ld`, pas à `g++`.

> 📎 *Les options de sécurité du linker (RELRO, PIE, stack protector) sont couvertes en section 45.4.*

---

## Où placer ces options dans un projet réel

Dans un projet géré par CMake (ce qui sera le cas de la majorité de vos projets — voir section 26), les options de compilation ne sont pas passées manuellement sur la ligne de commande. Elles sont déclarées dans le `CMakeLists.txt` :

```cmake
# Sélection du standard
set(CMAKE_CXX_STANDARD 23)  
set(CMAKE_CXX_STANDARD_REQUIRED ON)  

# Warnings
target_compile_options(mon_projet PRIVATE
    -Wall -Wextra -Wpedantic
)

# Profils selon le type de build
# cmake -DCMAKE_BUILD_TYPE=Debug    → -g -O0
# cmake -DCMAKE_BUILD_TYPE=Release  → -O3 -DNDEBUG
```

CMake gère automatiquement les options de débogage et d'optimisation via la variable `CMAKE_BUILD_TYPE`. Vous n'avez qu'à spécifier le profil souhaité au moment de la configuration. Mais comprendre ce que chaque option fait reste indispensable : c'est vous qui décidez des warnings à activer, du standard à cibler, et des compromis entre performance et facilité de débogage.

> 💡 *L'extrait CMake ci-dessus est un aperçu simplifié. La section 26 couvre CMake en profondeur.*

---

## Plan des sous-sections

Les quatre sous-sections qui suivent détaillent chaque famille d'options :

- **Section 2.6.1 — Warnings** (`-Wall`, `-Wextra`, `-Wpedantic`, `-Werror`) : quels warnings activer, comment les comprendre, et pourquoi `-Werror` est votre meilleur allié en développement.

- **Section 2.6.2 — Optimisation** (`-O0`, `-O2`, `-O3`, `-Os`) : ce que chaque niveau active concrètement, l'impact mesurable sur les performances, et les pièges de l'optimisation agressive.

- **Section 2.6.3 — Debug** (`-g`, `-ggdb3`) : les niveaux d'information de débogage, leur impact sur la taille du binaire, et l'interaction avec les niveaux d'optimisation.

- **Section 2.6.4 — Standard** (`-std=c++17`, `-std=c++20`, `-std=c++23`, `-std=c++26`) : comment choisir le bon standard pour votre projet, l'état du support compilateur en 2026, et les extensions GNU (`-std=gnu++23`).

> **Prochaine étape** : Section 2.6.1 — Warnings, où nous verrons comment transformer le compilateur en outil de revue de code automatique.

⏭️ [Warnings : -Wall, -Wextra, -Wpedantic, -Werror](/02-toolchain-ubuntu/06.1-warnings.md)
