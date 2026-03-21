🔝 Retour au [Sommaire](/SOMMAIRE.md)

# Chapitre 45 — Sécurité en C++ ⭐

## Module 16 : Patterns et Architecture — Niveau Expert

---

## Pourquoi la sécurité est un enjeu central en C++

Le C++ offre un contrôle direct sur la mémoire, les pointeurs et les ressources système. Cette puissance a un corollaire : le langage ne protège pas le développeur contre lui-même. Contrairement aux langages à runtime managé (Java, C#, Go) ou aux langages conçus autour de la sécurité mémoire (Rust), le C++ laisse au programmeur la responsabilité totale de la validité des accès mémoire, de la durée de vie des objets et de l'intégrité des données.

En pratique, cela signifie que la majorité des vulnérabilités critiques exploitées dans les logiciels systèmes — navigateurs, noyaux, serveurs — trouvent leur origine dans du code C ou C++. Les rapports publiés par Google (projet Chromium), Microsoft et le projet Android convergent sur un même constat : environ 70 % des vulnérabilités de sécurité sévères (CVE) dans les bases de code natives relèvent de problèmes de sécurité mémoire.

Ce chapitre aborde la sécurité en C++ sous un angle résolument pratique et moderne. L'objectif n'est pas de dresser un catalogue théorique de failles, mais de fournir les outils, les techniques de compilation, les méthodologies de test et la vision stratégique nécessaires pour écrire du C++ robuste dans un contexte professionnel soumis à des exigences de sécurité croissantes.

---

## Ce que couvre ce chapitre

Le chapitre est organisé en six sections qui suivent une progression logique, depuis les vulnérabilités classiques jusqu'aux réponses les plus récentes du comité de standardisation C++ :

**Section 45.1 — Buffer overflows et protection.** La vulnérabilité historique la plus exploitée en C et C++. On y étudie les mécanismes d'un débordement de tampon (stack et heap), les conséquences en termes d'exploitation, et les contre-mesures côté code (usage de conteneurs STL, `std::span`, vérifications de bornes).

**Section 45.2 — Integer overflows et underflows.** Les dépassements arithmétiques sont une source de bugs subtils qui peuvent devenir des vulnérabilités exploitables lorsqu'ils influencent des tailles d'allocation ou des indices de tableaux. On y voit les comportements définis et indéfinis selon les types (signés vs non signés), ainsi que les techniques de détection et de prévention.

**Section 45.3 — Use-after-free et temporal safety.** L'accès à un objet après sa destruction est l'une des classes de bugs les plus dangereuses et les plus difficiles à détecter à la revue de code. Cette section explore le problème, son lien avec les smart pointers et le RAII, et les outils capables de le détecter à l'exécution.

**Section 45.4 — Compilation avec protections.** Le compilateur et le système d'exploitation fournissent des couches de défense en profondeur : `-fstack-protector`, `-D_FORTIFY_SOURCE`, ASLR, PIE. Cette section détaille chaque mécanisme, son coût en performance et sa configuration sur GCC 15 et Clang 20 sous Ubuntu.

**Section 45.5 — Fuzzing avec AFL++ et LibFuzzer.** Le fuzzing est aujourd'hui la technique la plus efficace pour découvrir des vulnérabilités dans du code C++ avant qu'un attaquant ne le fasse. On y couvre la configuration d'AFL++ et de LibFuzzer, l'écriture de harnesses, et l'intégration du fuzzing dans un workflow CI/CD.

**Section 45.6 — Sécurité mémoire : Réponses concrètes du comité C++ en 2026.** Le paysage a changé. Les agences gouvernementales (NSA, CISA) et les régulations européennes (Cyber Resilience Act) exercent une pression croissante sur les langages non memory-safe. Cette section fait le point sur les Safety Profiles, le hardening en production avec les sanitizers, les stratégies de migration progressive vers Rust, et dresse un bilan honnête de la question : un C++ safe-by-default est-il atteignable en 2026 ?

---

## Prérequis

Ce chapitre s'adresse à des développeurs ayant une maîtrise solide du C++ moderne. Les connaissances suivantes sont nécessaires pour en tirer le meilleur parti :

- **Gestion de la mémoire** (chapitre 5) — compréhension de la stack, du heap, de `new`/`delete` et de leurs dangers.  
- **Smart pointers** (chapitre 9) — `std::unique_ptr`, `std::shared_ptr`, `std::weak_ptr` et le principe de possession.  
- **RAII** (section 6.3) — le pattern fondamental qui sous-tend la gestion sûre des ressources en C++.  
- **Outils de détection mémoire** (section 5.5) — familiarité de base avec Valgrind et AddressSanitizer.  
- **Compilation et options** (section 2.6) — savoir compiler avec des flags spécifiques sous GCC et Clang.  
- **CMake** (chapitre 26) — pour intégrer les options de sécurité dans un projet structuré.

---

## Philosophie : la défense en profondeur

La sécurité en C++ ne repose jamais sur une mesure unique. Elle s'appuie sur un empilement de couches complémentaires que l'on peut résumer ainsi :

1. **Le code** — utiliser les abstractions sûres du C++ moderne (`std::vector`, `std::span`, smart pointers, `std::string_view`) plutôt que les primitives C héritées (tableaux bruts, `malloc`/`free`, `char*`).

2. **Le compilateur** — activer les warnings maximaux (`-Wall -Wextra -Wpedantic -Werror`) et les protections intégrées (`-fstack-protector-strong`, `-D_FORTIFY_SOURCE=2`).

3. **L'analyse statique** — exécuter `clang-tidy` et `cppcheck` systématiquement (voir chapitre 32) pour détecter les patterns dangereux avant l'exécution.

4. **Les sanitizers** — compiler régulièrement avec AddressSanitizer, UndefinedBehaviorSanitizer et ThreadSanitizer (voir section 29.4) pour attraper les bugs à l'exécution pendant les tests.

5. **Le fuzzing** — soumettre les parsers, les décodeurs et toute surface d'entrée à un fuzzer pour explorer les chemins d'exécution que les tests unitaires ne couvrent pas.

6. **Le système d'exploitation** — s'appuyer sur ASLR, les piles non exécutables (NX), et les binaires PIE pour réduire l'exploitabilité des bugs résiduels.

Aucune de ces couches n'est suffisante seule. Leur combinaison systématique est ce qui distingue un projet C++ professionnel d'un projet vulnérable.

---

## Liens avec les autres chapitres

| Thème | Chapitre lié |
|---|---|
| Gestion mémoire et dangers | Chapitre 5 — Gestion de la Mémoire |
| Smart pointers et RAII | Chapitres 6 et 9 |
| Sanitizers (ASan, UBSan, TSan, MSan) | Section 29.4 — Sanitizers |
| Analyse statique (clang-tidy, cppcheck) | Chapitre 32 — Analyse Statique et Linting |
| Options de compilation | Section 2.6 — Options de compilation critiques |
| `std::span` et vues sûres | Section 13.5 — std::span |
| `std::expected` et gestion d'erreurs | Section 12.8 — std::expected (C++23) |
| Contrats C++26 | Section 12.14.1 — Contracts |
| Interopérabilité C++/Rust | Section 43.3 — C++ et Rust : FFI et interopérabilité |
| CI/CD et automatisation | Chapitre 38 — CI/CD pour C++ |

---

## Organisation des fichiers

```
45-securite/
├── README.md                          ← ce fichier
├── 01-buffer-overflows.md             # 45.1 Buffer overflows et protection
├── 02-integer-overflows.md            # 45.2 Integer overflows et underflows
├── 03-use-after-free.md               # 45.3 Use-after-free et temporal safety
├── 04-compilation-protections.md      # 45.4 Compilation avec protections
│   ├── 04.1-stack-protector.md        #   -fstack-protector
│   ├── 04.2-fortify-source.md         #   -D_FORTIFY_SOURCE
│   └── 04.3-aslr-pie.md              #   ASLR et PIE
├── 05-fuzzing.md                      # 45.5 Fuzzing avec AFL++, LibFuzzer
│   ├── 05.1-afl.md                    #   Configuration AFL++
│   └── 05.2-libfuzzer.md             #   LibFuzzer et intégration
└── 06-securite-memoire.md             # 45.6 Sécurité mémoire en 2026
    ├── 06.1-contexte-reglementaire.md #   Contexte réglementaire
    ├── 06.2-safety-profiles.md        #   Safety Profiles
    ├── 06.3-hardening-sanitizers.md   #   Hardening production
    ├── 06.4-migration-interop-rust.md #   Migration et interop Rust
    └── 06.5-bilan-safe-cpp-2026.md    #   Bilan safe-by-default
```

---

> **À retenir avant de commencer** : La sécurité n'est pas une fonctionnalité que l'on ajoute à la fin d'un projet. C'est une discipline qui se pratique à chaque ligne de code, à chaque option de compilation, à chaque étape du pipeline CI/CD. Ce chapitre vous donne les outils pour l'intégrer naturellement dans votre workflow quotidien.

⏭️ [Buffer overflows et protection](/45-securite/01-buffer-overflows.md)
