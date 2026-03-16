🔝 Retour au [Sommaire](/SOMMAIRE.md)

# Chapitre 29 — Débogage Avancé

## Module 10 : Débogage, Profiling et Qualité Code · Niveau Avancé

---

## Introduction

Un programme qui compile sans erreur n'est pas un programme qui fonctionne. Entre le moment où le compilateur accepte votre code et celui où votre application se comporte exactement comme prévu, il y a un territoire immense — celui du débogage. Et dans un langage comme C++, où la gestion de la mémoire est manuelle, où le comportement indéfini est silencieux, et où la concurrence ajoute des couches de non-déterminisme, ce territoire peut rapidement devenir hostile.

Le débogage en C++ n'est pas une activité accessoire qu'on pratique quand "ça ne marche pas". C'est une compétence fondamentale, un ensemble de techniques systématiques qui permettent de comprendre ce que fait réellement un programme — par opposition à ce qu'on pense qu'il fait. La distance entre ces deux réalités est souvent la source de tous les bugs.

Ce chapitre vous donne les outils et les méthodes pour réduire cette distance à zéro.

---

## Pourquoi un chapitre dédié au débogage avancé ?

Vous avez déjà rencontré `std::cout << "ici" << std::endl;` comme technique de débogage. Ça fonctionne, parfois. Mais cette approche atteint très vite ses limites :

- **Elle est invasive** — vous modifiez le code pour l'observer, ce qui peut changer le comportement du bug (en particulier en multithreading, où un simple `std::cout` peut masquer une race condition en ralentissant un thread).
- **Elle est binaire** — vous voyez si l'exécution passe par un point donné, mais vous ne voyez pas l'état complet du programme à cet instant.
- **Elle est lente** — chaque hypothèse nécessite une recompilation, une relance, et une analyse manuelle de la sortie.
- **Elle est impuissante** face aux bugs les plus vicieux : corruptions mémoire silencieuses, comportements indéfinis dont les effets se manifestent loin de leur cause, race conditions intermittentes.

Le débogage avancé, c'est passer d'une approche artisanale ("je mets des prints et je regarde") à une approche instrumentée et systématique, où chaque catégorie de bug a son outil dédié.

---

## Ce que vous allez apprendre

### 29.1 — GDB : Commandes essentielles et breakpoints

GDB (GNU Debugger) est le débogueur de référence sous Linux. Il vous permet d'exécuter votre programme pas à pas, d'inspecter l'état de la mémoire, des variables et de la pile d'appels à n'importe quel instant, et de poser des breakpoints conditionnels qui stoppent l'exécution uniquement quand une condition précise est remplie.

Vous apprendrez à naviguer dans un programme en cours d'exécution (`run`, `step`, `next`, `continue`), à inspecter des valeurs (`print`, `display`, `watch`), et à utiliser les breakpoints conditionnels pour cibler précisément un scénario de bug sans parcourir manuellement des milliers d'itérations.

### 29.2 — Débogage via IDE (VS Code, CLion)

GDB en ligne de commande est puissant, mais un débogueur graphique accélère considérablement le travail quotidien. Cette section couvre la configuration du débogage intégré dans VS Code (via l'extension C/C++ et les fichiers `launch.json`) et dans CLion, qui intègre nativement GDB et LLDB. Vous verrez comment poser des breakpoints visuels, inspecter des structures complexes, et naviguer dans la pile d'appels en un clic.

### 29.3 — Core dumps et post-mortem debugging

Quand un programme crash en production, il ne vous attend pas avec un débogueur ouvert. Mais si le système est correctement configuré, il génère un core dump — une image de la mémoire du processus au moment du crash. Cette section vous apprend à activer la génération de core dumps, à les charger dans GDB, et à reconstituer l'état exact du programme au moment de sa mort. C'est la technique essentielle pour diagnostiquer des crashs qui ne se reproduisent pas facilement.

### 29.4 — Sanitizers

Les sanitizers sont des outils d'instrumentation à la compilation qui détectent des catégories entières de bugs à l'exécution, avec une précision chirurgicale :

- **AddressSanitizer (ASan)** détecte les accès mémoire hors limites, les use-after-free, les double-free et les fuites mémoire. C'est l'outil qui transforme des corruptions mémoire silencieuses en erreurs explicites avec une trace complète.
- **UndefinedBehaviorSanitizer (UBSan)** capture les comportements indéfinis : dépassements d'entiers signés, déréférencements de pointeurs nuls, décalages de bits invalides — tous ces bugs que le compilateur a le droit d'ignorer et qui produisent des résultats imprévisibles.
- **ThreadSanitizer (TSan)** détecte les data races — ces accès concurrents non synchronisés qui produisent des bugs intermittents quasi impossibles à reproduire manuellement.
- **MemorySanitizer (MSan)** identifie les lectures de mémoire non initialisée, une source insidieuse de comportements erratiques.

### 29.5 — std::stacktrace : Traces d'exécution intégrées au débogage

Nouveauté C++23, `std::stacktrace` permet d'obtenir la pile d'appels directement depuis le code, sans outil externe. Cette section montre comment l'intégrer à vos gestionnaires d'erreurs et vos logs pour obtenir un diagnostic immédiat quand une situation inattendue survient.

---

## Taxonomie des bugs C++ et outils associés

Tous les bugs ne se déboguent pas de la même façon. Voici une cartographie des catégories courantes et des outils les plus adaptés à chacune :

| Catégorie de bug | Symptôme typique | Outil principal | Section |
|---|---|---|---|
| Erreur de logique | Résultat incorrect, comportement inattendu | GDB (breakpoints, inspection) | 29.1 |
| Crash (segfault) | Signal SIGSEGV, arrêt brutal | Core dump + GDB, ASan | 29.3, 29.4.1 |
| Fuite mémoire | Consommation mémoire croissante | ASan (mode leak), Valgrind | 29.4.1, 30.1 |
| Buffer overflow | Corruption silencieuse de données | ASan | 29.4.1 |
| Use-after-free | Comportement erratique, crash aléatoire | ASan | 29.4.1 |
| Comportement indéfini | Résultat variable selon l'optimisation | UBSan | 29.4.2 |
| Race condition | Bug intermittent sous charge | TSan | 29.4.3 |
| Lecture non initialisée | Valeur imprévisible | MSan | 29.4.4 |

Cette table est un point de départ : face à un bug, identifiez d'abord sa catégorie probable, puis choisissez l'outil adapté. C'est plus efficace que de lancer GDB systématiquement en espérant tomber sur le problème.

---

## Prérequis

Ce chapitre suppose que vous maîtrisez :

- **La compilation avec options de debug** (`-g`, `-ggdb3`) — section 2.6.3
- **Les bases de la gestion mémoire** (stack vs heap, pointeurs, allocation dynamique) — chapitre 5
- **Les smart pointers** (`std::unique_ptr`, `std::shared_ptr`) — chapitre 9
- **Les bases du multithreading** (`std::thread`, `std::mutex`) — chapitre 21

Si vous n'avez pas encore abordé le multithreading, vous pouvez parcourir les sections 29.1 à 29.3 et revenir aux sanitizers (29.4) après le chapitre 21.

---

## Environnement de travail

Tout au long de ce chapitre, vous aurez besoin de :

```bash
# GDB
sudo apt install gdb

# Sanitizers (inclus avec GCC et Clang)
# Aucune installation supplémentaire — activation via flags de compilation

# Vérification
gdb --version  
g++ --version   # GCC 15+ recommandé  
clang++ --version  # Clang 20+ recommandé  
```

Les sanitizers sont intégrés à GCC et Clang. Ils s'activent par des flags de compilation (`-fsanitize=...`), sans outil externe à installer. C'est l'un de leurs avantages majeurs par rapport à Valgrind : le coût d'adoption est quasi nul.

---

## Conseil de progression

Le débogage est une compétence qui se développe par la pratique. Chaque section de ce chapitre contient des exercices avec des programmes intentionnellement buggés. **Résistez à la tentation de lire la solution avant d'avoir essayé.** Le processus d'investigation — formuler une hypothèse, la tester avec un outil, ajuster — est exactement ce que vous devez entraîner.

Un développeur C++ efficace ne se distingue pas par sa capacité à écrire du code sans bugs (personne n'y arrive), mais par sa capacité à localiser et corriger un bug rapidement avec les bons outils.

---

## Structure des fichiers

```
29-debogage/
├── README.md                        ← vous êtes ici
├── 01-gdb-commandes.md
│   ├── 01.1-navigation.md
│   ├── 01.2-inspection.md
│   └── 01.3-breakpoints-conditionnels.md
├── 02-debogage-ide.md
├── 03-core-dumps.md
├── 04-sanitizers.md
│   ├── 04.1-addresssanitizer.md
│   ├── 04.2-ubsan.md
│   ├── 04.3-threadsanitizer.md
│   └── 04.4-memorysanitizer.md
├── 05-stacktrace-debug.md
└── exercices/
    ├── ex01-gdb-segfault/
    ├── ex02-gdb-conditionnel/
    ├── ex03-core-dump-analysis/
    ├── ex04-asan-buffer-overflow/
    ├── ex05-ubsan-integer-overflow/
    ├── ex06-tsan-race-condition/
    └── ex07-stacktrace-integration/
```

---

> **À retenir** : `std::cout` n'est pas un débogueur. GDB, les sanitizers et les core dumps le sont. Ce chapitre vous apprend à utiliser les bons outils pour chaque catégorie de bug — et à ne plus jamais perdre des heures sur un problème qu'un sanitizer aurait détecté en quelques secondes.

⏭️ [GDB : Commandes essentielles et breakpoints](/29-debogage/01-gdb-commandes.md)
