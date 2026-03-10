🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 1.4 — Anatomie d'un exécutable ELF sur Linux

> **Chapitre 1 — Introduction au C++ et à l'écosystème Linux**  
> **Niveau** : Débutant  
> **Durée estimée** : 40 à 55 minutes (section principale + sous-sections)

---

## Introduction

La section précédente a décrit le voyage d'un fichier source `.cpp` à travers les quatre étapes du pipeline de compilation. Le résultat final de ce voyage est un **fichier binaire** — un exécutable ou une bibliothèque partagée — que le noyau Linux sait charger en mémoire et exécuter. Mais que contient exactement ce fichier ? Comment est-il organisé ? Comment le noyau sait-il où commence le code, où se trouvent les données, et à quelle adresse commencer l'exécution ?

La réponse à toutes ces questions est le format **ELF** — *Executable and Linkable Format*. ELF est le format binaire standard de Linux (et de la plupart des systèmes Unix modernes : FreeBSD, Solaris, Android…). Il est utilisé pour trois types de fichiers que vous avez déjà rencontrés dans les sections précédentes :

- les **fichiers objet** (`.o`) — produits par le compilateur, consommés par le linker ;
- les **exécutables** — produits par le linker, chargés et exécutés par le noyau ;
- les **bibliothèques partagées** (`.so`) — chargées dynamiquement à l'exécution.

Comprendre le format ELF n'est pas un exercice académique réservé aux développeurs noyau. C'est une compétence directement utile au quotidien : diagnostiquer un exécutable qui ne se lance pas, vérifier qu'un binaire est compilé pour la bonne architecture, comprendre pourquoi une bibliothèque partagée est introuvable, analyser un crash avec un core dump, ou optimiser la taille d'une image Docker en examinant ce que contient le binaire. Chaque fois que vous utilisez `file`, `nm`, `ldd`, `readelf` ou `objdump`, vous interrogez la structure ELF du fichier.

Cette section présente la vue d'ensemble du format. Les deux sous-sections qui suivent entrent dans le détail : la section 1.4.1 décrit les composants structurels (headers, sections, segments), et la section 1.4.2 montre comment les inspecter avec les outils en ligne de commande.

---

## Un peu d'histoire

Avant ELF, les systèmes Unix utilisaient le format **a.out** (*assembler output*), un format simple mais limité. `a.out` ne supportait pas correctement les bibliothèques partagées, rendait la relocation complexe, et sa structure rigide ne permettait pas d'ajouter facilement de nouvelles sections de données (informations de débogage, métadonnées, etc.).

Le format ELF a été développé par **Unix System Laboratories** (USL) au début des années 1990, dans le cadre de la spécification System V ABI. Il a été conçu pour remplacer `a.out` en offrant une structure flexible, extensible et portable entre architectures. Linux a adopté ELF en 1995 (noyau 1.2), et il est depuis le seul format binaire utilisé sur les systèmes GNU/Linux.

> 💡 **Note** — Le nom par défaut des exécutables produits par GCC est `a.out`, un vestige historique du format du même nom. Quand vous tapez `g++ main.cpp` sans l'option `-o`, le résultat est un fichier `a.out` — mais son format est bien ELF, pas l'ancien format a.out.

---

## Vue d'ensemble de la structure ELF

Un fichier ELF est organisé autour de trois composants fondamentaux :

```
┌─────────────────────────────────┐
│          ELF Header             │  ← Point d'entrée : identifie le fichier
├─────────────────────────────────┤
│    Program Header Table         │  ← Vue "exécution" : comment charger en mémoire
│    (table des en-têtes de       │     (segments)
│     programme)                  │
├─────────────────────────────────┤
│                                 │
│          Section 1              │
│          (.text)                │  ← Code machine exécutable
│                                 │
├─────────────────────────────────┤
│          Section 2              │
│          (.rodata)              │  ← Données en lecture seule
├─────────────────────────────────┤
│          Section 3              │
│          (.data)                │  ← Variables globales initialisées
├─────────────────────────────────┤
│          Section 4              │
│          (.bss)                 │  ← Variables non-initialisées (taille seule)
├─────────────────────────────────┤
│          ...                    │
│     (autres sections)           │  ← Symboles, relocations, debug, etc.
│          ...                    │
├─────────────────────────────────┤
│    Section Header Table         │  ← Vue "liaison" : inventaire des sections
│    (table des en-têtes de       │
│     sections)                   │
└─────────────────────────────────┘
```

Le principe fondamental d'ELF est d'offrir **deux vues complémentaires** du même fichier :

**La vue "sections"** est destinée à l'éditeur de liens. Elle découpe le fichier en **sections** nommées (`.text`, `.data`, `.rodata`, `.symtab`…), chacune ayant un type et des attributs spécifiques. C'est cette vue qui est pertinente pour les fichiers objet (`.o`), que le linker doit lire, analyser et fusionner.

**La vue "segments"** est destinée au noyau et au chargeur dynamique. Elle regroupe les sections en **segments** (aussi appelés *program headers*), chacun décrivant une zone contiguë de mémoire à charger avec des permissions spécifiques (lecture, écriture, exécution). C'est cette vue qui est pertinente pour les exécutables et les bibliothèques partagées, que le noyau doit charger en mémoire.

Un fichier objet (`.o`) possède uniquement la vue sections (pas de segments — il n'est pas destiné à être chargé directement). Un exécutable possède les deux vues. Une bibliothèque partagée (`.so`) possède également les deux.

---

## L'en-tête ELF (*ELF Header*)

Tout fichier ELF commence par un **en-tête** de taille fixe (64 octets sur les systèmes 64 bits) qui contient les métadonnées essentielles du fichier. Cet en-tête est le point d'entrée pour tout outil qui souhaite lire le fichier.

Les champs les plus importants de l'en-tête sont :

**Le nombre magique** — les quatre premiers octets de tout fichier ELF sont toujours `7f 45 4c 46`, soit `\x7fELF` en ASCII. C'est ce que la commande `file` vérifie en premier pour identifier un fichier ELF. Vous pouvez le voir vous-même avec :

```bash
xxd -l 4 ./programme
# 00000000: 7f45 4c46                    .ELF
```

**La classe** — 32 bits (ELFCLASS32) ou 64 bits (ELFCLASS64). Sur un Ubuntu 64 bits moderne, tous les binaires sont en ELFCLASS64.

**L'encodage des données** — little-endian (LSB, le cas sur x86_64) ou big-endian (MSB, utilisé par certaines architectures comme SPARC et PowerPC en mode big-endian).

**Le type de fichier** — il distingue les trois catégories de fichiers ELF :

| Type | Valeur | Description |
|------|--------|-------------|
| `ET_REL` | Relocatable | Fichier objet (`.o`) |
| `ET_EXEC` | Executable | Exécutable à adresse fixe |
| `ET_DYN` | Shared object | Bibliothèque partagée (`.so`) ou exécutable PIE |

> 💡 **Note** — Sur les systèmes modernes, les exécutables sont compilés par défaut en mode **PIE** (*Position-Independent Executable*), ce qui signifie qu'ils sont techniquement des fichiers de type `ET_DYN` (shared object), pas `ET_EXEC`. C'est pourquoi la commande `file` affiche souvent `pie executable` ou `shared object` même pour un programme exécutable. Le mode PIE est nécessaire pour la randomisation de l'espace d'adressage (ASLR), une protection de sécurité majeure (voir section 45.4.3).

**L'architecture cible** — `EM_X86_64` pour x86_64, `EM_AARCH64` pour ARM 64 bits, `EM_RISCV` pour RISC-V, etc. Si vous tentez d'exécuter un binaire compilé pour la mauvaise architecture, le noyau refuse de le charger :

```
bash: ./programme: cannot execute binary file: Exec format error
```

**Le point d'entrée** (*entry point*) — l'adresse mémoire de la première instruction à exécuter. Contrairement à ce qu'on pourrait penser, ce n'est pas l'adresse de `main()`. Le point d'entrée est la fonction `_start`, un morceau de code fourni par la bibliothèque C (*crt0* — *C runtime startup*) qui initialise l'environnement du programme (pile, variables globales, constructeurs de classes C++ statiques…) puis appelle `main()`.

**Les offsets des tables** — l'en-tête indique où se trouvent dans le fichier la *Program Header Table* (table des segments) et la *Section Header Table* (table des sections), ainsi que le nombre d'entrées dans chacune.

---

## Les sections : la vue du linker

Les sections sont les blocs fondamentaux de la vue liaison. Chaque section a un nom, un type, des flags (permissions, chargement en mémoire ou non), et un contenu. Les sections les plus courantes dans un binaire C++ sont :

### Sections de code et de données

| Section | Contenu | Permissions mémoire |
|---------|---------|-------------------|
| `.text` | Code machine (instructions processeur) | Lecture + Exécution |
| `.rodata` | Données en lecture seule (littéraux chaînes, constantes, tables `switch`) | Lecture |
| `.data` | Variables globales et statiques initialisées avec des valeurs non nulles | Lecture + Écriture |
| `.bss` | Variables globales et statiques non-initialisées ou initialisées à zéro | Lecture + Écriture |

La section `.bss` (*Block Started by Symbol*, un nom hérité d'un assembleur des années 1950) a une particularité importante : elle n'occupe **aucune place sur le disque**. Le fichier ELF enregistre seulement sa taille, et le chargeur alloue la mémoire correspondante (remplie de zéros) au moment du chargement. C'est pourquoi un programme avec de grands tableaux globaux initialisés à zéro ne produit pas un binaire proportionnellement plus gros.

### Sections de métadonnées

| Section | Contenu |
|---------|---------|
| `.symtab` | Table des symboles complète (fonctions, variables globales, locales…) |
| `.strtab` | Table de chaînes associée à `.symtab` (les noms des symboles) |
| `.dynsym` | Table des symboles dynamiques (sous-ensemble de `.symtab`, nécessaire au chargement dynamique) |
| `.dynstr` | Table de chaînes associée à `.dynsym` |
| `.rel.text` / `.rela.text` | Entrées de relocation pour la section `.text` |
| `.rel.dyn` / `.rela.dyn` | Relocations dynamiques (résolues au chargement) |
| `.dynamic` | Informations pour le chargeur dynamique (bibliothèques requises, RPATH…) |
| `.plt` | *Procedure Linkage Table* — trampolines pour les appels de fonctions dynamiques |
| `.got` | *Global Offset Table* — table d'indirection pour les symboles dynamiques |

### Sections de débogage

Quand le binaire est compilé avec `-g`, des sections supplémentaires au format **DWARF** sont ajoutées :

| Section | Contenu |
|---------|---------|
| `.debug_info` | Descriptions des types, variables et fonctions |
| `.debug_line` | Correspondance instructions machine ↔ lignes du code source |
| `.debug_abbrev` | Abréviations utilisées dans `.debug_info` |
| `.debug_str` | Chaînes de caractères des informations de débogage |

Ces sections peuvent être très volumineuses (parfois plusieurs fois la taille du code machine) mais ne sont jamais chargées en mémoire pendant l'exécution normale. Elles sont lues uniquement par les debuggers (GDB) et les profilers. L'outil `strip` permet de les supprimer pour réduire la taille du binaire en production :

```bash
strip --strip-debug programme        # Supprime les infos de débogage  
strip --strip-all programme          # Supprime aussi la table des symboles  
```

> 🔥 **Point clé** — La commande `strip` est un outil essentiel dans un workflow DevOps. Un binaire C++ compilé avec `-g -O2` peut peser 50 Mo avec les informations de débogage et seulement 2 Mo après un `strip --strip-all`. Pour le déploiement en conteneur Docker, le stripping est quasi systématique. On conserve une copie non-strippée pour le débogage post-mortem si nécessaire.

---

## Les segments : la vue du noyau

Les segments (décrits dans la *Program Header Table*) regroupent une ou plusieurs sections en zones mémoire contigues avec des permissions communes. C'est la vue que le noyau utilise pour charger le programme. Le chargeur lit chaque segment de type `LOAD` et le copie (ou le mappe via `mmap`) dans l'espace d'adressage du processus.

Les segments les plus courants sont :

| Type de segment | Sections typiques incluses | Permissions | Rôle |
|----------------|---------------------------|-------------|------|
| `LOAD` (code) | `.text`, `.rodata` | R + X | Code exécutable et données constantes |
| `LOAD` (données) | `.data`, `.bss` | R + W | Variables globales modifiables |
| `DYNAMIC` | `.dynamic` | R | Informations pour le chargeur dynamique |
| `INTERP` | `.interp` | R | Chemin du chargeur dynamique (`/lib64/ld-linux-x86-64.so.2`) |
| `NOTE` | `.note.*` | R | Métadonnées (version ABI, build ID) |
| `GNU_STACK` | *(aucune)* | variable | Permissions de la pile (non-exécutable par défaut, sécurité) |

La distinction entre sections et segments est fondamentale :

- Les **sections** sont granulaires et nommées. Elles servent au linker et aux outils d'analyse. Un fichier objet (`.o`) n'a que des sections.
- Les **segments** sont des regroupements orientés mémoire. Ils servent au chargeur du noyau. Un exécutable et un `.so` ont des segments.

Un segment `LOAD` contient typiquement plusieurs sections. Par exemple, le segment de code (permissions R+X) regroupe `.text` et `.rodata`. Le segment de données (permissions R+W) regroupe `.data` et `.bss`. Ce regroupement est nécessaire parce que la gestion mémoire du noyau opère par **pages** (4 Ko sur x86_64), et chaque page a un jeu de permissions unique. Il est plus efficace de regrouper les sections partageant les mêmes permissions dans des pages contiguës.

---

## Le chargement d'un exécutable ELF par le noyau

Quand vous tapez `./programme` dans un terminal, voici ce qui se passe, étape par étape :

**1. Le shell appelle `execve()`.** Le shell invoque l'appel système `execve()` en passant le chemin du programme. Le noyau Linux prend la main.

**2. Le noyau lit l'en-tête ELF.** Il vérifie le nombre magique (`\x7fELF`), la classe (64 bits), l'architecture (x86_64), et le type (exécutable ou shared object PIE).

**3. Le noyau charge les segments `LOAD`.** Pour chaque segment de type `LOAD`, le noyau mappe les pages correspondantes du fichier en mémoire avec les bonnes permissions (lecture, écriture, exécution). La section `.bss`, qui n'a pas de contenu sur disque, est allouée comme pages anonymes remplies de zéros.

**4. Le noyau examine le segment `INTERP`.** Si le binaire est lié dynamiquement (ce qui est le cas par défaut), le segment `INTERP` contient le chemin de l'**éditeur de liens dynamique** — typiquement `/lib64/ld-linux-x86-64.so.2`. Le noyau charge ce programme en mémoire et lui passe le contrôle.

**5. L'éditeur de liens dynamique résout les dépendances.** `ld-linux` lit la section `.dynamic` de l'exécutable pour trouver la liste des bibliothèques partagées requises (`NEEDED`), les charge en mémoire, résout les symboles dynamiques via la `.got` et la `.plt`, et applique les relocations dynamiques.

**6. Le contrôle est transféré au point d'entrée.** L'éditeur de liens dynamique saute à l'adresse indiquée par le champ *entry point* de l'en-tête ELF. Cette adresse pointe vers `_start` (le code de démarrage de la libc), qui initialise l'environnement, appelle les constructeurs globaux C++, puis appelle `main()`.

**7. `main()` s'exécute.** Votre programme tourne enfin.

```
Shell
  │
  ▼ execve("./programme")
Noyau Linux
  ├── Lecture ELF header
  ├── Chargement segments LOAD (mmap)
  ├── Chargement ld-linux (INTERP)
  │
  ▼
ld-linux (éditeur de liens dynamique)
  ├── Chargement des .so requises (NEEDED)
  ├── Résolution des symboles (GOT/PLT)
  ├── Relocations dynamiques
  │
  ▼
_start (crt0 — C runtime startup)
  ├── Initialisation de l'environnement
  ├── Appel des constructeurs globaux C++
  │
  ▼
main()
  └── Votre programme
```

> 💡 **Note** — Pour un binaire lié **statiquement** (`g++ -static ...`), les étapes 4 et 5 n'existent pas. Il n'y a pas d'éditeur de liens dynamique, pas de bibliothèques partagées à charger. Le noyau transfère le contrôle directement à `_start`. C'est ce qui rend les binaires statiques plus rapides au démarrage et plus simples à déployer.

---

## Première inspection rapide d'un binaire

Avant de plonger dans les outils détaillés (section 1.4.2), voici les commandes les plus immédiates pour inspecter un binaire ELF :

```bash
# Identification rapide : architecture, type, linkage
file ./programme
# programme: ELF 64-bit LSB pie executable, x86-64, version 1 (SYSV),
# dynamically linked, interpreter /lib64/ld-linux-x86-64.so.2, ... not stripped

# Taille des sections principales
size ./programme
#    text    data     bss     dec     hex filename
#    1832     640       8    2480     9b0 programme

# Dépendances dynamiques
ldd ./programme
# libc.so.6 => /lib/x86_64-linux-gnu/libc.so.6
# libstdc++.so.6 => /lib/x86_64-linux-gnu/libstdc++.so.6
# ...
```

La commande `size` est particulièrement utile pour une vue synthétique. Elle affiche la taille (en octets) des trois sections fondamentales : `text` (code + données constantes), `data` (variables initialisées), et `bss` (variables non-initialisées). La colonne `dec` est le total en décimal — c'est l'empreinte mémoire minimale de votre programme, hors bibliothèques partagées.

---

## Pourquoi tout cela compte pour un développeur C++

Connaître la structure ELF vous permet de répondre à des questions concrètes que vous rencontrerez régulièrement :

**"Pourquoi mon binaire fait 200 Mo ?"** — Probablement les informations de débogage. Vérifiez avec `readelf -S` la taille des sections `.debug_*`, et utilisez `strip` pour les supprimer.

**"Mon programme plante avec `cannot execute binary file`."** — Vérifiez l'architecture avec `file`. Vous essayez probablement d'exécuter un binaire ARM sur une machine x86_64 (ou inversement).

**"Mon programme plante avec `cannot open shared object file`."** — Une bibliothèque dynamique est introuvable. Utilisez `ldd` pour identifier laquelle, puis vérifiez son installation et le chemin de recherche.

**"Je veux réduire la taille de mon image Docker."** — Comparez le binaire avant et après `strip`. Envisagez le linkage statique avec musl pour éliminer les dépendances dynamiques. Examinez avec `size` si la section `.data` est anormalement grande.

**"Je débogue un crash avec un core dump."** — Le core dump est lui-même un fichier ELF. GDB l'utilise conjointement avec le binaire non-strippé (contenant les sections `.debug_*`) pour reconstituer l'état du programme au moment du crash.

---

## Plan des sous-sections

Les deux sous-sections suivantes approfondissent la structure et les outils d'inspection :

- **[1.4.1 — Structure du format ELF (headers, sections, segments)](./04.1-structure-elf.md)** : description détaillée de chaque composant structurel, avec les champs de données, les types, les flags et les relations entre sections et segments.

- **[1.4.2 — Inspection avec readelf et objdump](./04.2-inspection-elf.md)** : utilisation pratique des outils `readelf`, `objdump`, `nm`, `size`, `file`, `strings` et `hexdump` pour analyser un binaire ELF sous tous les angles.

---

## Ce qu'il faut retenir

ELF est le format binaire universel de Linux. Il structure les fichiers objet, les exécutables et les bibliothèques partagées autour de deux vues complémentaires : les sections (granulaires, pour le linker et les outils d'analyse) et les segments (orientés mémoire, pour le noyau). L'en-tête ELF identifie le fichier et pointe vers les tables de sections et de segments. Le noyau charge les segments en mémoire, délègue la résolution des symboles dynamiques à `ld-linux`, et le code de démarrage de la libc initialise l'environnement avant d'appeler `main()`. Savoir lire la structure ELF d'un binaire est une compétence transversale qui sert autant au développement qu'au débogage, au déploiement et à l'optimisation.

---


⏭️ [Structure du format ELF (headers, sections, segments)](/01-introduction-cpp-linux/04.1-structure-elf.md)
