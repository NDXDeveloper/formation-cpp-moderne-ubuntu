🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 1.3 — Le cycle de compilation : Préprocesseur → Compilateur → Assembleur → Linker

> **Chapitre 1 — Introduction au C++ et à l'écosystème Linux**  
> **Niveau** : Débutant  
> **Durée estimée** : 40 à 55 minutes (section principale + sous-sections)  

---

## Introduction

En Python, vous écrivez un fichier `.py` et vous l'exécutez directement. En Java, vous compilez en bytecode et une machine virtuelle l'interprète. En C++, rien de tout cela. Votre code source traverse **quatre étapes de transformation distinctes** avant de devenir un binaire exécutable par le noyau Linux. Chacune de ces étapes a un rôle précis, produit un résultat intermédiaire observable, et peut échouer de manière spécifique.

Comprendre ce pipeline n'est pas un luxe théorique. C'est une nécessité quotidienne. Un développeur C++ qui ne comprend pas le cycle de compilation est incapable de diagnostiquer correctement une erreur. Quand le compilateur affiche `undefined reference to 'foo()'`, s'agit-il d'une erreur de compilation ou d'édition de liens ? Quand une macro se comporte de façon inattendue, le problème vient-il du préprocesseur ou du code ? Quand un binaire plante au chargement avec `cannot open shared object file`, le problème est-il dans le build ou dans le déploiement ?

Cette section présente la vue d'ensemble du pipeline. Les trois sous-sections qui suivent (1.3.1, 1.3.2, 1.3.3) détaillent chaque étape en profondeur.

---

## Vue d'ensemble du pipeline

Le schéma ci-dessous résume la chaîne complète, de la première ligne de code source jusqu'au binaire exécutable :

```
  Fichier source (.cpp / .h)
         │
         ▼
  ┌───────────────────┐
  │  1. PRÉPROCESSEUR │    g++ -E main.cpp -o main.ii
  │     (cpp)         │
  └────────┬──────────┘
           │  Fichier prétraité (.ii)
           ▼
  ┌───────────────────┐
  │  2. COMPILATEUR   │    g++ -S main.ii -o main.s
  │     (cc1plus)     │
  └────────┬──────────┘
           │  Code assembleur (.s)
           ▼
  ┌───────────────────┐
  │  3. ASSEMBLEUR    │    as main.s -o main.o
  │     (as)          │
  └────────┬──────────┘
           │  Fichier objet (.o)
           ▼
  ┌───────────────────┐
  │  4. ÉDITEUR DE    │    ld main.o -o main  (simplifié)
  │     LIENS (ld)    │
  └────────┬──────────┘
           │
           ▼
    Exécutable (ELF)
```

Quand vous tapez une commande comme `g++ -o main main.cpp`, ces quatre étapes s'exécutent **en séquence, de façon transparente**. Le programme `g++` n'est pas lui-même le compilateur : c'est un **driver** (un programme orchestrateur) qui invoque chaque outil dans l'ordre, transmet les options appropriées et enchaîne les résultats. Vous pouvez à tout moment interrompre la chaîne à une étape donnée pour inspecter le résultat intermédiaire — c'est exactement ce que nous allons apprendre à faire.

---

## Les quatre étapes en bref

### Étape 1 — Le préprocesseur

Le préprocesseur est un **transformateur de texte**. Il ne comprend pas le C++ en tant que langage ; il opère uniquement sur le texte du fichier source. Son travail consiste à :

- **résoudre les `#include`** : remplacer chaque directive par le contenu intégral du fichier référencé (en-têtes système ou en-têtes du projet) ;
- **développer les macros `#define`** : substituer chaque utilisation d'une macro par son expansion textuelle ;
- **évaluer les directives conditionnelles** (`#ifdef`, `#ifndef`, `#if`, `#else`, `#endif`) : inclure ou exclure des blocs de code selon les conditions définies.

Le résultat est un fichier prétraité (extension `.ii` par convention) qui ne contient plus aucune directive préprocesseur — uniquement du code C++ pur, souvent de plusieurs dizaines de milliers de lignes car les en-têtes inclus sont volumineux.

**Flag GCC pour isoler cette étape** : `g++ -E`

```bash
g++ -E main.cpp -o main.ii
```

> 📎 *Détail complet dans la section [1.3.1 — Le préprocesseur : #include, #define, macros](./03.1-preprocesseur.md).*

### Étape 2 — Le compilateur (front-end + back-end)

Le compilateur prend le fichier prétraité et le transforme en **code assembleur** spécifique à l'architecture cible (x86_64, ARM, RISC-V…). C'est l'étape la plus complexe du pipeline, et elle se décompose elle-même en plusieurs phases internes :

- **Analyse lexicale** (*lexing*) : découpe le texte en *tokens* (mots-clés, identifiants, opérateurs, littéraux).
- **Analyse syntaxique** (*parsing*) : construit un arbre syntaxique abstrait (*AST — Abstract Syntax Tree*) qui représente la structure du programme.
- **Analyse sémantique** : vérifie la cohérence du programme (types, résolution de noms, respect des règles du langage). C'est à cette phase que la plupart des erreurs de compilation classiques sont détectées.
- **Génération de code intermédiaire** : traduit l'AST en une représentation intermédiaire (IR). GCC utilise GIMPLE puis RTL ; Clang/LLVM utilise LLVM IR.
- **Optimisation** : applique les transformations d'optimisation sur la représentation intermédiaire (élimination de code mort, inline de fonctions, déroulement de boucles, vectorisation…). Le niveau d'optimisation est contrôlé par les flags `-O0` à `-O3`.
- **Génération de code assembleur** : traduit la représentation intermédiaire optimisée en instructions assembleur pour l'architecture cible.

**Flag GCC pour isoler cette étape** : `g++ -S`

```bash
g++ -S main.cpp -o main.s
```

Le fichier `.s` résultant contient du code assembleur lisible par un humain (avec un peu de pratique).

> 📎 *Détail complet dans la section [1.3.2 — La compilation : Génération du code objet](./03.2-compilation.md).*

### Étape 3 — L'assembleur

L'assembleur traduit le code assembleur textuel (`.s`) en **code machine binaire** contenu dans un fichier objet (`.o`). Ce fichier objet est au format ELF (sur Linux) et contient :

- les instructions machine encodées en binaire (section `.text`) ;
- les données statiques initialisées (section `.data`) et non-initialisées (section `.bss`) ;
- la **table des symboles** : la liste des fonctions et variables définies dans ce fichier, ainsi que les symboles *externes* (fonctions et variables utilisées mais définies ailleurs).

Un fichier objet n'est **pas** un programme exécutable. Il peut contenir des références non résolues — des appels à des fonctions qui ne sont pas définies dans ce fichier. La résolution de ces références est le travail de l'étape suivante.

**Flag GCC pour isoler cette étape** : `g++ -c`

```bash
g++ -c main.cpp -o main.o
```

> 💡 **Note** — Le flag `-c` demande à `g++` d'exécuter les trois premières étapes (préprocesseur, compilation, assemblage) et de s'arrêter avant l'édition de liens. C'est le flag le plus couramment utilisé dans les systèmes de build comme CMake et Ninja, qui compilent chaque fichier source séparément avant de les lier ensemble.

### Étape 4 — L'éditeur de liens (linker)

L'éditeur de liens prend un ou plusieurs fichiers objet (`.o`) et les **combine en un binaire final** — un exécutable ou une bibliothèque partagée. Son rôle principal est la **résolution des symboles** : pour chaque symbole externe référencé dans un fichier objet, le linker cherche sa définition dans les autres fichiers objet ou dans les bibliothèques liées.

L'éditeur de liens sur les systèmes GNU/Linux est traditionnellement `ld` (GNU ld), mais il existe des alternatives plus rapides comme `gold` et `mold` (que nous aborderons dans le chapitre 2).

Le linker effectue aussi :

- la **fusion des sections** : les sections `.text` de tous les fichiers objet sont concaténées dans une seule section `.text` dans l'exécutable final, de même pour `.data`, `.bss`, etc. ;
- la **relocation** : ajustement des adresses internes pour refléter la position finale de chaque section dans l'exécutable ;
- la **résolution des bibliothèques** : liaison avec les bibliothèques statiques (`.a`) ou dynamiques (`.so`) spécifiées.

**Commande pour l'édition de liens** (normalement invoquée via `g++`) :

```bash
g++ main.o utils.o -lm -lpthread -o mon_programme
```

> 📎 *Détail complet dans la section [1.3.3 — L'édition de liens : Résolution des symboles](./03.3-edition-liens.md).*

---

## Démonstration complète : suivre un programme à travers le pipeline

Prenons un programme minimal et observons chaque étape de sa transformation. Créez un fichier `hello.cpp` :

```cpp
#include <cstdio>

#define MESSAGE "Bonjour depuis le pipeline de compilation !\n"

int main() {
    printf(MESSAGE);
    return 0;
}
```

> 💡 **Note** — Nous utilisons `printf` et `<cstdio>` plutôt que `std::print` ou `std::cout` dans cet exemple, car les en-têtes C sont beaucoup plus petits que les en-têtes C++. Cela rend les fichiers intermédiaires plus lisibles pour l'apprentissage. Nous passerons à `std::print` dès la section 2.7.

### Étape 1 : préprocesseur

```bash
g++ -E hello.cpp -o hello.ii  
wc -l hello.ii  
```

Résultat typique : le fichier `hello.ii` contient **plusieurs centaines de lignes** (voire plus selon la version de la libc), alors que le fichier source n'en avait que 7. Tout le contenu de `<cstdio>` a été injecté textuellement. La macro `MESSAGE` a été remplacée par sa valeur :

```cpp
// ... des centaines de lignes d'en-têtes système ...

int main() {
    printf("Bonjour depuis le pipeline de compilation !\n");
    return 0;
}
```

### Étape 2 : compilation vers l'assembleur

```bash
g++ -S hello.cpp -o hello.s  
cat hello.s  
```

Le fichier `hello.s` contient du code assembleur x86_64. Vous y trouverez des instructions comme `call`, `mov`, `ret`, ainsi que des labels et des directives pour l'assembleur. Voici un extrait simplifié (la sortie réelle varie selon le compilateur et le niveau d'optimisation) :

```asm
    .file   "hello.cpp"
    .section    .rodata
.LC0:
    .string "Bonjour depuis le pipeline de compilation !\n"
    .text
    .globl  main
main:
    pushq   %rbp
    movq    %rsp, %rbp
    leaq    .LC0(%rip), %rdi
    call    printf@PLT
    movl    $0, %eax
    popq    %rbp
    ret
```

Quelques observations : la chaîne de caractères est placée dans la section `.rodata` (read-only data). La fonction `main` est déclarée avec le label `main:` et l'attribut `.globl` (symbole visible globalement). L'appel à `printf` est marqué `@PLT` (*Procedure Linkage Table*), ce qui indique qu'il s'agit d'un appel à une fonction externe qui sera résolue au moment du chargement dynamique.

### Étape 3 : assemblage vers le fichier objet

```bash
g++ -c hello.cpp -o hello.o  
file hello.o  
```

La commande `file` affiche :

```
hello.o: ELF 64-bit LSB relocatable, x86-64, version 1 (SYSV), not stripped
```

Le mot clé ici est **relocatable** : ce fichier objet contient du code machine, mais les adresses ne sont pas encore fixées. On peut inspecter sa table des symboles :

```bash
nm hello.o
```

Sortie typique :

```
                 U printf
0000000000000000 T main
```

`T main` signifie que le symbole `main` est **défini** dans ce fichier (section `.text`). `U printf` signifie que `printf` est **utilisé mais non défini** (*Undefined*) — il devra être résolu par le linker.

### Étape 4 : édition de liens vers l'exécutable

```bash
g++ hello.o -o hello  
file hello  
```

La commande `file` affiche maintenant :

```
hello: ELF 64-bit LSB pie executable, x86-64, version 1 (SYSV),  
dynamically linked, interpreter /lib64/ld-linux-x86-64.so.2, ... not stripped  
```

Le binaire est un **exécutable** (plus un simple *relocatable*). Il est **dynamiquement lié** : il dépend de bibliothèques partagées qui seront chargées au moment de l'exécution. On peut vérifier ces dépendances :

```bash
ldd hello
```

Sortie typique :

```
linux-vdso.so.1 (0x00007ffd...)  
libc.so.6 => /lib/x86_64-linux-gnu/libc.so.6 (0x00007f...)  
/lib64/ld-linux-x86-64.so.2 (0x00007f...)
```

La dépendance sur `libc.so.6` est la bibliothèque C standard, qui contient la définition de `printf`. Le linker a résolu le symbole `printf` en le reliant à cette bibliothèque partagée.

---

## La commande unique vs les étapes séparées

En pratique, personne ne lance les quatre étapes manuellement au quotidien. La commande `g++ -o hello hello.cpp` exécute tout le pipeline d'un seul coup. Mais les systèmes de build comme **CMake** et **Ninja** procèdent différemment : ils compilent chaque fichier source séparément (`g++ -c`) pour produire des fichiers objet, puis invoquent une seule édition de liens finale. Cette approche est appelée **compilation séparée** et présente un avantage crucial : quand vous modifiez un seul fichier `.cpp`, seul ce fichier est recompilé, et le linker re-lie les fichiers objet. Les autres fichiers ne sont pas retraités.

```
Modification de utils.cpp uniquement :

  main.o  (inchangé, réutilisé)  ──┐
                                   ├──→  linker  ──→  exécutable
  utils.o (recompilé)            ──┘
```

C'est ce mécanisme qui rend la **compilation incrémentale** possible. Sur un projet de milliers de fichiers, recompiler un seul fichier prend quelques secondes au lieu de plusieurs minutes. L'outil **ccache** (section 2.3) optimise encore davantage ce processus en mettant en cache les résultats de compilation.

---

## Les erreurs à chaque étape

Un aspect pratique essentiel : chaque étape du pipeline produit des **catégories d'erreurs distinctes**. Savoir à quelle étape une erreur se produit est la première chose à déterminer quand vous déboguez un problème de build.

### Erreurs de préprocesseur

Elles surviennent quand un fichier inclus est introuvable ou quand une directive est mal formée :

```
fatal error: fichier_inexistant.h: No such file or directory
   #include "fichier_inexistant.h"
            ^~~~~~~~~~~~~~~~~~~~~~~
```

**Cause typique** : un chemin d'inclusion manquant dans les options de compilation (`-I`), un fichier d'en-tête absent ou un nom de fichier mal orthographié.

### Erreurs de compilation

Elles surviennent quand le code ne respecte pas les règles du langage C++ :

```
error: 'x' was not declared in this scope  
error: no matching function for call to 'foo(int, int)'  
error: expected ';' before '}' token  
```

**Cause typique** : erreur de syntaxe, variable non déclarée, types incompatibles, violation des contraintes de templates.

### Erreurs d'assemblage

Elles sont rares en pratique, car le compilateur génère normalement un assembleur correct. Elles peuvent survenir si vous écrivez de l'assembleur inline (`asm`) avec des erreurs de syntaxe.

### Erreurs d'édition de liens

Elles surviennent quand un symbole utilisé n'est défini nulle part, ou est défini plusieurs fois :

```
undefined reference to `foo(int)'  
multiple definition of `bar'  
```

**Cause typique** : un fichier objet ou une bibliothèque manquant dans la commande de liaison, une fonction déclarée dans un `.h` mais jamais définie dans un `.cpp`, ou un symbole défini dans plusieurs fichiers objet.

> 🔥 **Point clé** — La distinction la plus importante pour un débutant est celle entre les **erreurs de compilation** et les **erreurs d'édition de liens**. Les premières signifient que le compilateur ne comprend pas votre code. Les secondes signifient que le compilateur a compris chaque fichier individuellement, mais que le linker ne parvient pas à assembler le tout. Le message `undefined reference` est *toujours* une erreur de linker, jamais une erreur de compilation.

---

## Récapitulatif des flags et extensions

| Étape | Flag g++ | Entrée | Sortie | Extension |
|-------|----------|--------|--------|-----------|
| Préprocesseur | `-E` | `.cpp` + `.h` | Code prétraité | `.ii` |
| Compilation | `-S` | `.ii` (ou `.cpp`) | Assembleur | `.s` |
| Assemblage | `-c` | `.s` (ou `.cpp`) | Fichier objet | `.o` |
| Édition de liens | *(aucun flag spécial)* | `.o` + `.a` / `.so` | Exécutable ou `.so` | *(pas d'extension ou `.out`)* |

> 💡 **Note** — Les flags sont cumulatifs dans le sens descendant. Le flag `-c` exécute les trois premières étapes (préprocesseur + compilation + assemblage). Le flag `-S` exécute les deux premières (préprocesseur + compilation). Le flag `-E` n'exécute que le préprocesseur. L'absence de ces flags exécute les quatre étapes.

---

## Ce qu'il faut retenir

Le cycle de compilation du C++ est une chaîne de quatre transformations successives. Chaque étape produit un résultat intermédiaire que vous pouvez inspecter grâce aux flags de GCC et Clang. Les erreurs se manifestent différemment selon l'étape où elles se produisent. La compilation séparée (un fichier objet par fichier source, puis une édition de liens globale) est le fondement de tous les systèmes de build modernes.

Les trois sous-sections suivantes détaillent les aspects spécifiques de chaque étape :

- **[1.3.1 — Le préprocesseur](./03.1-preprocesseur.md)** : `#include`, `#define`, macros et compilation conditionnelle.
- **[1.3.2 — La compilation](./03.2-compilation.md)** : du code prétraité au code objet, en passant par l'analyse syntaxique, sémantique et l'optimisation.
- **[1.3.3 — L'édition de liens](./03.3-edition-liens.md)** : résolution des symboles, bibliothèques statiques et dynamiques, erreurs courantes.

---


⏭️ [Le préprocesseur : #include, #define, macros](/01-introduction-cpp-linux/03.1-preprocesseur.md)
