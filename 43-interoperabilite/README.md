🔝 Retour au [Sommaire](/SOMMAIRE.md)

# Chapitre 43 — C++ et Autres Langages

## Module 15 : Interopérabilité · Niveau Expert

---

## Pourquoi l'interopérabilité est incontournable

Aucun projet logiciel d'envergure ne vit en vase clos. Une application de trading haute fréquence expose ses moteurs de calcul C++ à des dashboards Python. Un pipeline de données embarqué communique via FFI entre un noyau C++ et des modules Rust. Un outil de simulation scientifique compile son cœur numérique en WebAssembly pour tourner dans un navigateur. Dans chacun de ces cas, le C++ ne disparaît pas : il **coopère**.

L'interopérabilité — la capacité d'un programme C++ à communiquer avec du code écrit dans un autre langage — est devenue une compétence structurante pour tout développeur système ou backend en 2026. Trois forces convergentes expliquent cette évolution :

**La spécialisation des langages.** Python domine le prototypage, le machine learning et le scripting. Rust s'impose progressivement dans les couches critiques où la sécurité mémoire est exigée par la réglementation. JavaScript reste le langage universel du navigateur. Aucun d'entre eux ne remplace le C++ ; ils le complètent. L'enjeu n'est plus de choisir *un* langage, mais de faire collaborer *plusieurs* langages au sein d'une même architecture.

**La pression réglementaire sur la sécurité mémoire.** Les directives de la NSA, de la CISA et le Cyber Resilience Act européen poussent les organisations à réduire leur surface d'exposition aux vulnérabilités mémoire. La réponse pragmatique n'est pas de réécrire des millions de lignes de C++ du jour au lendemain, mais d'introduire Rust ou d'autres langages memory-safe **aux frontières** des composants les plus sensibles, tout en conservant le C++ existant. Cette stratégie de migration progressive repose entièrement sur une interopérabilité maîtrisée.

**L'essor du WebAssembly.** Wasm a transformé le navigateur en plateforme d'exécution quasi-native. Des bases de code C++ historiques — moteurs 3D, codecs, bibliothèques de calcul — peuvent désormais atteindre le web sans réécriture, à condition de savoir les compiler et les interfacer correctement via Emscripten.

---

## Le défi fondamental : l'ABI et les frontières entre langages

Faire communiquer deux langages ne se résume pas à appeler une fonction. Chaque langage possède ses propres conventions : représentation des types en mémoire, conventions d'appel (*calling conventions*), gestion de la durée de vie des objets, propagation des erreurs. Le C++ ajoute une complexité supplémentaire avec le *name mangling* — la transformation des noms de symboles par le compilateur pour supporter la surcharge de fonctions, les namespaces et les templates.

Le dénominateur commun, dans la quasi-totalité des scénarios d'interopérabilité, est **l'ABI C**. Le langage C offre une interface binaire stable, prévisible et universellement comprise : pas de mangling, pas d'exceptions, des types de taille fixe. C'est pourquoi la déclaration `extern "C"` en C++ est le point de passage obligé vers le monde extérieur. Qu'il s'agisse de pybind11, du crate `cxx` en Rust ou d'Emscripten pour WebAssembly, chaque outil d'interopérabilité repose — directement ou indirectement — sur cette frontière C.

Comprendre cette mécanique est essentiel avant d'utiliser les outils de plus haut niveau qui l'abstraient.

---

## Ce que couvre ce chapitre

Ce chapitre explore quatre axes d'interopérabilité, du plus fondamental au plus spécialisé :

**C++ ↔ C** — Le socle. La section 43.1 détaille le mécanisme `extern "C"`, la compatibilité ABI, et les techniques pour exposer une API C++ à travers une interface C stable. C'est la brique sur laquelle tout le reste s'appuie.

**C++ ↔ Python** — Le binôme le plus répandu. La section 43.2 couvre pybind11 et son successeur nanobind pour exposer des fonctions et des classes C++ à Python avec un minimum de *boilerplate*. C'est l'interopérabilité la plus courante en data science, en machine learning et dans l'outillage interne.

**C++ ↔ Rust** — L'axe stratégique de 2026. La section 43.3 traite de l'interopérabilité avec Rust via `extern "C"`, le bridge `cxx` et autocxx. Elle aborde également la stratégie de migration progressive — quand et comment introduire Rust dans une base de code C++ existante — et les retours d'expérience de l'industrie.

**C++ → WebAssembly** — La compilation pour le web. La section 43.4 présente Emscripten et le workflow de compilation d'un programme C++ vers Wasm, avec intégration JavaScript.

---

## Prérequis

Ce chapitre suppose acquis les concepts suivants, traités dans les modules précédents :

- Le cycle de compilation C++ complet : préprocesseur, compilation, édition de liens (chapitre 1, sections 1.3 et 1.4).  
- La distinction entre linkage statique et dynamique (section 27.4).  
- Les smart pointers et la gestion moderne de la mémoire (chapitre 9).  
- Les bases de CMake : targets, `target_link_libraries`, `find_package` et `FetchContent` (chapitre 26).  
- Une familiarité minimale avec Python est utile pour la section 43.2, et avec les concepts de Rust (*ownership*, *borrowing*) pour la section 43.3 — sans être un prérequis strict, car chaque section introduit le contexte nécessaire.

---

## Vue d'ensemble des outils et approches

| Interopérabilité | Outil principal | Mécanisme sous-jacent | Cas d'usage typique |
|---|---|---|---|
| C++ ↔ C | `extern "C"`, headers C | ABI C directe | Bibliothèques partagées, plugins |
| C++ ↔ Python | pybind11 / nanobind | ABI C + CPython API | Data science, scripting, ML |
| C++ ↔ Rust | `cxx` / autocxx | ABI C + code généré | Migration sécurité mémoire, composants critiques |
| C++ → Wasm | Emscripten | LLVM → Wasm backend | Applications web, portage de code existant |

> 🔥 **Tendance 2026 :** l'interopérabilité C++ ↔ Rust est passée du stade expérimental à un enjeu industriel concret. Les grandes organisations (Google, Microsoft, Meta) documentent publiquement leurs stratégies de cohabitation entre les deux langages. La section 43.3 reflète cet état de l'art.

---

*Les sections suivantes détaillent chaque axe d'interopérabilité, en commençant par la brique fondamentale : la frontière entre C++ et C.*

⏭️ [C++ et C : extern "C" et ABI compatibility](/43-interoperabilite/01-cpp-et-c.md)
