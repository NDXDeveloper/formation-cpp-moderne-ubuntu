🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 1. Introduction au C++ et à l'écosystème Linux

> **Module 1 — L'Environnement de Développement sur Linux**  
> **Niveau** : Débutant  
> **Durée estimée** : 3 à 4 heures

---

## Objectifs du chapitre

Ce premier chapitre pose les fondations nécessaires avant d'écrire la moindre ligne de code. À l'issue de sa lecture, vous serez capable de :

- situer le C++ dans l'histoire des langages de programmation et comprendre pourquoi il reste incontournable en 2026 ;
- expliquer en quoi le C++ se distingue des langages managés (Python, Java, Go) et pourquoi il est un choix naturel pour le DevOps, le system programming et les infrastructures cloud ;
- décrire les quatre grandes étapes du cycle de compilation — préprocesseur, compilateur, assembleur, éditeur de liens — et le rôle précis de chacune ;
- lire et interpréter la structure d'un exécutable ELF, le format binaire standard de Linux.

---

## Pourquoi commencer par là ?

Beaucoup de formations débutent directement par la syntaxe du langage. Nous faisons un choix différent : **comprendre l'environnement avant de coder**.

C++ n'est pas un langage interprété. Chaque programme que vous écrivez traverse une chaîne de transformation complexe avant de devenir un binaire exécutable par le noyau Linux. Ignorer cette chaîne, c'est se condamner à ne jamais comprendre les erreurs de compilation, les problèmes de linkage, les bibliothèques manquantes ou les segmentation faults qui jalonnent la vie quotidienne d'un développeur C++.

Le système d'exploitation joue un rôle central dans cette chaîne. Linux est historiquement la plateforme de prédilection pour le développement C++ système et cloud : le noyau lui-même est écrit en C (avec des composants C++ dans l'écosystème utilisateur), les outils de compilation sont natifs, et l'immense majorité des infrastructures de production tournent sur des distributions Linux. Comprendre comment Linux charge et exécute un binaire ELF, c'est acquérir un savoir qui vous servira à chaque étape de cette formation.

---

## Plan du chapitre

### 1.1 — Histoire et évolution du C++ (C++98 → C++26)

Le C++ a plus de quarante ans, mais il n'a jamais autant évolué qu'au cours de la dernière décennie. Cette section retrace les grandes étapes du langage, depuis sa création par Bjarne Stroustrup au début des années 1980 jusqu'à la ratification du standard C++26. Vous découvrirez comment le comité ISO fonctionne, pourquoi le cycle de publication a été fixé à trois ans, et ce qui fait du C++ moderne un langage radicalement différent du C++ que l'on enseignait dans les années 2000.

### 1.2 — Pourquoi C++ pour le DevOps et le System Programming

Python domine le scripting, Go séduit pour les microservices, Rust monte en puissance pour la sécurité mémoire. Alors pourquoi apprendre le C++ en 2026 ? Cette section examine les domaines où le C++ reste irremplaçable : les contraintes de performance extrême (latence sub-microseconde, throughput réseau), le contrôle fin de la mémoire et du matériel, l'écosystème massif de bibliothèques existantes, et son rôle dans les infrastructures critiques (bases de données, moteurs de jeu, systèmes embarqués, finance quantitative). Nous aborderons aussi la complémentarité croissante entre C++ et Rust, un thème qui reviendra tout au long de la formation.

### 1.3 — Le cycle de compilation : Préprocesseur → Compilateur → Assembleur → Linker

C'est le cœur technique de ce chapitre. Vous suivrez le parcours d'un fichier source `.cpp` à travers chacune des quatre étapes de la chaîne de compilation :

- **Le préprocesseur** (section 1.3.1) : résolution des `#include`, expansion des macros `#define`, compilation conditionnelle. Vous verrez pourquoi le préprocesseur est à la fois indispensable et souvent source de bugs subtils.
- **La compilation** (section 1.3.2) : transformation du code source prétraité en code objet (fichiers `.o`). Vous comprendrez ce que fait réellement le compilateur, de l'analyse syntaxique à la génération de code machine, en passant par les optimisations.
- **L'édition de liens** (section 1.3.3) : résolution des symboles entre les différents fichiers objet et les bibliothèques. C'est l'étape qui produit l'exécutable final, et c'est aussi celle qui génère les erreurs les plus cryptiques pour les débutants (`undefined reference`, `multiple definition`).

Chaque sous-section illustre l'étape correspondante avec les flags de GCC qui permettent de l'isoler (`-E`, `-S`, `-c`), afin que vous puissiez observer concrètement ce qui se passe à chaque phase.

### 1.4 — Anatomie d'un exécutable ELF sur Linux

Une fois le binaire produit, que contient-il exactement ? Le format ELF (*Executable and Linkable Format*) est le standard utilisé par Linux pour les exécutables, les bibliothèques partagées et les fichiers objet. Cette section vous apprend à le lire :

- **Structure du format ELF** (section 1.4.1) : l'en-tête ELF, les sections (`.text`, `.data`, `.bss`, `.rodata`, `.symtab`…), les segments et la table des en-têtes de programme. Vous comprendrez comment le noyau Linux utilise ces informations pour charger le programme en mémoire.
- **Inspection avec readelf et objdump** (section 1.4.2) : mise en pratique avec les outils en ligne de commande qui permettent de disséquer un binaire ELF. Ces outils sont indispensables pour diagnostiquer des problèmes de linkage, vérifier les dépendances dynamiques ou comprendre l'impact des options de compilation sur le binaire final.

---

## Prérequis

Ce chapitre est conçu pour les débutants en C++. Toutefois, il suppose que vous disposez de :

- un poste sous **Ubuntu** (22.04 LTS ou plus récent, 24.04 LTS recommandé) ou une distribution dérivée de Debian ;
- un accès au **terminal** et une familiarité minimale avec la ligne de commande (naviguer dans l'arborescence, éditer un fichier, lancer une commande) ;
- une compréhension générale de ce qu'est un programme informatique (variables, fonctions, exécution séquentielle) — une expérience dans n'importe quel langage suffit.

L'installation des compilateurs et des outils nécessaires sera traitée en détail dans le **chapitre 2** (Mise en place de la Toolchain sur Ubuntu).

---

## Conventions utilisées dans ce chapitre

Tout au long de cette formation, nous adoptons les conventions suivantes :

Les commandes à taper dans un terminal sont présentées ainsi :

```bash
g++ -std=c++23 -Wall -Wextra -o mon_programme main.cpp
```

Les extraits de code C++ utilisent la coloration syntaxique :

```cpp
#include <print>

int main() {
    std::print("Bienvenue dans la formation C++ moderne !\n");
    return 0;
}
```

> 💡 **Note pédagogique** — Les encadrés de ce type signalent une remarque importante, une astuce ou un piège courant.

> 🔥 **Point clé** — Les encadrés de ce type mettent en avant un concept fondamental ou une bonne pratique professionnelle à retenir.

---

## Positionnement dans la formation

```
PARTIE I : FONDATIONS
└── Module 1 : L'Environnement de Développement sur Linux
    ├── ➡️  Chapitre 1 : Introduction au C++ et à l'écosystème Linux  ← vous êtes ici
    └── Chapitre 2 : Mise en place de la Toolchain sur Ubuntu
```

Ce chapitre fournit le **contexte théorique**. Le chapitre suivant passe à la **pratique** en installant et configurant l'ensemble de la chaîne d'outils sur votre machine Ubuntu.

---

## Sections du chapitre

| Section | Titre | Fichier |
|---------|-------|---------|
| 1.1 | Histoire et évolution du C++ (C++98 → C++26) | [01-histoire-evolution-cpp.md](./01-histoire-evolution-cpp.md) |
| 1.2 | Pourquoi C++ pour le DevOps et le System Programming | [02-pourquoi-cpp-devops.md](./02-pourquoi-cpp-devops.md) |
| 1.3 | Le cycle de compilation : Préprocesseur → Compilateur → Assembleur → Linker | [03-cycle-compilation.md](./03-cycle-compilation.md) |
| 1.3.1 | Le préprocesseur : #include, #define, macros | [03.1-preprocesseur.md](./03.1-preprocesseur.md) |
| 1.3.2 | La compilation : Génération du code objet | [03.2-compilation.md](./03.2-compilation.md) |
| 1.3.3 | L'édition de liens : Résolution des symboles | [03.3-edition-liens.md](./03.3-edition-liens.md) |
| 1.4 | Anatomie d'un exécutable ELF sur Linux | [04-anatomie-elf.md](./04-anatomie-elf.md) |
| 1.4.1 | Structure du format ELF (headers, sections, segments) | [04.1-structure-elf.md](./04.1-structure-elf.md) |
| 1.4.2 | Inspection avec readelf et objdump | [04.2-inspection-elf.md](./04.2-inspection-elf.md) |

⏭️ [Histoire et évolution du C++ (C++98 → C++26)](/01-introduction-cpp-linux/01-histoire-evolution-cpp.md)
