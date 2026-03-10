🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 2.1 — Installation des compilateurs : GCC (g++) et LLVM (clang++)

> **Chapitre 2 · Mise en place de la Toolchain sur Ubuntu** · Niveau Débutant

---

## Introduction

Un compilateur C++ transforme votre code source — lisible par un humain — en instructions machine exécutables par le processeur. C'est la pièce maîtresse de toute toolchain. Sur Linux, deux grandes familles de compilateurs coexistent et se complètent :

- **GCC (GNU Compiler Collection)** — le compilateur historique du monde GNU/Linux, dont le front-end C++ est invoqué via la commande `g++` ;
- **LLVM / Clang** — une infrastructure de compilation modulaire plus récente, dont le front-end C++ est `clang++`.

Aucun des deux n'est « meilleur » dans l'absolu : ils ont des forces complémentaires, et la pratique professionnelle courante consiste à **compiler avec les deux** pour bénéficier de diagnostics croisés. Ce chapitre vous montre comment les installer, les faire cohabiter proprement sur Ubuntu, et choisir lequel utiliser selon le contexte.

---

## Installation de GCC (g++)

### Le méta-paquet `build-essential`

Sur Ubuntu, la manière la plus directe d'obtenir `g++` est d'installer le méta-paquet `build-essential`, qui tire en dépendances le compilateur C (`gcc`), le compilateur C++ (`g++`), la bibliothèque standard C (`libc-dev`), ainsi que `make` et `dpkg-dev` :

```bash
sudo apt update  
sudo apt install build-essential  
```

Une fois l'installation terminée, vérifiez la version installée :

```bash
g++ --version
# Output (exemple sur Ubuntu 24.04) :
# g++ (Ubuntu 13.2.0-23ubuntu4) 13.2.0
```

La version fournie par défaut dépend de la release d'Ubuntu. Sur Ubuntu 24.04 LTS, c'est GCC 13 ; sur Ubuntu 25.10, GCC 14 est le défaut. Or, pour bénéficier du meilleur support C++23 et C++26, vous voudrez souvent une version plus récente — c'est l'objet de la sous-section suivante.

### Installer une version spécifique de GCC

Le PPA **Ubuntu Toolchain** fournit les versions récentes de GCC pour les releases LTS. Voici comment installer GCC 15 (la dernière version majeure stable en mars 2026) :

```bash
# Ajout du PPA Toolchain (si nécessaire, selon votre version d'Ubuntu)
sudo add-apt-repository ppa:ubuntu-toolchain-r/test -y  
sudo apt update  

# Installation de GCC 15 (compilateur C et C++)
sudo apt install gcc-15 g++-15
```

Vérification :

```bash
g++-15 --version
# Output :
# g++-15 (Ubuntu 15.x.x-xubuntuX) 15.x.x
```

À ce stade, vous disposez de **deux versions** de g++ sur votre système : celle par défaut et la version 15. La commande `g++` pointe toujours sur l'ancienne. Pour basculer confortablement entre les deux, on utilise le mécanisme `update-alternatives`, décrit en section 2.1.1.

---

## Installation de Clang (clang++)

### Depuis les dépôts Ubuntu

Ubuntu empaquète Clang dans ses dépôts standards. Pour installer la version fournie par défaut :

```bash
sudo apt install clang
```

Comme pour GCC, la version proposée dépend de la release d'Ubuntu et peut ne pas être la plus récente.

### Depuis le dépôt officiel LLVM

Le projet LLVM maintient un dépôt APT dédié qui fournit les versions les plus récentes pour toutes les releases Ubuntu supportées. C'est la méthode recommandée pour obtenir Clang 20 :

```bash
# Téléchargement et exécution du script d'installation officiel LLVM
wget https://apt.llvm.org/llvm.sh  
chmod +x llvm.sh  
sudo ./llvm.sh 20  
```

Ce script ajoute automatiquement le dépôt LLVM approprié et installe `clang-20`, `clang++-20` ainsi que les outils associés (`clang-format-20`, `clang-tidy-20`, `lld-20`, etc.).

Vérification :

```bash
clang++-20 --version
# Output :
# Ubuntu clang version 20.x.x (...)
# Target: x86_64-pc-linux-gnu
# Thread model: posix
```

> 💡 **Astuce** — Le script `llvm.sh` accepte un argument optionnel `all` pour installer l'ensemble de la suite LLVM d'un coup :  
>  
> ```bash
> sudo ./llvm.sh 20 all
> ```  
>  
> Cela inclut notamment `lld` (linker rapide), `lldb` (debugger LLVM), `libc++` (implémentation LLVM de la bibliothèque standard) et les outils d'analyse statique. On en reparlera dans les chapitres suivants.

---

## 2.1.1 — Gestion des versions avec `update-alternatives`

### Le problème

Après avoir installé GCC 13, GCC 15 et Clang 20, vous vous retrouvez avec plusieurs exécutables :

```
/usr/bin/g++-13
/usr/bin/g++-15
/usr/bin/clang++-20
```

Or, beaucoup d'outils (CMake, scripts de build, CI) invoquent simplement `g++` ou `clang++` sans numéro de version. Le mécanisme **`update-alternatives`** d'Ubuntu (hérité de Debian) permet de contrôler vers quel exécutable concret ces noms génériques pointent, et de basculer d'une version à l'autre en une seule commande.

### Enregistrer les alternatives

Chaque appel à `update-alternatives --install` enregistre un candidat pour un nom générique donné. Le dernier nombre est la **priorité** : la plus élevée devient le choix par défaut en mode automatique.

```bash
# Enregistrer g++-13 avec priorité 10
sudo update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-13 10

# Enregistrer g++-15 avec priorité 20 (sera le défaut)
sudo update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-15 20
```

Faites de même pour `gcc` si vous compilez aussi du C :

```bash
sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-13 10  
sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-15 20  
```

Et pour Clang :

```bash
sudo update-alternatives --install /usr/bin/clang++ clang++ /usr/bin/clang++-20 10  
sudo update-alternatives --install /usr/bin/clang clang /usr/bin/clang-20 10  
```

### Basculer entre les versions

Pour choisir interactivement quelle version `g++` désigne :

```bash
sudo update-alternatives --config g++
```

Le système affiche un menu :

```
Il existe 2 choix pour l'alternative g++ (fournissant /usr/bin/g++).

  Sélection   Chemin            Priorité  État
------------------------------------------------------------
* 0            /usr/bin/g++-15   20        mode automatique
  1            /usr/bin/g++-13   10        mode manuel
  2            /usr/bin/g++-15   20        mode manuel

Appuyez sur <Entrée> pour conserver la valeur par défaut[*],  
ou tapez un numéro de sélection :  
```

Tapez le numéro souhaité et validez. La commande `g++ --version` reflètera immédiatement le changement.

### Vérification rapide

Après configuration, un résumé en une commande :

```bash
echo "=== GCC ===" && g++ --version | head -1 && \  
echo "=== Clang ===" && clang++ --version | head -1  
```

### Bonnes pratiques

Lorsque vous travaillez sur un projet précis et que vous voulez fixer le compilateur **sans modifier les alternatives système**, préférez les variables d'environnement de CMake ou les variables `CXX` et `CC` :

```bash
# Approche ponctuelle : spécifier le compilateur pour une session
export CXX=g++-15  
export CC=gcc-15  
cmake -B build -G Ninja  

# Ou directement en argument CMake
cmake -B build -G Ninja -DCMAKE_CXX_COMPILER=clang++-20 -DCMAKE_C_COMPILER=clang-20
```

Cette approche est plus explicite, reproductible et n'impacte pas les autres projets sur la machine.

---

## 2.1.2 — Comparaison GCC vs Clang : avantages et inconvénients

Le choix entre GCC et Clang n'est pas binaire. En pratique, les développeurs C++ professionnels utilisent les deux. Voici un comparatif synthétique pour guider vos choix.

### Messages d'erreur et diagnostics

C'est historiquement le domaine où Clang a pris l'avantage. Ses messages d'erreur sont réputés plus lisibles : ils indiquent précisément la colonne fautive, affichent des suggestions de correction (*fix-it hints*) et produisent des messages colorés bien structurés. GCC a considérablement rattrapé son retard dans les versions récentes (13+), mais Clang reste souvent un cran au-dessus pour la clarté des diagnostics, particulièrement sur les erreurs impliquant des templates profondément imbriqués.

Pour un débutant, cette lisibilité est un vrai atout pédagogique. **Compiler régulièrement avec Clang, même si GCC est votre compilateur principal**, est une habitude précieuse : vous obtiendrez parfois un diagnostic qui éclaire une erreur que GCC signale de manière plus cryptique.

### Performance du code généré

Les deux compilateurs produisent du code hautement optimisé, et aucun ne domine systématiquement l'autre. Les différences de performance sur du code réel sont généralement de l'ordre de quelques pourcents, dans un sens ou dans l'autre selon le type de charge.

GCC a traditionnellement un léger avantage sur certaines optimisations numériques et sur la vectorisation automatique dans des boucles complexes. Clang, de son côté, tend à mieux gérer l'inlining agressif et certaines optimisations inter-procédurales. Sur un projet donné, la seule façon de trancher est de **mesurer** (voir chapitre 35 — Benchmarking).

### Vitesse de compilation

Clang est souvent **plus rapide à compiler** que GCC, en particulier sur du code template-heavy. L'écart peut atteindre 10 à 30 % sur de gros projets, ce qui s'accumule vite en cycle de développement. Combiné avec le linker `lld` (issu du projet LLVM), la chaîne Clang+lld offre des temps de build remarquablement courts.

### Outillage et écosystème

Clang est conçu comme une infrastructure modulaire. Sa bibliothèque `libclang` et son AST structuré servent de fondation à toute une galaxie d'outils :

- **clangd** — serveur de langage (Language Server Protocol) pour l'autocomplétion et le diagnostic en temps réel dans les IDE ;
- **clang-tidy** — analyseur statique et moderniseur de code ;
- **clang-format** — formateur de code automatique ;
- **AddressSanitizer, UBSan, ThreadSanitizer, MemorySanitizer** — initialement développés dans LLVM, puis portés dans GCC.

GCC dispose aussi de ses propres outils (les sanitizers sont désormais disponibles dans GCC avec la même interface `-fsanitize=`), mais l'écosystème d'outillage de LLVM est globalement plus riche et plus intégré.

### Bibliothèque standard

Un point souvent méconnu : le compilateur et la bibliothèque standard sont deux composants distincts.

- **GCC** utilise par défaut **libstdc++**, la bibliothèque standard du projet GNU. C'est l'implémentation la plus répandue sur Linux.
- **Clang** utilise aussi libstdc++ par défaut sur Linux, mais peut être configuré pour utiliser **libc++**, l'implémentation LLVM.

En pratique sur Ubuntu, que vous compiliez avec `g++` ou `clang++`, vous utiliserez le plus souvent libstdc++. C'est ce que nous ferons dans cette formation, sauf mention contraire.

### Support des standards

Les deux compilateurs sont en compétition permanente pour implémenter les derniers standards. Historiquement, GCC a souvent été le premier à proposer un support étendu de chaque nouveau standard (il a été parmi les premiers à supporter C++11, C++14, C++17, C++20). Clang suit de près, et les écarts se mesurent désormais en mois plutôt qu'en années. En 2026, les deux couvrent la quasi-totalité de C++23 et proposent un support croissant de C++26 — ce que nous détaillons dans la section suivante.

### Tableau récapitulatif

| Critère | GCC (g++) | Clang (clang++) |
|---|---|---|
| **Messages d'erreur** | Bons (améliorés depuis GCC 13) | Excellents, plus lisibles |
| **Performance du binaire** | Excellent | Excellent |
| **Vitesse de compilation** | Bonne | Souvent plus rapide (10-30 %) |
| **Outillage (tidy, format, LSP)** | Limité | Riche (clangd, clang-tidy, clang-format) |
| **Bibliothèque standard** | libstdc++ (intégrée) | libc++ ou libstdc++ |
| **Support standards** | Souvent premier | Suit de très près |
| **Licence** | GPLv3 | Apache 2.0 avec exception LLVM |
| **Plateformes cibles** | Large (Linux, ARM, RISC-V, MIPS…) | Large, excellent support cross-compilation |

### Recommandation pratique

Pour cette formation et pour vos projets professionnels, la stratégie recommandée est :

1. **Développez avec Clang** au quotidien, pour profiter de ses diagnostics clairs, de la vitesse de compilation et de l'intégration transparente avec `clangd` dans votre IDE.
2. **Compilez et testez régulièrement avec GCC**, au minimum dans votre pipeline CI. Les deux compilateurs n'interprètent pas toujours les ambiguïtés du standard de la même façon ; un code qui compile sur les deux est un code plus portable et plus correct.
3. **Profilez avec les deux** quand la performance compte : comparez les binaires produits par `g++ -O2` et `clang++ -O2` avec un benchmark (chapitre 35) et choisissez celui qui produit le meilleur résultat pour votre cas d'usage.

---

## 2.1.3 — État 2026 : GCC 15 et Clang 20 — Nouveautés et support C++26 🔥

Le paysage des compilateurs C++ évolue rapidement. En mars 2026, les versions stables les plus récentes sont **GCC 15** et **Clang 20**. Cette section dresse un état des lieux de ce qu'elles apportent, avec un focus sur le support du standard C++26 qui vient d'être ratifié par l'ISO.

### GCC 15

GCC 15, publié au printemps 2025, est la version majeure la plus récente de la collection GNU. Parmi les évolutions notables :

- **Support C++23 quasi complet** — `std::print`, `std::expected`, `std::flat_map`, `std::flat_set`, `std::mdspan`, `std::generator` et `std::stacktrace` sont implémentés dans libstdc++. Le support langage (deducing this, `if consteval`, attributs multidimensionnels…) est achevé.
- **Support C++26 initial** — le support des *Contracts* (préconditions, postconditions, assertions de contrat) est en cours d'implémentation. La réflexion statique (*static reflection*) est encore expérimentale. Le flag `-std=c++26` (ou `-std=c++2c`) active les fonctionnalités disponibles.
- **Diagnostics améliorés** — les messages d'erreur sur les concepts et les templates ont gagné en clarté, avec des suggestions de contraintes manquantes.
- **Optimisations** — amélioration de l'auto-vectorisation, meilleur support de PGO (Profile-Guided Optimization) et LTO (Link-Time Optimization) incrémental.

Activation du standard C++26 avec GCC 15 :

```bash
g++-15 -std=c++26 -Wall -Wextra -o program program.cpp
# Ou l'alias provisoire :
g++-15 -std=c++2c -Wall -Wextra -o program program.cpp
```

### Clang 20

Clang 20, issu du cycle de release semestriel de LLVM, apporte :

- **Support C++23 complet** — toutes les fonctionnalités de langage et la majorité des ajouts de bibliothèque (via libc++) sont disponibles. Avec libstdc++ (le défaut sur Ubuntu), le support dépend de la version de libstdc++ installée.
- **Support C++26 en progression** — les *Contracts* sont implémentés de manière expérimentale. La réflexion statique avance.
- **`std::execution` (Senders/Receivers)** — l'un des ajouts majeurs de C++26 pour l'asynchronisme structuré. L'implémentation dans libc++ progresse ; côté libstdc++, c'est encore partiel.
- **Temps de compilation** — Clang 20 poursuit ses efforts d'optimisation du front-end ; les builds de projets template-heavy bénéficient de gains supplémentaires par rapport à Clang 19.
- **Outillage** — `clang-tidy-20` intègre de nouveaux *checks* pour détecter les usages de fonctionnalités dépréciées et suggérer les idiomes C++23/26.

Activation du standard C++26 avec Clang 20 :

```bash
clang++-20 -std=c++26 -Wall -Wextra -o program program.cpp
# Ou l'alias provisoire :
clang++-20 -std=c++2c -Wall -Wextra -o program program.cpp
```

### Tableau de support C++23 / C++26 (mars 2026)

Le tableau suivant donne un **aperçu simplifié** du support des fonctionnalités marquantes. Le support évolue avec chaque release mineure ; consultez les pages officielles pour un suivi à jour.

| Fonctionnalité | Standard | GCC 15 | Clang 20 |
|---|---|---|---|
| `std::print` / `std::format` | C++23 | ✅ Complet | ✅ Complet |
| `std::expected` | C++23 | ✅ Complet | ✅ Complet |
| `std::flat_map` / `std::flat_set` | C++23 | ✅ Complet | ✅ Complet |
| `std::mdspan` | C++23 | ✅ Complet | ✅ Complet |
| `std::generator` | C++23 | ✅ Complet | ✅ Complet |
| `std::stacktrace` | C++23 | ✅ Complet | ✅ Complet |
| Deducing `this` | C++23 | ✅ Complet | ✅ Complet |
| Contrats (Contracts) | C++26 | 🔶 Partiel | 🔶 Expérimental |
| Réflexion statique (Reflection) | C++26 | 🔶 Expérimental | 🔶 Expérimental |
| `std::execution` (Senders/Receivers) | C++26 | 🔶 Partiel | 🔶 Partiel |

**Légende :** ✅ Complet — 🔶 Partiel ou expérimental — ❌ Non disponible

> 🔥 **Conseil pratique** — En mars 2026, le C++23 est votre cible la plus sûre pour du code de production : les deux compilateurs le couvrent intégralement. Le C++26, bien que ratifié, n'est pas encore complètement supporté ; utilisez ses fonctionnalités dans des projets exploratoires ou derrière des *feature flags*, en attendant une couverture complète qui devrait arriver dans les prochains cycles de release.

### Suivre l'évolution du support

Deux ressources de référence pour suivre l'état du support compilateur en temps réel :

- **cppreference.com** — chaque page de fonctionnalité indique les versions minimales de GCC, Clang et MSVC qui la supportent ;
- **Compiler Explorer (godbolt.org)** — permet de tester instantanément si un extrait de code compile avec une version donnée de GCC ou Clang, directement dans le navigateur.

Nous reviendrons en détail sur le contenu de C++26 au [chapitre 12.14 — C++26 : Standard ratifié — Les grandes nouveautés](/12-nouveautes-cpp17-26/14-cpp26-overview.md).

---

## Vérification de l'installation

Avant de passer à la suite, assurez-vous que tout est en place en exécutant ce script de vérification rapide :

```bash
echo "--- Compilateurs installés ---"  
echo -n "g++     : " && g++ --version 2>/dev/null | head -1 || echo "NON INSTALLÉ"  
echo -n "g++-15  : " && g++-15 --version 2>/dev/null | head -1 || echo "NON INSTALLÉ"  
echo -n "clang++ : " && clang++ --version 2>/dev/null | head -1 || echo "NON INSTALLÉ"  
echo -n "clang++-20 : " && clang++-20 --version 2>/dev/null | head -1 || echo "NON INSTALLÉ"  

echo ""  
echo "--- Test de compilation rapide ---"  
echo '#include <iostream>  
int main() { std::cout << "Toolchain OK\n"; }' > /tmp/test_toolchain.cpp  

g++ -std=c++17 /tmp/test_toolchain.cpp -o /tmp/test_gcc && echo "GCC     : OK" || echo "GCC     : ÉCHEC"  
clang++ -std=c++17 /tmp/test_toolchain.cpp -o /tmp/test_clang && echo "Clang   : OK" || echo "Clang   : ÉCHEC"  

rm -f /tmp/test_toolchain.cpp /tmp/test_gcc /tmp/test_clang
```

Si les deux compilateurs affichent `OK`, votre installation est fonctionnelle et vous êtes prêt pour la suite.

---

## Ce qu'il faut retenir

- **Deux compilateurs valent mieux qu'un.** GCC et Clang ont des forces complémentaires. Les installer tous les deux est la norme professionnelle.
- **`update-alternatives`** permet de basculer entre versions de manière propre au niveau système. Pour un projet précis, préférez les variables `CXX`/`CC` ou les arguments CMake.
- **Clang brille** par ses diagnostics, sa vitesse de compilation et son écosystème d'outils (`clangd`, `clang-tidy`, `clang-format`).
- **GCC brille** par sa maturité, son support précoce des nouveaux standards et ses optimisations numériques.
- En **mars 2026**, le C++23 est pleinement supporté par les deux compilateurs. Le C++26 est en cours d'implémentation — les *Contracts* et la *Reflection* sont les fonctionnalités les plus attendues.

---


⏭️ [Gestion des versions avec update-alternatives](/02-toolchain-ubuntu/01.1-gestion-versions.md)
