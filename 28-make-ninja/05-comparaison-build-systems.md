🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 28.5 Comparaison Make vs Ninja vs Meson : Performances et cas d'usage

> **Objectif** : Synthétiser les forces et faiblesses de Make, Ninja et Meson en une comparaison structurée, et fournir un guide de décision clair pour choisir le bon outil selon le contexte.

---

## Clarification préalable : ce qu'on compare

Avant de comparer, rappelons que ces trois outils n'occupent pas la même position dans le pipeline de build (section 28, introduction) :

- **Make** est un **build tool** qui peut aussi servir de build system artisanal (Makefile écrit à la main).
- **Ninja** est un **build tool pur** — toujours piloté par un outil de plus haut niveau.
- **Meson** est un **build system** (méta-build system) qui génère pour Ninja.

La comparaison directe la plus pertinente est :

| Configuration | Niveau 3 (méta-build) | Niveau 2 (build tool) |
|--------------|----------------------|----------------------|
| CMake + Make | CMake | Make |
| CMake + Ninja | CMake | Ninja |
| Meson + Ninja | Meson | Ninja |
| Makefile artisanal | *(aucun)* | Make |

Comparer « Make vs Ninja » revient à comparer deux **build tools**. Comparer « CMake vs Meson » revient à comparer deux **build systems**. Comparer « Make vs Meson » n'a pas vraiment de sens — ils ne sont pas au même niveau d'abstraction, sauf quand Make est utilisé comme build system artisanal.

Cette section compare les trois outils sous l'angle **pratique** : pour un développeur C++ en 2026, quel outil (ou quelle combinaison) choisir ?

---

## Comparaison technique

### Performances de build

| Métrique | Make | Ninja | Meson (via Ninja) |
|----------|------|-------|-------------------|
| Temps de chargement (projet moyen) | 300–800 ms | 10–30 ms | 10–30 ms (Ninja) |
| No-op build (projet moyen) | 500 ms – 1.3 s | 30–80 ms | 30–80 ms (Ninja) |
| No-op build (grand projet) | 2–10 s | 100–200 ms | 100–200 ms (Ninja) |
| Parallélisation | Explicite (`-j`) | Automatique | Automatique (Ninja) |
| Détection de changement de commande | ❌ Timestamps seuls | ✅ Timestamps + hash | ✅ Timestamps + hash (Ninja) |
| Compilation incrémentale | Correcte si dépendances complètes | Correcte + rapide | Correcte + rapide |

Le constat est sans appel pour la performance du build tool : **Ninja surpasse Make dans toutes les métriques mesurables**. Meson génère pour Ninja, donc il bénéficie des mêmes performances. L'écart est le plus marqué sur les no-op builds et les rebuilds incrémentaux — les scénarios quotidiens du développement.

### Temps de configuration

| Outil | Temps de configuration typique | Remarque |
|-------|-------------------------------|----------|
| CMake → Make | 2–10 s | Génère une hiérarchie de Makefiles |
| CMake → Ninja | 2–10 s | Génère un fichier build.ninja unique |
| Meson → Ninja | 0.5–3 s | Parser plus rapide que CMake |

La configuration (étape `cmake -B build` ou `meson setup build`) est dominée par le méta-build system, pas par le build tool. Meson configure plus vite que CMake grâce à son langage plus simple à parser. Cependant, la configuration n'est exécutée que quand les fichiers de build changent — c'est une opération peu fréquente comparée aux compilations.

### Expressivité et lisibilité

| Critère | Makefile artisanal | CMake | Meson |
|---------|-------------------|-------|-------|
| Lisibilité pour un débutant | Moyenne | Faible (legacy + modern) | Bonne |
| Expressivité du langage | Turing-complet | Turing-complet | Non Turing-complet |
| Risque de mauvais usage | Élevé | Moyen (ancien vs modern) | Faible |
| Courbe d'apprentissage | Modérée | Raide | Douce |
| Quantité de boilerplate | Faible (petit projet) / Élevée (grand) | Moyenne | Faible |

Meson gagne en lisibilité et en sécurité d'usage. CMake gagne en expressivité et en flexibilité. Make artisanal est adapté aux petits projets mais ne passe pas à l'échelle.

### Écosystème et intégration

| Critère | Make | CMake (+Ninja) | Meson (+Ninja) |
|---------|------|----------------|----------------|
| Bibliothèques tierces (find_package) | ❌ Manuel | ✅ ~Universel | ⚠️ pkg-config + wraps |
| Conan / vcpkg | ❌ | ✅ Intégration native | ⚠️ Partiel |
| Support IDE (CLion, VS Code, VS) | Basique | ✅ Natif, première classe | ⚠️ Extensions, en progrès |
| CI/CD (GitHub Actions, GitLab CI) | ✅ Trivial | ✅ Presets, matrix builds | ✅ Bon support |
| Cross-compilation | Manuel | ✅ Toolchain files | ✅ Cross files |
| Modules C++20 | ❌ | ✅ CMake 4.0+ | ⚠️ En cours |
| Portabilité Windows | ⚠️ (MinGW/MSYS2) | ✅ Natif (MSVC, Ninja, VS) | ✅ Bon (Ninja, VS) |
| Paquets système Linux | ❌ | ⚠️ | ✅ pkg-config natif |

L'écosystème est le **facteur déterminant** dans le choix. CMake domine pour les projets C++ multiplateforme avec des dépendances tierces. Meson domine pour les projets Linux natifs intégrés à l'écosystème freedesktop. Make artisanal est hors course pour les projets de taille non triviale.

---

## Guide de décision

### Quel build tool sous CMake ?

Si vous utilisez CMake (ce que cette formation recommande), le choix du build tool est simple :

```
Utilisez Ninja. Point final.
```

Il n'existe aucun scénario courant en 2026 où Make serait préférable à Ninja comme backend de CMake. Ninja est plus rapide, détecte les changements de commande, parallélise automatiquement, et supporte `compile_commands.json` (via `CMAKE_EXPORT_COMPILE_COMMANDS=ON`). L'installation est triviale (`apt install ninja-build`).

L'exception théorique — un système embarqué minimal où Ninja ne peut pas être installé — est suffisamment rare pour ne pas influencer la recommandation générale.

### Quel build system pour un nouveau projet C++ ?

```
Votre projet cible principalement l'écosystème Linux/freedesktop ?
│
├── OUI (bibliothèque GNOME, pilote, composant système)
│   │
│   Vous avez besoin de Conan/vcpkg ou de nombreuses dépendances CMake ?
│   │
│   ├── OUI → CMake + Ninja
│   │         L'écosystème de dépendances l'emporte.
│   │
│   └── NON (dépendances système via pkg-config)
│       └── → Meson + Ninja
│             Simplicité, pkg-config natif, conventions freedesktop.
│
└── NON (projet multiplateforme, industrie, nombreuses dépendances)
    └── → CMake + Ninja
          Standard de l'industrie, écosystème complet.
```

### Quand un Makefile artisanal reste pertinent

Un Makefile écrit à la main reste raisonnable dans des cas spécifiques :

- **Petit outil autonome** (< 10 fichiers, 0-1 dépendances) : un Makefile de 30 lignes est plus léger qu'un projet CMake.
- **Automatisation de tâches** non liées à la compilation C++ (génération de documentation, scripts de déploiement, orchestration de commandes) : Make est un excellent task runner.
- **Makefile wrapper** autour de CMake : un `Makefile` à la racine qui fournit `make test`, `make format`, `make docker` comme raccourcis vers les commandes CMake/Ninja correspondantes.
- **Environnements pédagogiques** : comprendre Make est un prérequis pour comprendre le fonctionnement des build systems en général.

---

## Combinaisons recommandées par contexte

| Contexte | Combinaison recommandée | Justification |
|----------|------------------------|---------------|
| Projet C++ professionnel, multiplateforme | **CMake + Ninja** + Conan/vcpkg | Standard de l'industrie, écosystème complet |
| Bibliothèque open source C++ | **CMake + Ninja** | Maximum de consommateurs potentiels |
| Projet Linux system / freedesktop | **Meson + Ninja** | pkg-config natif, conventions Linux |
| Petit outil CLI personnel | **Makefile artisanal** ou CMake | Proportionné à la taille du projet |
| Prototypage rapide | **CMake + Ninja** (minimal) | Un `CMakeLists.txt` de 5 lignes suffit |
| Contribution à un projet existant | L'outil du projet | Ne changez pas le build system d'un projet établi |
| Cours / formation | **Make** puis **CMake + Ninja** | Make pour comprendre les fondamentaux, CMake pour la pratique |

---

## Le choix de cette formation

Cette formation a fait un choix clair : **CMake + Ninja** comme stack principal, avec une couverture de Make (pour les fondamentaux et la culture technique) et de Meson (pour la vision alternative et l'écosystème Linux).

Ce choix repose sur trois constats pragmatiques.

Premièrement, **CMake est le standard de facto** de l'écosystème C++ en 2026. La quasi-totalité des bibliothèques, des gestionnaires de paquets et des IDE le supportent nativement. Maîtriser CMake ouvre le plus de portes professionnelles.

Deuxièmement, **Ninja est objectivement supérieur** à Make comme build tool. Il n'y a aucune raison technique de choisir Make comme backend de CMake en 2026.

Troisièmement, **Meson est une alternative légitime** dans le contexte Linux, mais son écosystème de dépendances plus restreint et son adoption industrielle plus limitée en font un choix de niche — certes pertinent et de qualité, mais de niche.

Un développeur C++ complet en 2026 maîtrise CMake, utilise Ninja, sait lire un Makefile, et connaît Meson. C'est ce que ce module vous a apporté.

---

## Récapitulatif en un tableau

| | Make (artisanal) | Make (via CMake) | Ninja (via CMake) | Meson + Ninja |
|---|:---:|:---:|:---:|:---:|
| **Performance build** | ⚠️ | ⚠️ | ✅ | ✅ |
| **No-op build** | ⚠️ | ⚠️ | ✅ | ✅ |
| **Détection changement cmd** | ❌ | ❌ | ✅ | ✅ |
| **Parallélisation auto** | ❌ | ❌ | ✅ | ✅ |
| **compile_commands.json** | ❌ | ⚠️ Manuel | ⚠️ Manuel | ✅ Auto |
| **Écosystème dépendances** | ❌ | ✅ | ✅ | ⚠️ |
| **Support IDE** | ❌ | ✅ | ✅ | ⚠️ |
| **Lisibilité fichiers build** | Moyenne | N/A (généré) | N/A (généré) | ✅ |
| **Portabilité** | ⚠️ Unix | ✅ | ✅ | ✅ |
| **Adapté aux petits projets** | ✅ | ⚠️ | ⚠️ | ✅ |
| **Adapté aux grands projets** | ❌ | ✅ | ✅ | ✅ |
| **Recommandation 2026** | Niche | ❌ Préférer Ninja | ✅ **Recommandé** | ✅ Si contexte Linux |

---

> **Fin du chapitre 28.** Vous avez désormais une vision complète du pipeline de build C++ — des Makefiles fondamentaux (28.1, 28.2) au fonctionnement interne de Ninja (28.3) en passant par l'alternative Meson (28.4). Combiné avec les chapitres 26 (CMake) et 27 (gestion des dépendances), vous maîtrisez l'ensemble du Module 9 : Build Systems et Gestion de Projet. Le Module 10 aborde le débogage, le profiling et la qualité du code.

⏭️ [Module 10 : Débogage, Profiling et Qualité Code](/module-10-debogage-profiling.md)
