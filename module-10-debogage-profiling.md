🔝 Retour au [Sommaire](/SOMMAIRE.md)

# Module 10 — Débogage, Profiling et Qualité Code

> 🎯 Niveau : Avancé

Ce module couvre les outils de diagnostic — ce qui vous permet de trouver les bugs, les fuites mémoire, les hotspots de performance, et les problèmes de qualité de code avant qu'ils n'atteignent la production. GDB, quatre sanitizers (Address, UndefinedBehavior, Thread, Memory), Valgrind, Massif, `perf`, flamegraphs, clang-tidy, cppcheck, clang-format. Chaque outil a un rôle précis et des contraintes d'utilisation spécifiques — les combiner incorrectement produit des résultats faux ou aucun résultat.

---

## Objectifs pédagogiques

1. **Déboguer** un programme C++ avec GDB : breakpoints conditionnels, watchpoints, inspection de la mémoire, core dumps pour le post-mortem debugging.
2. **Instrumenter** le code avec les quatre sanitizers (ASan, UBSan, TSan, MSan) et interpréter leurs rapports.
3. **Détecter** les fuites mémoire et les erreurs d'accès avec Valgrind (memcheck) et profiler les allocations heap avec Massif.
4. **Profiler** la performance CPU avec `perf` (sampling, compteurs matériels) et visualiser les résultats avec des flamegraphs.
5. **Configurer** clang-tidy et cppcheck pour l'analyse statique, et clang-format pour le formatage automatique.
6. **Intégrer** ces outils dans le workflow de développement quotidien (CMake, IDE, pre-commit).

---

## Prérequis

- **Module 2, chapitre 5** : gestion mémoire manuelle (stack, heap, pointeurs) — nécessaire pour comprendre ce que Valgrind et AddressSanitizer détectent.
- **Module 7, chapitre 21** : threads et concurrence — nécessaire pour exploiter ThreadSanitizer et comprendre ses rapports de data races.
- **Module 9, chapitre 26** : CMake — les sanitizers, Valgrind, la couverture de code et l'analyse statique s'intègrent dans le build system via des target properties et des CMake Presets.
- **Module 1, chapitre 2, section 2.6** : options de compilation (`-g`, `-O0`, `-O2`) — le choix du niveau d'optimisation et des symboles de debug conditionne l'utilisabilité de chaque outil.

---

## Chapitres

### Chapitre 29 — Débogage Avancé

Débogage interactif avec GDB, intégration IDE, analyse post-mortem via core dumps, et les quatre sanitizers du compilateur. Ce chapitre est l'outil de premier recours quand un programme se comporte incorrectement.

- **GDB** : commandes de navigation (`run`, `step`, `next`, `continue`, `finish`), inspection (`print`, `display`, `watch`, `info locals`, `info threads`), breakpoints conditionnels (`break file.cpp:42 if count > 100`), watchpoints sur les variables.
- **Débogage via IDE** : configuration de launch.json (VS Code) et des run configurations (CLion), intégration GDB/LLDB transparente, conditional breakpoints visuels.
- **Core dumps** : activation (`ulimit -c unlimited`), analyse post-mortem avec `gdb ./program core`, extraction de la stack trace et de l'état des variables au moment du crash.
- **Sanitizers** : AddressSanitizer (`-fsanitize=address` — buffer overflow, use-after-free, stack overflow), UndefinedBehaviorSanitizer (`-fsanitize=undefined` — signed overflow, null deref, misaligned access), ThreadSanitizer (`-fsanitize=thread` — data races), MemorySanitizer (`-fsanitize=memory` — lecture de mémoire non initialisée, Clang uniquement).
- **`std::stacktrace`** (C++23) : intégration des traces d'exécution dans le code de débogage, complémentaire aux sanitizers.

### Chapitre 30 — Analyse Mémoire

Détection et diagnostic des problèmes mémoire en profondeur. Valgrind est l'outil de référence pour les fuites et les accès invalides ; Massif profiler les allocations heap pour identifier les consommateurs de mémoire.

- **Valgrind memcheck** : détection de fuites (`definitely lost`, `indirectly lost`, `possibly lost`, `still reachable`), accès invalides (read/write après free, buffer overflow), lectures de mémoire non initialisée. Overhead : ~20x ralentissement.
- **Lecture des rapports Valgrind** : interprétation des stack traces, suppression des faux positifs avec les fichiers `.supp`, intégration dans les tests automatisés.
- **Massif** : heap profiling — visualisation de l'évolution des allocations dans le temps, identification des fonctions qui consomment le plus de mémoire, `ms_print` pour les rapports texte.
- **Memory leaks** : stratégies de détection et de résolution — pattern d'investigation, corrélation avec le code source, vérification post-fix.

### Chapitre 31 — Profiling de Performance

Identification des hotspots CPU et optimisation guidée par les données. Ce chapitre couvre `perf` (le profiler standard de Linux), `gprof` (historique), et la visualisation avec des flamegraphs.

- **`perf record` et `perf report`** : sampling statistique — enregistrement des échantillons CPU, rapport par fonction et par ligne source. Avantage : overhead minimal (~2%), profiling de code en production possible.
- **`perf stat`** : compteurs matériels (cache misses, branch mispredictions, instructions per cycle) — diagnostic de problèmes de localité mémoire et de prédiction de branchement.
- **gprof** : profiling par instrumentation (`-pg`) — comptage exact des appels de fonctions. Limitation : overhead plus élevé, incompatible avec certaines optimisations, approche historique.
- **Flamegraphs** : visualisation hiérarchique des profils `perf` — identification visuelle immédiate des chemins chauds. Génération avec les scripts de Brendan Gregg ou l'outil Hotspot.
- **Hotspot** : interface graphique pour les données `perf`, alternative visuelle à `perf report`.

### Chapitre 32 — Analyse Statique et Linting

Détection de bugs et de mauvaises pratiques sans exécuter le code. clang-tidy pour l'analyse sémantique, cppcheck pour les erreurs classiques, clang-format pour imposer un style uniforme.

- **clang-tidy** : analyse statique moderne intégrée à l'écosystème LLVM. Configuration via `.clang-tidy` (fichier YAML), checks organisés par catégorie (`bugprone-*`, `modernize-*`, `performance-*`, `readability-*`, `cppcoreguidelines-*`). Checks recommandés comme point de départ minimal.
- **cppcheck** : détection d'erreurs classiques (null pointer dereference, division par zéro, buffer overflow statiquement détectable) — complémentaire à clang-tidy, moins de faux positifs.
- **clang-format 19** : formatage automatique basé sur un fichier `.clang-format`, styles prédéfinis (LLVM, Google, Chromium, Mozilla) ou style personnalisé. Garantit un formatage uniforme sans débat.
- **Intégration dans le workflow** : exécution automatique via CMake custom targets, intégration IDE (VS Code, CLion), pre-commit hooks (préparation pour le Module 17, chapitre 47).

---

## Points de vigilance

- **Optimisation `-O2`/`-O3` qui rend GDB inutilisable.** Le compilateur réordonne les instructions, élimine les variables, inline les fonctions — le debugger ne peut plus mapper le code machine au code source. Les variables apparaissent comme `<optimized out>`, les breakpoints se déclenchent à des lignes inattendues. Solution : utiliser `-Og` (optimisations compatibles avec le debug, GCC) ou `-O0` pour le débogage. Pour profiler du code optimisé, gardez `-O2` mais ajoutez `-g` pour conserver les symboles (voir point suivant).

- **Valgrind et AddressSanitizer utilisés simultanément.** Les deux outils instrumentent les accès mémoire mais de manière incompatible — Valgrind fonctionne par émulation binaire, ASan par instrumentation à la compilation. Les lancer ensemble produit des faux positifs, des crashes, ou une détection dégradée. Utilisez-les séparément : ASan dans le build de développement (plus rapide, overhead ~2x), Valgrind pour les analyses approfondies (memcheck, Massif, overhead ~20x). De même, TSan et ASan sont mutuellement exclusifs — un build par sanitizer.

- **`perf` sans symboles de débogage.** `perf record` / `perf report` sans symboles affiche des adresses hexadécimales au lieu des noms de fonctions. Même en build Release, ajoutez `-g` (ou `-g1` pour les symboles minimaux) pour que `perf` puisse résoudre les symboles. Sur les librairies système, installez les paquets `-dbgsym` (`sudo apt install libc6-dbg`). Sans symboles, un flamegraph est illisible.

- **clang-tidy trop verbeux sans configuration.** Lancer clang-tidy sans fichier `.clang-tidy` active tous les checks par défaut — des centaines de warnings sur un projet existant, dont beaucoup de faux positifs ou de suggestions non pertinentes. Le résultat est inutilisable et décourage l'adoption. Partez d'un `.clang-tidy` minimal avec quelques catégories (`bugprone-*`, `performance-*`, `modernize-use-override`) et étendez progressivement. Un outil de qualité qui n'est pas utilisé parce qu'il est trop bruyant ne sert à rien.

---

## Compétences acquises

À l'issue de ce module, vous savez :
- Déboguer un programme C++ avec GDB (breakpoints conditionnels, watchpoints, core dumps) et via l'IDE.
- Utiliser les quatre sanitizers (ASan, UBSan, TSan, MSan) sur les bons builds, sans les combiner de manière incompatible.
- Détecter et résoudre les fuites mémoire avec Valgrind memcheck, et profiler les allocations avec Massif.
- Profiler la performance CPU avec `perf`, lire les compteurs matériels avec `perf stat`, et générer des flamegraphs exploitables.
- Configurer clang-tidy avec un `.clang-tidy` adapté au projet, et clang-format pour un formatage uniforme.
- Intégrer ces outils dans CMake et dans le workflow de développement (IDE, pre-commit).

---


⏭️ [Débogage Avancé](/29-debogage/README.md)
