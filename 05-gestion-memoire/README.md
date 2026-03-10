🔝 Retour au [Sommaire](/SOMMAIRE.md)

# Chapitre 5 — Gestion de la Mémoire (Le cœur du sujet)

## 🎯 Objectifs du chapitre

Ce chapitre constitue le tournant fondamental de votre apprentissage du C++. Comprendre la gestion de la mémoire, c'est comprendre **pourquoi C++ existe** et ce qui le distingue radicalement de langages comme Python, Java ou Go. Là où ces langages délèguent la mémoire à un ramasse-miettes (*garbage collector*), C++ vous confie les clés de la machine — avec la puissance et la responsabilité que cela implique.

À l'issue de ce chapitre, vous serez capable de :

- Distinguer les deux grandes zones mémoire d'un programme — la **stack** et le **heap** — et comprendre leurs caractéristiques respectives en termes de vitesse, de taille et de durée de vie.
- Utiliser l'allocation dynamique avec `new` / `delete` et `new[]` / `delete[]`, tout en comprenant pourquoi le C++ moderne cherche à s'en éloigner.
- Manipuler les pointeurs et l'arithmétique associée pour interagir avec la mémoire à bas niveau.
- Identifier les erreurs mémoire classiques — fuites mémoire (*memory leaks*), pointeurs pendants (*dangling pointers*), double libération (*double free*) — et comprendre leurs conséquences.
- Utiliser les outils de détection comme **Valgrind** et **AddressSanitizer** pour traquer ces erreurs avant qu'elles n'atteignent la production.

---

## 🧠 Pourquoi ce chapitre est essentiel

La gestion de la mémoire est le **sujet central** du C++. Ce n'est pas une exagération. Pratiquement toutes les fonctionnalités avancées du langage — le RAII (chapitre 6), les smart pointers (chapitre 9), la sémantique de mouvement (chapitre 10) — ont été conçues pour résoudre les problèmes que vous allez découvrir ici.

Quand un programme C++ plante avec un `Segmentation fault`, quand une application serveur voit sa consommation mémoire grimper inexorablement, quand un bug intermittent ne se manifeste qu'en production sous forte charge — dans la grande majorité des cas, la cause est une erreur de gestion mémoire.

Ce chapitre pose volontairement les bases avec les mécanismes **manuels** (`new`, `delete`, pointeurs bruts). L'objectif n'est pas de vous encourager à les utiliser au quotidien — le C++ moderne offre des alternatives bien plus sûres — mais de vous faire comprendre **ce qui se passe sous le capot**. Un développeur qui maîtrise les smart pointers sans comprendre la mémoire manuelle est comme un conducteur qui ne sait pas ce qu'est un embrayage : il avance, mais il est démuni dès que quelque chose sort de l'ordinaire.

---

## 📐 Modèle mental : la mémoire d'un processus

Lorsque vous lancez un programme compilé sur Linux, le noyau lui attribue un espace d'adressage virtuel. Cet espace est découpé en plusieurs régions, chacune ayant un rôle précis :

```
Adresses hautes
┌─────────────────────────┐
│        Stack (↓)        │  ← Variables locales, paramètres, adresses de retour
│                         │    Croît vers les adresses basses
│─────────────────────────│
│          ...            │  ← Espace libre entre stack et heap
│─────────────────────────│
│        Heap (↑)         │  ← Mémoire allouée dynamiquement (new, malloc)
│                         │    Croît vers les adresses hautes
│─────────────────────────│
│    Données non init.    │  ← .bss (variables globales non initialisées)
│─────────────────────────│
│    Données initialisées │  ← .data (variables globales initialisées)
│─────────────────────────│
│    Code (Text)          │  ← .text (instructions machine)
└─────────────────────────┘
Adresses basses
```

Ce schéma est une simplification, mais il capture l'essentiel. Deux zones vont concentrer toute notre attention dans ce chapitre : la **stack**, gérée automatiquement par le compilateur, et le **heap**, géré manuellement par le développeur.

---

## 🗺️ Plan du chapitre

Le chapitre progresse du concept vers la pratique, puis vers la détection d'erreurs :

**5.1 — Comprendre la Stack vs le Heap** — Nous commençons par une exploration détaillée des deux zones mémoire principales. Vous apprendrez à visualiser le diagramme mémoire d'un processus, à comprendre les caractéristiques de chaque zone et à identifier quelle zone est utilisée selon la façon dont vous déclarez vos variables.

**5.2 — Allocation dynamique : `new`/`delete`, `new[]`/`delete[]`** — Nous plongeons dans les mécanismes d'allocation et de libération manuelle de mémoire sur le heap. Vous apprendrez la syntaxe, les règles de correspondance entre `new` et `delete`, et les cas où l'allocation dynamique est nécessaire.

**5.3 — Arithmétique des pointeurs et accès bas niveau** — Les pointeurs sont l'interface directe avec la mémoire. Nous couvrons l'arithmétique des pointeurs, la relation entre pointeurs et tableaux, et les manipulations bas niveau qui font la force (et le danger) du C++.

**5.4 — Dangers : memory leaks, dangling pointers, double free** — Ce qui peut mal tourner, et **comment** ça tourne mal. Nous étudions les trois catégories d'erreurs mémoire les plus courantes, avec des exemples concrets de code problématique et leurs conséquences.

**5.5 — Outils de détection : Valgrind, AddressSanitizer** — Parce que l'œil humain ne suffit pas, nous mettons en place les outils qui détectent automatiquement les erreurs mémoire. Valgrind pour l'analyse dynamique post-compilation, et AddressSanitizer pour l'instrumentation à la compilation.

---

## 🔗 Liens avec le reste de la formation

Ce chapitre est un **prérequis direct** pour plusieurs sections ultérieures :

- **Chapitre 6 — Classes et Encapsulation** : le principe RAII (section 6.3) est la réponse du C++ aux problèmes de gestion manuelle de la mémoire que nous allons découvrir ici.
- **Chapitre 9 — Smart Pointers** : `std::unique_ptr`, `std::shared_ptr` et `std::weak_ptr` sont des abstractions construites directement au-dessus de `new` et `delete`.
- **Chapitre 10 — Sémantique de mouvement** : la move semantics optimise les transferts de ressources allouées sur le heap.
- **Chapitre 29 — Débogage avancé** et **Chapitre 30 — Analyse mémoire** : les outils introduits en section 5.5 sont approfondis dans ces chapitres avancés.
- **Chapitre 45 — Sécurité en C++** : les erreurs mémoire (buffer overflows, use-after-free) constituent la première source de vulnérabilités de sécurité en C++.

---

## ⚠️ Convention importante

Tout au long de ce chapitre, nous utilisons volontairement `new`, `delete` et des pointeurs bruts (`*`). C'est un choix pédagogique : il faut comprendre le mécanisme avant d'adopter les abstractions.

**Dans du code de production moderne, vous ne devriez quasiment jamais écrire `new` ou `delete` directement.** Les smart pointers (chapitre 9) et les conteneurs de la STL (chapitres 13-14) gèrent la mémoire pour vous de façon sûre et performante. Gardez cela en tête pendant votre lecture — nous construisons ici les fondations sur lesquelles repose tout le reste.

---

> *"In C++ it's harder to shoot yourself in the foot, but when you do, you blow off your whole leg."*  
> — Bjarne Stroustrup, créateur du C++

⏭️ [Comprendre la Stack (Pile) vs le Heap (Tas)](/05-gestion-memoire/01-stack-vs-heap.md)
