🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 30.1 — Valgrind : Détection de fuites et erreurs mémoire

## Introduction

Valgrind est une infrastructure d'instrumentation dynamique pour les programmes Linux. Concrètement, Valgrind exécute votre programme sur un processeur virtuel synthétique : chaque instruction machine est traduite, analysée, puis exécutée sous contrôle. Cette approche permet d'observer avec une précision remarquable tout ce qui se passe en mémoire — chaque allocation, chaque libération, chaque lecture et chaque écriture — sans modifier le code source ni recompiler le binaire.

Le projet a été créé par Julian Seward en 2002 et reste, plus de vingt ans plus tard, l'outil de référence pour l'analyse mémoire sur Linux. Son architecture modulaire repose sur un noyau d'instrumentation au-dessus duquel s'exécutent différents outils spécialisés. Le plus utilisé, **Memcheck**, est dédié à la détection d'erreurs mémoire et de fuites. D'autres outils comme **Massif** (profiling du heap, couvert en section 30.2), **Callgrind** (profiling d'appels de fonctions) ou **Helgrind** (détection de data races entre threads) exploitent la même infrastructure.

---

## Principe de fonctionnement

Valgrind ne fonctionne pas comme un débogueur classique. Là où GDB s'attache à un processus existant, Valgrind *remplace* l'environnement d'exécution du programme. Le binaire n'est jamais exécuté directement sur le processeur physique : Valgrind le charge, traduit dynamiquement chaque bloc d'instructions machine en une version instrumentée, puis exécute cette version traduite.

Ce mécanisme de traduction binaire dynamique (*Dynamic Binary Translation*, DBT) est ce qui confère à Valgrind deux de ses propriétés essentielles :

- **Aucune recompilation nécessaire.** Valgrind travaille sur le binaire final. Vous pouvez analyser un exécutable compilé par quelqu'un d'autre, une librairie tierce pour laquelle vous n'avez pas le code source, ou même un programme dont vous avez perdu le Makefile. La seule recommandation est de compiler avec les symboles de débogage (`-g`) pour obtenir des rapports lisibles — mais ce n'est pas une obligation.

- **Un surcoût significatif à l'exécution.** La traduction et l'instrumentation de chaque instruction ont un coût. Sous Memcheck, un programme s'exécute typiquement 10 à 20 fois plus lentement qu'en temps normal. Ce ralentissement est le prix de l'exhaustivité : Memcheck traque l'état de chaque octet de mémoire et de chaque bit de registre.

Le schéma d'exécution est le suivant :

```
Exécution normale :
    Programme  ──→  CPU physique  ──→  Résultat

Exécution sous Valgrind :
    Programme  ──→  Valgrind (traduction + instrumentation)  ──→  CPU virtuel  ──→  Résultat
                         │
                         └──→  Rapport d'erreurs
```

Pour Memcheck spécifiquement, l'instrumentation consiste à maintenir un *shadow memory* : une copie parallèle de toute la mémoire du programme où chaque octet est annoté comme « initialisé » ou « non initialisé », « alloué » ou « libéré ». Chaque opération mémoire est vérifiée contre cet état fantôme, et toute incohérence est signalée.

---

## Installation sur Ubuntu

Valgrind est disponible dans les dépôts officiels d'Ubuntu :

```bash
sudo apt update  
sudo apt install valgrind  
```

On vérifie l'installation :

```bash
valgrind --version
```

Sur Ubuntu 24.04 LTS et ultérieur, les dépôts fournissent Valgrind 3.22+, qui prend en charge les jeux d'instructions récents (AVX-512, certaines extensions ARMv8). Pour les projets utilisant des instructions très récentes ou des fonctionnalités expérimentales du noyau, il est parfois nécessaire de compiler Valgrind depuis les sources — mais pour la grande majorité des cas d'usage, la version des dépôts est suffisante.

> 💡 **Note** : Valgrind ne supporte que les architectures x86, x86_64, ARM32, ARM64, MIPS, PPC et S390x sous Linux (et partiellement macOS et Solaris). Il n'est pas disponible sous Windows.

---

## Première exécution

L'utilisation de base de Valgrind est d'une simplicité remarquable. Considérons un programme contenant une fuite mémoire volontaire :

```cpp
// fuite.cpp
#include <iostream>

int main() {
    int* tableau = new int[100];
    tableau[0] = 42;
    std::cout << tableau[0] << "\n";
    // Oubli volontaire : pas de delete[] tableau;
    return 0;
}
```

On compile avec les informations de débogage, sans optimisations :

```bash
g++ -std=c++23 -g -O0 -o fuite fuite.cpp
```

Puis on exécute sous Valgrind :

```bash
valgrind ./fuite
```

Par défaut, Valgrind lance l'outil Memcheck (équivalent à `valgrind --tool=memcheck ./fuite`). La sortie ressemble à ceci :

```
==12345== Memcheck, a memory error detector
==12345== Copyright (C) 2002-2024, and GNU GPL'd, by Julian Seward et al.
==12345== Using Valgrind-3.23.0 and LibVEX; rerun with -h for copyright info
==12345== Command: ./fuite
==12345==
42
==12345==
==12345== HEAP SUMMARY:
==12345==     in use at exit: 400 bytes in 1 blocks
==12345==   total heap usage: 3 allocs, 2 frees, 73,104 bytes allocated
==12345==
==12345== LEAK SUMMARY:
==12345==    definitely lost: 400 bytes in 1 blocks
==12345==    indirectly lost: 0 bytes in 0 blocks
==12345==      possibly lost: 0 bytes in 0 blocks
==12345==    still reachable: 0 bytes in 0 blocks
==12345==         suppressed: 0 bytes in 0 blocks
==12345== Rerun with --leak-check=full to see details of leaked memory
==12345==
==12345== For lists of detected and suppressed errors, rerun with: -s
==12345== ERROR SUMMARY: 0 errors from 0 contexts (suppressed: 0 from 0)
```

Plusieurs éléments méritent attention dans cette sortie :

- **`==12345==`** : Chaque ligne de Valgrind est préfixée par le PID du processus analysé. Cela permet de distinguer la sortie de Valgrind de la sortie standard du programme (ici, la ligne `42`).

- **HEAP SUMMARY** : À la fin de l'exécution, 400 octets restent alloués (100 entiers × 4 octets). Le programme a réalisé 3 allocations au total (le `new int[100]`, plus des allocations internes de la librairie standard pour `std::cout`) et seulement 2 libérations.

- **LEAK SUMMARY** : Valgrind catégorise les fuites. Ici, 400 octets sont marqués *definitely lost* — c'est-à-dire qu'aucun pointeur du programme ne référence plus cette zone au moment de la sortie. C'est la catégorie la plus grave : cette mémoire est irrémédiablement perdue.

- **Le conseil** : Valgrind suggère de relancer avec `--leak-check=full` pour obtenir les détails, notamment la pile d'appels (*stack trace*) de chaque allocation fuitée.

---

## Options essentielles

Valgrind propose de nombreuses options. En voici les plus utiles pour l'analyse mémoire courante :

### Détails des fuites

```bash
valgrind --leak-check=full ./fuite
```

Active le rapport détaillé des fuites avec les piles d'appels. C'est l'option que vous utiliserez dans la grande majorité des cas. Sans elle, Valgrind signale l'existence de fuites mais pas leur origine.

### Origine des valeurs non initialisées

```bash
valgrind --track-origins=yes ./mon_programme
```

Lorsque Memcheck détecte l'utilisation d'une valeur non initialisée, cette option lui permet de remonter jusqu'à l'allocation d'origine. Le surcoût mémoire et CPU augmente, mais les rapports deviennent nettement plus exploitables.

### Afficher les blocs accessibles

```bash
valgrind --leak-check=full --show-reachable=yes ./mon_programme
```

Par défaut, Valgrind ne détaille que les blocs *lost*. Avec `--show-reachable=yes`, les blocs encore référencés à la sortie (mais non libérés) sont également affichés. Utile pour un audit mémoire exhaustif, mais génère beaucoup de bruit sur les programmes utilisant intensivement la librairie standard.

### Redirection vers un fichier

```bash
valgrind --log-file=valgrind_rapport.txt ./mon_programme
```

Redirige la sortie de Valgrind vers un fichier, ce qui facilite l'analyse post-exécution, le partage en code review ou l'archivage en CI.

### Combinaison courante en pratique

La commande que vous utiliserez le plus souvent combine les options les plus utiles :

```bash
valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ./mon_programme
```

Cette commande active le rapport détaillé de toutes les catégories de fuites et le traçage de l'origine des valeurs non initialisées.

---

## Catégories de fuites mémoire

Valgrind classe les blocs de mémoire non libérés en quatre catégories dans son *LEAK SUMMARY*. Comprendre cette classification est essentiel pour prioriser les corrections.

### Definitely lost

Aucun pointeur dans le programme ne pointe vers le bloc alloué, ni vers l'intérieur de celui-ci. La mémoire est irrémédiablement perdue — c'est une fuite certaine. C'est la catégorie la plus critique et celle à corriger en priorité.

```cpp
void creer_fuite() {
    int* p = new int(42);
    // p sort de la portée : plus aucune référence vers le bloc
}
```

### Indirectly lost

Le bloc n'est pas directement référencé, mais il est accessible *via* un autre bloc lui-même classé *definitely lost*. Typiquement, il s'agit de la mémoire pointée par les membres d'un objet dont le pointeur racine a été perdu. Corriger la fuite racine (*definitely lost*) corrige automatiquement les fuites indirectes.

```cpp
struct Node {
    int* data;
    Node(int v) : data(new int(v)) {}
};

void creer_fuite_indirecte() {
    Node* n = new Node(42);
    // n est definitely lost
    // n->data est indirectly lost
}
```

### Possibly lost

Valgrind a trouvé un pointeur qui pointe vers l'intérieur du bloc, mais pas vers son début. Cela peut indiquer une fuite réelle, ou simplement un pattern légitime (arithmétique de pointeurs, alignement interne, certaines implémentations de conteneurs). Ces cas nécessitent une inspection manuelle.

### Still reachable

Un pointeur vers le bloc existe encore au moment de la sortie du programme. Il ne s'agit pas d'une fuite au sens strict — le programme *pourrait* libérer la mémoire — mais il ne le fait pas. C'est courant avec les singletons, les caches globaux ou les allocations de la librairie standard. Dans la plupart des cas, ces blocs sont bénins : le système d'exploitation récupère la mémoire à la fin du processus.

---

## Types d'erreurs détectées par Memcheck

Au-delà des fuites mémoire, Memcheck détecte plusieurs catégories d'erreurs qui provoquent un comportement indéfini :

### Lectures et écritures invalides

Tout accès en lecture ou en écriture à une zone mémoire non allouée ou déjà libérée :

```cpp
int* p = new int(10);  
delete p;  
*p = 20;  // Invalid write of size 4
```

Memcheck signale la taille de l'accès invalide, l'adresse cible, et la pile d'appels. Si le bloc a été libéré, il indique également où l'allocation et la libération ont eu lieu.

### Utilisation de valeurs non initialisées

L'utilisation d'une variable non initialisée dans une condition ou un calcul :

```cpp
int x;  
if (x > 0) {  // Conditional jump depends on uninitialised value  
    // ...
}
```

Avec `--track-origins=yes`, Memcheck remonte à la déclaration ou à l'allocation qui a créé la valeur non initialisée.

### Appels système avec des paramètres invalides

Memcheck vérifie les buffers passés aux appels système (`write`, `read`, etc.) et signale lorsqu'ils contiennent des données non initialisées ou pointent vers des zones invalides.

### Désallocations incorrectes

L'utilisation de `delete` sur un pointeur alloué avec `new[]` (ou inversement), ou l'appel de `free` sur un pointeur alloué avec `new` :

```cpp
int* tab = new int[10];  
delete tab;  // Mismatched free() / delete / delete[]  
```

### Doubles libérations

La libération d'un bloc déjà libéré :

```cpp
int* p = new int(5);  
delete p;  
delete p;  // Invalid free() / delete / delete[]  
```

---

## Limites et considérations pratiques

Valgrind est un outil puissant, mais il faut connaître ses limites pour l'utiliser efficacement.

**Performance.** Le ralentissement de 10 à 20× sous Memcheck rend l'outil impraticable pour les tests de performance ou les programmes avec des contraintes de temps réel. Pour les suites de tests volumineuses, il est courant de n'exécuter Valgrind que sur un sous-ensemble ciblé ou en mode nightly dans la CI.

**Mémoire.** Valgrind consomme environ 2× la mémoire du programme analysé (du fait du shadow memory). Pour les programmes déjà gourmands en mémoire, cela peut poser des problèmes sur des machines aux ressources limitées.

**Faux positifs.** Certaines optimisations de la librairie standard ou des librairies tierces génèrent des rapports que Valgrind ne sait pas interpréter correctement. Valgrind fournit un mécanisme de fichiers de *suppressions* (`.supp`) pour filtrer ces faux positifs connus. Des fichiers de suppression pour la glibc et d'autres librairies courantes sont inclus par défaut.

**Multithreading.** Memcheck détecte les erreurs mémoire dans un contexte multithread, mais il ne détecte pas les *data races* à proprement parler. Pour cela, il faut utiliser l'outil **Helgrind** ou **DRD** de Valgrind — ou préférer ThreadSanitizer (section 29.4.3), qui est plus rapide et plus précis pour cette catégorie de bugs.

**Pas de support simultané avec les sanitizers.** Un binaire compilé avec `-fsanitize=address` ne peut pas être exécuté sous Valgrind. Les deux outils instrumentent la mémoire de manière incompatible. Il faut choisir l'un ou l'autre pour une exécution donnée.

---

## Plan des sous-sections

Les sous-sections qui suivent détaillent l'utilisation pratique de Valgrind :

- **30.1.1 — memcheck : Détection de fuites** : Utilisation approfondie de Memcheck pour détecter et catégoriser les fuites mémoire, avec des scénarios réalistes et les options de ligne de commande adaptées à chaque situation.

- **30.1.2 — Lecture des rapports Valgrind** : Méthodologie de lecture et d'interprétation des rapports d'erreurs, décodage des piles d'appels, corrélation avec le code source, et stratégies pour traiter les rapports volumineux.

⏭️ [memcheck : Détection de fuites](/30-analyse-memoire/01.1-memcheck.md)
