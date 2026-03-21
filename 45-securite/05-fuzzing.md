🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 45.5 — Fuzzing avec AFL++, LibFuzzer ⭐

## Chapitre 45 — Sécurité en C++ ⭐

---

## Introduction

Les sections précédentes ont couvert les vulnérabilités (45.1–45.3) et les protections compilateur/OS qui en réduisent l'exploitabilité (45.4). Mais ces protections ne trouvent pas les bugs — elles limitent les dégâts une fois que le bug est déclenché. Les tests unitaires (chapitre 33) vérifient que le programme se comporte correctement sur les entrées prévues par le développeur. Reste une question cruciale : que se passe-t-il sur les entrées que personne n'a imaginées ?

C'est exactement la question à laquelle le fuzzing répond. Le fuzzing — ou test par injection de données aléatoires — consiste à alimenter un programme avec des entrées générées automatiquement, en très grande quantité, en observant les crashs, les timeouts et les comportements anormaux. Un fuzzer moderne exécute des millions de cas de test par heure, explorant des chemins d'exécution que les tests manuels ne couvriront jamais.

Le fuzzing est aujourd'hui la technique la plus productive pour découvrir des vulnérabilités de sécurité dans du code C++. Google a trouvé plus de 40 000 bugs dans Chrome et ses dépendances grâce au fuzzing continu via le projet OSS-Fuzz. Microsoft, Mozilla, Apple et l'ensemble de l'industrie ont adopté le fuzzing comme composante standard de leurs pipelines de sécurité.

Cette section présente les principes du fuzzing, les deux outils de référence dans l'écosystème C++ (AFL++ et LibFuzzer), et les stratégies pour intégrer le fuzzing dans un projet professionnel.

---

## Principes du fuzzing

### Du fuzzing naïf au fuzzing guidé par couverture

Le fuzzing a considérablement évolué depuis ses origines. Il est utile de distinguer les générations pour comprendre pourquoi les outils modernes sont si efficaces.

**Fuzzing naïf (génération aléatoire).** La forme la plus primitive : générer des entrées purement aléatoires et les injecter dans le programme. Cette approche trouve les bugs les plus évidents — ceux qui se déclenchent sur n'importe quelle entrée malformée — mais elle est incapable d'explorer les chemins d'exécution profonds. Un parser JSON qui commence par vérifier que le premier caractère est `{` rejettera 99,6 % des entrées aléatoires dès le premier octet.

**Fuzzing par mutation.** Au lieu de générer des entrées depuis zéro, le fuzzer part d'un corpus d'entrées valides (seed corpus) et les modifie par des mutations : inversion de bits, insertion d'octets, duplication de blocs, remplacement par des valeurs limites (0, -1, `INT_MAX`, etc.). Cette approche est plus efficace car les mutations produisent des entrées "proches" d'entrées valides, qui ont une meilleure chance de traverser les validations initiales.

**Fuzzing guidé par couverture (coverage-guided).** C'est la révolution qui a rendu le fuzzing industriellement viable. Le fuzzer instrumente le programme pour observer quels chemins d'exécution (branches, blocs de base) sont couverts par chaque entrée. Quand une mutation déclenche un nouveau chemin jamais vu auparavant, l'entrée est conservée dans le corpus et sert de base pour de futures mutations. Le fuzzer converge ainsi progressivement vers une couverture complète du code :

```
Corpus initial         Mutation         Nouvelle couverture ?
┌─────────────┐       ┌─────────┐      ┌────────────────────┐
│ {"key": 42} │──────▶│ {"": 42}│─────▶│ Oui → ajout corpus │
└─────────────┘       └─────────┘      └────────────────────┘
                      ┌─────────────┐  ┌────────────────────┐
                 ────▶│ {"key": -1} │─▶│ Oui → ajout corpus │
                      └─────────────┘  └────────────────────┘
                      ┌─────────────┐  ┌────────────────────┐
                 ────▶│ {"key": 43} │─▶│ Non → rejeté       │
                      │ (même code  │  └────────────────────┘
                      │  path)      │
                      └─────────────┘

        ↻ Boucle continue : muter, exécuter, observer, conserver si nouveau
```

AFL++ et LibFuzzer sont tous les deux des fuzzers guidés par couverture. C'est ce qui les rend capables de trouver des bugs dans du code profondément imbriqué — parsers, machines à états, protocoles complexes — que le fuzzing naïf ne pourrait jamais atteindre.

### Ce que le fuzzing trouve

Le fuzzing est particulièrement efficace pour découvrir les classes de bugs suivantes en C++ :

| Classe de bug | Détection | Mécanisme |
|---|---|---|
| Buffer overflows (stack et heap) | Crash ou ASan | Écriture hors bornes déclenchée par une entrée malformée |
| Use-after-free | ASan | Accès à de la mémoire libérée sur un chemin d'exécution rare |
| Integer overflows | UBSan | Dépassement d'entier signé sur des valeurs limites |
| Null pointer dereference | Crash (SIGSEGV) | Chemin d'erreur mal géré |
| Division par zéro | Crash (SIGFPE) ou UBSan | Valeur inattendue dans un diviseur |
| Assertions violées | Crash (SIGABRT) | Précondition non satisfaite |
| Stack overflow (récursion) | Crash (SIGSEGV) | Entrée déclenchant une récursion profonde |
| Timeouts / boucles infinies | Timeout du fuzzer | Entrée déclenchant une complexité algorithmique excessive |
| Memory leaks | LeakSanitizer | Allocations non libérées sur certains chemins |

La combinaison du fuzzing avec les sanitizers est essentielle : sans ASan, un heap buffer overflow peut ne pas produire de crash (la mémoire adjacente appartient encore au processus) et passer inaperçu. Avec ASan, le moindre accès hors bornes est immédiatement signalé.

### La notion de harness (point d'entrée du fuzzer)

Un fuzzer n'exécute pas le programme entier — il appelle une fonction spécifique en lui passant des données générées. Cette fonction de point d'entrée s'appelle le **harness** (ou fuzz target). Le harness est une fonction qui :

1. Reçoit un buffer de données brutes (ce que le fuzzer génère).
2. Passe ces données à la fonction ou au composant à tester.
3. Ne conserve aucun état entre les appels (chaque exécution est indépendante).

```cpp
// Structure standard d'un harness pour LibFuzzer
#include <cstdint>
#include <cstddef>

// Le fuzzer appelle cette fonction des millions de fois
// avec des données différentes à chaque appel
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    // Passer les données au code à tester
    parse_input(data, size);
    return 0;  // 0 = pas d'erreur de harness
}
```

```cpp
// Structure équivalente pour AFL++ (lecture depuis stdin)
#include <iostream>
#include <vector>
#include <iterator>

int main() {
    // AFL++ alimente stdin avec les données générées
    std::vector<uint8_t> data(
        std::istreambuf_iterator<char>(std::cin),
        std::istreambuf_iterator<char>()
    );
    parse_input(data.data(), data.size());
    return 0;
}
```

La qualité du harness détermine l'efficacité du fuzzing. Un bon harness est minimal (pas de setup inutile), rapide (pas d'I/O disque ou réseau), et expose directement la surface d'attaque ciblée.

---

## AFL++ et LibFuzzer : positionnement

L'écosystème du fuzzing C++ est dominé par deux outils, chacun avec sa philosophie et ses points forts :

### AFL++ (American Fuzzy Lop ++)

AFL++ est le successeur communautaire de l'AFL original créé par Michał Zalewski en 2013. C'est un fuzzer **externe** : il lance le programme comme un processus séparé, injecte les données via stdin ou un fichier, et observe le comportement (crash, timeout, nouveau coverage). AFL++ est le fuzzer le plus polyvalent et le plus configurable :

- **Mode d'instrumentation flexible** — compile-time (via `afl-clang-fast++`), QEMU (binaires non instrumentés), Unicorn (firmware), Frida (instrumentation dynamique).  
- **Stratégies de mutation avancées** — mutations structurées, dictionnaires, power schedules adaptatifs, MOpt (optimisation des opérateurs de mutation).  
- **Fuzzing parallèle natif** — distribution sur plusieurs cœurs ou machines.  
- **Détection de nouveauté fine** — compteurs de hit-count par branche, pas seulement couverture binaire.

AFL++ est le choix naturel lorsqu'on fuzz un programme complet, un outil en ligne de commande, un service réseau, ou lorsqu'on ne peut pas modifier le code source (fuzzing de binaires via QEMU).

### LibFuzzer

LibFuzzer est le fuzzer **in-process** intégré au projet LLVM/Clang. Au lieu de lancer un processus par exécution, LibFuzzer appelle une fonction du programme directement, dans le même processus. Le harness est compilé et linké avec la bibliothèque de fuzzing :

- **Performance extrême** — pas de fork, pas de context switch. LibFuzzer peut exécuter des dizaines de milliers d'itérations par seconde sur une seule fonction.  
- **Intégration Clang native** — un seul flag (`-fsanitize=fuzzer`) suffit à instrumenter et linker.  
- **Combinaison naturelle avec les sanitizers** — ASan, UBSan, MSan fonctionnent directement dans le même processus.  
- **Corpus management intégré** — merge, minimisation, cross-pollination entre corpus.

LibFuzzer est le choix naturel pour le fuzzing de bibliothèques, de fonctions de parsing, de codecs — tout composant exposé comme une API qui prend un buffer en entrée. C'est l'outil utilisé par OSS-Fuzz pour la majorité de ses cibles.

### Comparaison

| Critère | AFL++ | LibFuzzer |
|---|---|---|
| Modèle d'exécution | Externe (processus séparé) | In-process (même processus) |
| Vitesse (itérations/sec) | Milliers | Dizaines de milliers |
| Cible idéale | Programmes complets, CLI, services | Fonctions, parsers, bibliothèques |
| Compilateur | GCC ou Clang | Clang uniquement |
| Instrumentation binaire | Oui (QEMU, Frida) | Non |
| Configuration | Riche, nombreuses options | Minimaliste |
| Intégration CI | Bonne | Excellente (OSS-Fuzz) |

Les deux outils ne sont pas mutuellement exclusifs. Dans un projet C++ sérieux, il est courant d'utiliser LibFuzzer pour les composants de parsing (couverture rapide, intégration CI facile) et AFL++ pour les tests d'intégration plus larges ou les binaires legacy sans harness dédié.

---

## Stratégie de fuzzing pour un projet C++

### Identifier les surfaces d'attaque

Toute fonction qui traite des données provenant de l'extérieur est une cible de fuzzing potentielle. Les priorités, par ordre d'impact sur la sécurité :

1. **Parsers de formats** — JSON, XML, YAML, Protocol Buffers, formats binaires custom. Ce sont les cibles les plus rentables car elles traitent des entrées complexes avec de nombreux chemins d'exécution.

2. **Décodeurs de médias** — images (PNG, JPEG, WebP), audio, vidéo. Historiquement, les bibliothèques de décodage de médias sont parmi les plus vulnérables.

3. **Protocoles réseau** — parsers HTTP, gRPC, protocoles custom. Tout ce qui lit depuis un socket et interprète les données.

4. **Désérialisation** — reconstruction d'objets C++ depuis des données binaires ou textuelles (voir chapitre 25).

5. **Traitement de chaînes** — expressions régulières, templates, interpolation de variables.

### Constituer un seed corpus

Le seed corpus est l'ensemble d'entrées valides qui sert de point de départ aux mutations. Sa qualité a un impact direct sur la vitesse de convergence du fuzzer :

```
corpus/
├── valid_simple.json        # {"name": "test"}
├── valid_nested.json        # {"a": {"b": {"c": 1}}}
├── valid_array.json         # [1, 2, 3, "hello"]
├── valid_unicode.json       # {"emoji": "🎉"}
├── valid_empty_object.json  # {}
├── valid_empty_array.json   # []
├── valid_numbers.json       # {"int": 42, "float": 3.14, "neg": -1}
└── valid_escapes.json       # {"esc": "tab\there\nnewline"}
```

Chaque fichier du corpus devrait exercer un chemin d'exécution différent. Il ne sert à rien d'avoir 100 fichiers qui exercent le même code — le fuzzer guidé par couverture ne les distingue pas et perd du temps à les muter.

Bonnes pratiques pour le seed corpus :

- **Couvrir les cas structurels** — objet vide, tableau vide, imbrications profondes, valeurs aux limites.  
- **Rester minimal** — des fichiers petits (quelques centaines d'octets) permettent des mutations plus rapides.  
- **Utiliser des fichiers réels** — si le programme traite des images PNG, inclure quelques PNG réels de petite taille.  
- **Minimiser le corpus au fil du temps** — `afl-cmin` (AFL++) et `libfuzzer -merge` éliminent les entrées redondantes.

### Utiliser des dictionnaires

Un dictionnaire (dictionary) fournit au fuzzer des tokens spécifiques au format ciblé. Sans dictionnaire, un fuzzer qui mute un fichier JSON a très peu de chances de générer par hasard le token `"null"` ou `"true"`. Avec un dictionnaire, ces tokens sont injectés régulièrement dans les mutations :

```
# json.dict — dictionnaire pour le fuzzing d'un parser JSON
"{"
"}"
"["
"]"
":"
","
"null"
"true"
"false"
"\""
"\\"
"\\n"
"\\u0000"
"-0"
"0.0"
"1e308"
"-1e308"
"1e-308"
```

AFL++ et LibFuzzer supportent tous les deux les dictionnaires au format ci-dessus. De nombreux dictionnaires prêts à l'emploi sont disponibles dans le dépôt AFL++ pour les formats courants (JSON, XML, HTML, HTTP, SQL, etc.).

### Combiner fuzzing et sanitizers

Le fuzzing sans sanitizer est considérablement moins efficace. Un buffer overflow de quelques octets peut ne provoquer aucun crash observable — les données écrasées ne sont peut-être jamais lues, ou elles sont dans une zone allouée mais non utilisée. Le bug passe inaperçu.

Avec ASan, le moindre accès hors bornes est immédiatement détecté et signalé, même s'il n'aurait jamais produit de crash en conditions normales :

```bash
# LibFuzzer avec ASan + UBSan — configuration recommandée
clang++ -std=c++23 -O1 -g \
    -fsanitize=fuzzer,address,undefined \
    -fno-omit-frame-pointer \
    harness.cpp parser.cpp -o fuzz_parser

# AFL++ avec ASan
export AFL_USE_ASAN=1  
afl-clang-fast++ -std=c++23 -O1 -g \  
    -fsanitize=address,undefined \
    harness_afl.cpp parser.cpp -o fuzz_parser_afl
```

La combinaison standard pour le fuzzing de sécurité :

| Sanitizer | Détecte | Surcoût | Recommandation |
|---|---|---|---|
| ASan | Buffer overflows, UAF, double-free, leaks | ~2× | **Toujours activer** |
| UBSan | Signed overflow, null deref, shift overflow | ~5 % | **Toujours activer** |
| MSan | Lecture de mémoire non initialisée | ~3× | Utile, mais incompatible avec ASan |

ASan et UBSan peuvent être activés simultanément. MSan est incompatible avec ASan — il faut un build de fuzzing séparé si on veut les deux couvertures.

---

## Intégration dans le pipeline CI/CD

Le fuzzing n'est réellement efficace que s'il est continu. Un fuzzer qui tourne 10 minutes avant chaque release ne trouvera que les bugs les plus superficiels. Les bugs profonds nécessitent des heures, des jours, voire des semaines de fuzzing cumulé.

### Fuzzing continu

Le modèle recommandé est le **fuzzing continu** : un ou plusieurs jobs de fuzzing tournent en permanence sur une infrastructure dédiée, accumulant de la couverture au fil du temps. Quand un crash est trouvé, une alerte est envoyée et le cas de test minimal est archivé.

Google propose OSS-Fuzz comme service gratuit de fuzzing continu pour les projets open source. Pour les projets privés, la même architecture peut être reproduite avec des runners CI dédiés.

### Structure d'intégration

```
project/
├── src/                    # Code de production
├── tests/                  # Tests unitaires (GTest)
├── fuzz/                   # Harnesses de fuzzing
│   ├── fuzz_json_parser.cpp
│   ├── fuzz_protocol.cpp
│   ├── fuzz_image_decoder.cpp
│   ├── corpus/             # Seed corpus par cible
│   │   ├── json/
│   │   ├── protocol/
│   │   └── images/
│   └── dictionaries/       # Dictionnaires par format
│       ├── json.dict
│       └── protocol.dict
├── CMakeLists.txt
└── .github/workflows/
    └── fuzzing.yml         # Job de fuzzing continu
```

### Intégration CMake

```cmake
option(ENABLE_FUZZING "Build fuzz targets" OFF)

if(ENABLE_FUZZING)
    if(NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        message(FATAL_ERROR "Le fuzzing avec LibFuzzer nécessite Clang")
    endif()

    # Flags communs à toutes les cibles de fuzzing
    set(FUZZ_FLAGS -fsanitize=fuzzer,address,undefined -fno-omit-frame-pointer)

    # Cible de fuzzing : parser JSON
    add_executable(fuzz_json_parser
        fuzz/fuzz_json_parser.cpp
        src/json_parser.cpp
    )
    target_compile_options(fuzz_json_parser PRIVATE ${FUZZ_FLAGS} -O1 -g)
    target_link_options(fuzz_json_parser PRIVATE ${FUZZ_FLAGS})

    # Cible de fuzzing : protocole réseau
    add_executable(fuzz_protocol
        fuzz/fuzz_protocol.cpp
        src/protocol_parser.cpp
    )
    target_compile_options(fuzz_protocol PRIVATE ${FUZZ_FLAGS} -O1 -g)
    target_link_options(fuzz_protocol PRIVATE ${FUZZ_FLAGS})
endif()
```

```bash
# Build des cibles de fuzzing
cmake -B build -DENABLE_FUZZING=ON -DCMAKE_CXX_COMPILER=clang++  
cmake --build build  

# Exécution
./build/fuzz_json_parser fuzz/corpus/json/ -dict=fuzz/dictionaries/json.dict
```

### Gestion des crashs

Quand le fuzzer trouve un crash, il sauvegarde l'entrée qui l'a provoqué dans un fichier. Ce fichier doit être :

1. **Réduit** — le fuzzer propose souvent une commande de minimisation (`-minimize_crash=1` pour LibFuzzer, `afl-tmin` pour AFL++) qui réduit l'entrée au minimum nécessaire pour reproduire le crash.
2. **Analysé** — compiler et exécuter le harness avec ASan et l'entrée minimisée pour obtenir un diagnostic complet (stack trace, type de bug).
3. **Converti en test de régression** — l'entrée qui provoque le crash est ajoutée au corpus et/ou transformée en test unitaire pour éviter la régression :

```cpp
// Test de régression généré depuis un crash de fuzzing
TEST(JsonParser, FuzzRegressionCrash42) {
    // Entrée minimisée qui provoquait un heap-buffer-overflow
    const uint8_t crash_input[] = {0x7b, 0x22, 0x00, 0x22, 0x3a, 0x5b, 0x5d};
    // Ne doit pas crasher après le fix
    EXPECT_NO_FATAL_FAILURE(
        parse_json(crash_input, sizeof(crash_input))
    );
}
```

---

## Ce que le fuzzing ne trouve pas

Le fuzzing est un outil puissant, mais il a des limites qu'il est important de comprendre pour ne pas en surestimer la couverture :

**Bugs de logique métier.** Le fuzzer détecte les crashs et les comportements indéfinis, pas les résultats incorrects. Si un parser JSON retourne `42` au lieu de `43` pour un champ donné, le fuzzer ne le remarquera pas — sauf si un `assert` dans le code transforme le bug logique en crash.

**Vulnérabilités temporelles.** Les race conditions et les bugs de concurrence sont difficiles à trouver par fuzzing car ils dépendent de l'ordonnancement des threads, pas des données d'entrée. ThreadSanitizer (section 29.4.3) est l'outil adapté.

**Chemins nécessitant un état complexe.** Un bug qui ne se produit qu'après une séquence précise d'opérations (connexion → authentification → requête spécifique → déconnexion → reconnexion) est difficile à atteindre par mutation de données brutes. Le fuzzing structuré (structure-aware fuzzing) et le fuzzing basé sur des grammaires aident dans ces cas, mais ils nécessitent un investissement supplémentaire.

**Problèmes de performance et de consommation mémoire.** Un algorithme quadratique n'est pas un bug au sens du fuzzer, sauf si le timeout est configuré de manière assez agressive pour détecter les entrées qui déclenchent une complexité excessive.

---

## Organisation des sous-sections

Les deux sous-sections suivantes fournissent les guides de configuration pratiques pour chaque outil :

- **Section 45.5.1** — Configuration AFL++ : installation, instrumentation, stratégies de mutation, fuzzing parallèle, et intégration CI.  
- **Section 45.5.2** — LibFuzzer et intégration : écriture de harnesses, options de ligne de commande, gestion du corpus, et déploiement via OSS-Fuzz.

---

## Pour aller plus loin

- **Chapitre 33** — Google Test : les tests de régression qui complètent le fuzzing en pérennisant les crashs découverts.  
- **Section 29.4** — Sanitizers : ASan, UBSan, MSan — compagnons indispensables du fuzzing.  
- **Section 38.2** — GitHub Actions : infrastructure pour les jobs de fuzzing continu.  
- **Section 45.1** — Buffer overflows : la classe de vulnérabilité la plus fréquemment découverte par fuzzing.  
- **Section 45.3** — Use-after-free : la deuxième classe la plus fréquemment découverte par fuzzing.  
- **Section 45.6** — Sécurité mémoire en 2026 : le fuzzing comme composante d'une stratégie de sécurité complète.

⏭️ [Configuration AFL++](/45-securite/05.1-afl.md)
