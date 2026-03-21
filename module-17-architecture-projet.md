🔝 Retour au [Sommaire](/SOMMAIRE.md)

# Module 17 — Architecture de Projet Professionnel

> 🎯 Niveau : Expert

Ce module ferme la formation sur les aspects organisationnels et humains du développement C++ en équipe. Structure de projet, standards de codage, documentation Doxygen, pre-commit hooks, workflows Git, code reviews, dette technique, semantic versioning, veille technologique. Ce sont des compétences qui ne s'apprennent pas dans les livres sur le langage — elles s'apprennent en maintenant des projets sur la durée, avec d'autres développeurs. Ce module concerne autant les soft skills (code review, gestion de la dette, collaboration) que les compétences techniques (pre-commit, Doxygen, semver). Un développeur expert qui ne sait pas organiser un projet pour qu'il survive à son départ n'est pas un expert complet.

---

## Objectifs pédagogiques

1. **Structurer** un projet C++ professionnel : organisation des répertoires (`src/`, `include/`, `tests/`, `docs/`), séparation `.h`/`.cpp` pour la compilation incrémentale, namespaces.
2. **Documenter** du code avec Doxygen et générer une documentation technique exploitable par l'équipe.
3. **Choisir** et appliquer un standard de codage (Google C++ Style Guide, LLVM Style, C++ Core Guidelines), et l'imposer automatiquement.
4. **Configurer** un pipeline pre-commit complet pour C++ : clang-format, clang-tidy, tests rapides — exécutés avant chaque commit.
5. **Mener** des code reviews efficaces et gérer la dette technique de manière structurée.
6. **Appliquer** le Semantic Versioning sur les librairies internes et maintenir des changelogs exploitables.
7. **Construire** une veille technologique durable : livres de référence, conférences, processus ISO, communautés, et distinction entre standards ratifiés et proposals en cours.

---

## Prérequis

- **Module 10, chapitre 32** : clang-tidy et clang-format — les pre-commit hooks de ce module automatisent ces outils déjà maîtrisés.
- **Module 9, chapitre 26** : CMake — la structure de projet et la compilation incrémentale s'appuient sur un `CMakeLists.txt` correctement organisé.
- **Module 13, chapitre 38** : CI/CD — les pre-commit hooks sont le complément local du pipeline CI. Les deux doivent être cohérents.
- **Modules 1 à 16** complétés (pour la Conclusion) : le récapitulatif des compétences et la checklist Cloud Native synthétisent l'ensemble de la formation.

---

## Chapitres

### Chapitre 46 — Organisation et Standards

La structure technique d'un projet C++ maintenable à long terme. Ce chapitre couvre l'organisation des fichiers, la documentation, et le choix d'un standard de codage — les trois fondations sur lesquelles repose la lisibilité du projet pour l'ensemble de l'équipe.

- **Organisation des répertoires** : convention `src/` (sources), `include/` (headers publics), `tests/` (tests unitaires), `docs/` (documentation), `cmake/` (modules CMake), `third_party/` ou gestion via Conan/vcpkg.
- **Séparation `.h`/`.cpp`** : impact sur la compilation incrémentale (modifier un `.cpp` ne recompile que cette unité, modifier un `.h` recompile tout ce qui l'inclut), forward declarations pour réduire les dépendances entre headers.
- **Namespaces** : éviter la pollution de l'espace de noms global, convention de nommage (`projet::module::detail`), ne jamais utiliser `using namespace std;` dans un header.
- **Doxygen** : syntaxe des commentaires (`@brief`, `@param`, `@return`, `@throws`, `@see`), configuration du `Doxyfile` (extraction de la documentation, génération HTML, graphes de dépendances), intégration dans le build CMake.
- **Standards de codage** : Google C++ Style Guide (le plus prescriptif, très répandu), LLVM Style (utilisé par le projet LLVM/Clang), C++ Core Guidelines (co-écrit par Bjarne Stroustrup et Herb Sutter, le plus orienté C++ moderne). Critères de choix : préférence d'équipe, écosystème existant, outillage (clang-tidy implémente les checks des Core Guidelines).

### Chapitre 47 — Collaboration et Maintenance

La dimension collaborative du développement C++. Ce chapitre transforme les bonnes pratiques individuelles en processus d'équipe reproductibles — workflows Git, pre-commit hooks, code reviews, dette technique, versioning.

- **Git et workflows** : GitFlow (branches `develop`, `feature/*`, `release/*`, `hotfix/*` — adapté aux releases planifiées) vs trunk-based development (commits fréquents sur `main`, feature flags — adapté à la CI/CD). Critères de choix selon la taille de l'équipe et le rythme de release.
- **Pre-commit hooks** : framework `pre-commit` (Python), installation (`pre-commit install`), fichier `.pre-commit-config.yaml` versionné dans le repo. Hooks essentiels pour C++ : clang-format (formatage automatique), clang-tidy (analyse statique), tests rapides (subset de tests unitaires qui s'exécutent en quelques secondes).
- **Configuration pre-commit** : intégration clang-format (reformater les fichiers modifiés uniquement), intégration clang-tidy (analyser les fichiers modifiés uniquement, pas tout le projet), tests rapides (tag Google Test `[fast]` ou suite dédiée).
- **Code reviews** : ce qu'il faut vérifier (design, correctness, edge cases, nommage, tests), ce qu'il ne faut pas vérifier (formatage — c'est le job de clang-format), taille des pull requests (petites et fréquentes), feedback constructif.
- **Gestion de la dette technique** : identification (code smells, TODO, warnings ignorés, couverture en baisse), quantification (métriques clang-tidy, couverture de code, temps de build), priorisation (impact vs effort), allocation de temps (sprint de refactoring, règle du scout).
- **Semantic Versioning** : `MAJOR.MINOR.PATCH`, quand incrémenter chaque composante, lien avec la stabilité ABI pour les librairies C++ (changement de `MAJOR` = rupture ABI possible), changelogs structurés (format Keep a Changelog).

### Chapitre 48 — Ressources et Veille Technologique

Les ressources pour continuer à progresser après la formation. Ce chapitre est une boîte à outils pour la veille technologique — livres, conférences, processus de standardisation, communautés.

- **Livres de référence** : *Effective C++* (Scott Meyers — bonnes pratiques fondamentales), *C++ Concurrency in Action* (Anthony Williams — concurrence en profondeur), *A Tour of C++* (Bjarne Stroustrup — vue d'ensemble moderne), *Embracing Modern C++ Safely* (Lakos et al. — adoption progressive des features modernes en contexte industriel).
- **Conférences** : CppCon (la plus grande, talks disponibles sur YouTube), Meeting C++ (européenne, Berlin), C++ Now (orientée experts), ACCU Conference (UK, qualité technique élevée).
- **Standards et évolutions futures** : C++26 ratifié, calendrier du comité ISO (cycle de 3 ans), processus de standardisation (proposals → étude → wording → vote), suivi des proposals sur open-std.org et GitHub (cplusplus/papers), premières proposals C++29 — ce qui se prépare.
- **Communautés** : Stack Overflow (Q&A technique), Reddit r/cpp (actualités et discussions), Discord C++ (échanges en temps réel), Compiler Explorer / godbolt.org (exploration du code assembleur généré, test de features entre compilateurs).

### Conclusion

Synthèse de l'ensemble de la formation : récapitulatif des compétences acquises, trajectoires professionnelles (system programming, backend haute performance, embedded/IoT, finance quantitative, DevOps/SRE, game development), ressources complémentaires, et checklist opérationnelle du développeur C++ Cloud Native.

---

## Points de vigilance

- **Doxygen mal configuré qui génère du bruit.** Un `Doxyfile` par défaut documente tout — y compris les détails d'implémentation internes, les membres privés, et les headers de librairies tierces. Le résultat est une documentation volumineuse et inexploitable. Configurez `EXTRACT_PRIVATE = NO`, `EXCLUDE_PATTERNS` pour les dépendances externes, et `FILE_PATTERNS` pour ne documenter que vos sources. Documentez les interfaces publiques (`.h`) en priorité — la documentation de l'implémentation interne a une durée de vie courte.

- **Pre-commit hooks trop lents qui poussent au `--no-verify`.** Si un pre-commit hook prend plus de 10-15 secondes, les développeurs commencent à le bypasser avec `git commit --no-verify`. clang-tidy sur tout le projet est trop lent pour un hook — limitez-le aux fichiers modifiés (`--diff-filter`). Les tests rapides doivent s'exécuter en quelques secondes (subset taggé, pas la suite complète). Mesurez le temps d'exécution du hook et optimisez-le. Le pipeline CI reste le filet de sécurité pour les vérifications exhaustives.

- **Semantic Versioning ignoré dans les librairies internes.** Une librairie interne sans versioning sémantique qui change son API publique ou son ABI sans incrémenter le `MAJOR` cause des ruptures silencieuses dans les projets consommateurs — erreurs de link, comportements incorrects, crashes. Même les librairies internes méritent un versioning rigoureux. En C++, un changement d'ABI (ajout d'un membre à une classe, modification d'une vtable) est une rupture `MAJOR`, pas `MINOR`. Utilisez `cmake` avec `set_target_properties(... VERSION x.y.z SOVERSION x)` pour les shared libraries.

- **Veille technologique : confondre proposals et standards.** Une proposal C++29 listée sur open-std.org n'est pas du C++ — c'est une suggestion en cours d'étude qui peut être modifiée, reportée, ou rejetée. Baser du code de production sur une proposal non ratifiée, c'est s'engager sur une API instable. Règle : utilisez les standards ratifiés (C++26 et antérieurs) en production, suivez les proposals pour anticiper les évolutions, mais ne les adoptez pas avant ratification et support compilateur stable. Compiler Explorer (godbolt.org) permet de vérifier le support réel d'une feature sur GCC et Clang avant de l'utiliser.

---

## Compétences acquises

À l'issue de ce module, vous savez :
- Structurer un projet C++ professionnel avec une organisation de répertoires, des namespaces, et une séparation header/source qui optimisent la compilation incrémentale.
- Documenter les interfaces publiques avec Doxygen et générer une documentation HTML exploitable.
- Choisir et appliquer un standard de codage, imposé automatiquement par clang-format et clang-tidy via des pre-commit hooks.
- Choisir un workflow Git adapté à votre équipe et mener des code reviews qui améliorent réellement la qualité du code.
- Gérer la dette technique de manière structurée (identification, quantification, priorisation) et appliquer le Semantic Versioning sur les librairies.
- Construire une veille technologique durable en distinguant les standards ratifiés des proposals en cours, et en exploitant les conférences, livres et communautés de référence.

---


⏭️ [Organisation et Standards](/46-organisation-standards/README.md)
