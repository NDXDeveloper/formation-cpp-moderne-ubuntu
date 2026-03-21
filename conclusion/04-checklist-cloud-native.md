🔝 Retour au [Sommaire](/SOMMAIRE.md)

# Checklist du développeur C++ Cloud Native ⭐

## Document de référence opérationnel

---

Cette checklist rassemble, point par point, tout ce qu'un projet C++ moderne déployé en environnement cloud doit intégrer. Elle synthétise les bonnes pratiques couvertes tout au long de la formation en un document unique, utilisable comme point de départ pour chaque nouveau projet ou comme grille d'audit pour un projet existant.

Chaque item renvoie au module ou à la section de la formation qui le couvre en détail. Les items marqués **[CRITIQUE]** sont des prérequis non négociables pour un projet de production. Les items marqués **[RECOMMANDÉ]** apportent une valeur significative mais peuvent être différés selon le contexte. Les items marqués **[AVANCÉ]** s'adressent aux projets matures ou à forte exigence de performance/sécurité.

---

## 1. Structure du projet

**[CRITIQUE]** Organisation des répertoires respectant les conventions standard :

```
project-root/
├── CMakeLists.txt              # Build principal
├── CMakePresets.json            # Presets de configuration
├── conanfile.py                 # Dépendances (ou vcpkg.json)
├── Dockerfile                   # Build et packaging conteneur
├── .clang-format                # Style de formatage
├── .clang-tidy                  # Règles d'analyse statique
├── .pre-commit-config.yaml      # Hooks de pre-commit
├── .github/workflows/           # Pipelines CI/CD (ou .gitlab-ci.yml)
├── README.md                    # Documentation d'entrée
├── LICENSE
├── src/                         # Code source (.cpp)
│   ├── main.cpp
│   └── CMakeLists.txt
├── include/                     # Headers publics (.h / .hpp)
│   └── project_name/
├── lib/                         # Bibliothèques internes
│   ├── module_a/
│   │   ├── include/
│   │   ├── src/
│   │   └── CMakeLists.txt
│   └── module_b/
├── tests/                       # Tests unitaires et d'intégration
│   ├── unit/
│   ├── integration/
│   └── CMakeLists.txt
├── benchmarks/                  # Micro-benchmarks
│   └── CMakeLists.txt
├── docs/                        # Documentation (Doxygen, guides)
│   └── Doxyfile
└── scripts/                     # Scripts utilitaires (build, deploy)
```

*(Section 46.1)*

**[CRITIQUE]** Séparation stricte `.h`/`.cpp` pour la compilation incrémentale. Les headers publics exposent l'API ; les détails d'implémentation restent dans les `.cpp`. *(Section 46.2)*

**[CRITIQUE]** Utilisation de namespaces pour éviter la pollution de l'espace global. Le namespace racine correspond au nom du projet ; les sous-modules utilisent des namespaces imbriqués. *(Section 46.3)*

**[RECOMMANDÉ]** Documentation Doxygen configurée et générée automatiquement. Au minimum : les classes publiques, les fonctions d'API, et les types exposés. *(Section 46.4)*

**[RECOMMANDÉ]** Un standard de codage adopté et documenté (Google C++ Style Guide, C++ Core Guidelines, ou convention interne). Le standard est enforced automatiquement par clang-format et clang-tidy, pas par des reviews manuelles. *(Section 46.5)*

---

## 2. Standard C++ et langage

**[CRITIQUE]** Standard C++ minimum : **C++20**. C++23 si le support compilateur est suffisant pour les fonctionnalités utilisées. Le standard est spécifié explicitement dans le CMakeLists.txt :

```cmake
target_compile_features(my_target PUBLIC cxx_std_20)
# ou
set(CMAKE_CXX_STANDARD 20)  
set(CMAKE_CXX_STANDARD_REQUIRED ON)  
set(CMAKE_CXX_EXTENSIONS OFF)  
```

*(Section 2.6.4)*

**[CRITIQUE]** Aucun `new`/`delete` nu dans le code applicatif. Gestion mémoire exclusivement via smart pointers (`std::unique_ptr`, `std::shared_ptr`) et conteneurs RAII. *(Section 9.4)*

**[CRITIQUE]** Sémantique de mouvement correctement implémentée pour les types custom qui gèrent des ressources. Move constructors et move assignment operators sont `noexcept`. *(Sections 10.3, 17.4)*

**[RECOMMANDÉ]** Utilisation des fonctionnalités C++ moderne là où elles apportent de la clarté et de la sécurité :  
- `std::optional` pour les valeurs potentiellement absentes *(Section 12.2)*  
- `std::expected` (C++23) pour la gestion d'erreurs sur les chemins non-exceptionnels *(Section 12.8)*  
- `std::span` pour les paramètres pointant vers des données contiguës *(Section 13.5)*  
- `std::string_view` pour les paramètres en lecture seule sur des chaînes  
- Structured bindings pour les retours multiples *(Section 12.1)*  
- Concepts pour contraindre les interfaces templatées *(Section 16.6)*  
- Ranges pour les pipelines de transformation de données *(Section 15.6)*

**[RECOMMANDÉ]** `std::print` / `std::format` (C++23) utilisé en remplacement de `std::cout` et `printf` pour le code applicatif. Le logging de production passe par une bibliothèque dédiée (spdlog), pas par `std::print`. *(Sections 2.7, 12.7, 40.1)*

**[AVANCÉ]** Contrats C++26 (préconditions, postconditions) activés sur les interfaces publiques quand le support compilateur le permet. *(Section 12.14.1)*

---

## 3. Build system

**[CRITIQUE]** CMake comme build system, version minimum **3.25+** (idéalement 3.31+). Approche moderne basée sur les targets :  
- `add_executable` / `add_library` pour définir les cibles  
- `target_link_libraries` pour les dépendances  
- `target_include_directories` pour les chemins d'inclusion  
- Visibilité `PUBLIC` / `PRIVATE` / `INTERFACE` correctement spécifiée sur chaque target

*(Sections 26.1–26.4)*

**[CRITIQUE]** Génération pour **Ninja** par défaut :

```bash
cmake -G Ninja -B build -S .  
cmake --build build  
```

*(Section 26.5)*

**[CRITIQUE]** CMake Presets (`CMakePresets.json`) pour standardiser les configurations. Au minimum trois presets :  
- `dev` : Debug, sanitizers activés, warnings maximaux  
- `release` : Release, optimisations activées, LTO  
- `ci` : Configuration utilisée par le pipeline CI/CD

*(Section 27.6)*

**[CRITIQUE]** Gestion des dépendances externes via un gestionnaire de paquets :  
- **Conan 2.0** (conanfile.py avec intégration CMake) *(Section 27.2)*  
- ou **vcpkg** (vcpkg.json en mode manifeste) *(Section 27.3)*  
- ou `FetchContent` pour les dépendances légères *(Section 26.3.2)*

Les dépendances ne sont jamais vendored manuellement sans justification documentée.

**[RECOMMANDÉ]** **ccache** configuré pour accélérer les compilations locales et en CI :

```cmake
find_program(CCACHE ccache)  
if(CCACHE)  
    set(CMAKE_C_COMPILER_LAUNCHER ${CCACHE})
    set(CMAKE_CXX_COMPILER_LAUNCHER ${CCACHE})
endif()
```

*(Section 2.3)*

**[AVANCÉ]** **sccache** (cache distribué) pour les environnements CI avec plusieurs runners. *(Section 38.3.2)*

---

## 4. Compilation et qualité du build

**[CRITIQUE]** Warnings maximaux activés et traités comme des erreurs en CI :

```cmake
target_compile_options(my_target PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:
        -Wall -Wextra -Wpedantic -Werror
        -Wconversion -Wsign-conversion -Wshadow
        -Wnon-virtual-dtor -Woverloaded-virtual
    >
)
```

*(Section 2.6.1)*

**[CRITIQUE]** Build testable avec au moins deux compilateurs (GCC et Clang). Les différences de diagnostic entre compilateurs révèlent des catégories de bugs distinctes. *(Section 2.1.2, 38.7)*

**[RECOMMANDÉ]** Optimisation `-O2` pour les builds de production. `-O3` seulement si le profiling montre un gain mesurable. `-Os` pour les déploiements contraints en taille (conteneurs, embarqué). *(Section 2.6.2)*

**[AVANCÉ]** **LTO** (Link-Time Optimization) activé pour les builds de production :

```cmake
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
```

*(Section 41.5)*

**[AVANCÉ]** **PGO** (Profile-Guided Optimization) pour les composants critiques en performance : build instrumenté → exécution avec charge représentative → rebuild optimisé. *(Section 41.4)*

---

## 5. Analyse statique et formatage

**[CRITIQUE]** **clang-format** configuré avec un fichier `.clang-format` à la racine du projet. Le style est enforced automatiquement (pre-commit hook ou CI), pas négocié par review. *(Section 32.3)*

**[CRITIQUE]** **clang-tidy** configuré avec un fichier `.clang-tidy`. Checks recommandés au minimum :

```yaml
Checks: >
  -*,
  bugprone-*,
  cppcoreguidelines-*,
  misc-*,
  modernize-*,
  performance-*,
  readability-*,
  -modernize-use-trailing-return-type,
  -readability-magic-numbers
WarningsAsErrors: '*'
```

*(Section 32.1)*

**[RECOMMANDÉ]** **cppcheck** comme second outil d'analyse statique complémentaire. Chaque outil a ses forces et détecte des catégories de problèmes différentes. *(Section 32.2)*

**[CRITIQUE]** **Pre-commit hooks** configurés via le framework pre-commit :

```yaml
# .pre-commit-config.yaml
repos:
  - repo: https://github.com/pre-commit/pre-commit-hooks
    rev: v4.6.0
    hooks:
      - id: trailing-whitespace
      - id: end-of-file-fixer
  - repo: https://github.com/pre-commit/mirrors-clang-format
    rev: v19.1.0
    hooks:
      - id: clang-format
  - repo: local
    hooks:
      - id: clang-tidy
        name: clang-tidy
        entry: scripts/run-clang-tidy.sh
        language: script
        files: \.(cpp|hpp|h)$
```

*(Sections 47.2–47.3)*

---

## 6. Tests

**[CRITIQUE]** Framework de test : **Google Test**. Intégré via CMake avec `FetchContent` ou le gestionnaire de paquets. *(Section 33.1)*

**[CRITIQUE]** Structure de tests :  
- **Tests unitaires** pour chaque module/bibliothèque (`TEST`, `TEST_F`). *(Section 33.2)*  
- **Tests d'intégration** pour les interactions entre composants et les scénarios de bout en bout.  
- Mocking avec **Google Mock** pour isoler les dépendances externes (réseau, filesystem, bases de données). *(Section 33.4)*

**[CRITIQUE]** Les tests s'exécutent dans le pipeline CI à chaque commit. Un test en échec bloque le merge.

**[RECOMMANDÉ]** **Tests paramétrés** (`TEST_P`) pour les cas avec de multiples entrées. *(Section 33.2.3)*

**[RECOMMANDÉ]** **Couverture de code** mesurée avec gcov/lcov, avec rapport HTML généré en CI. Objectif minimum : 80% de couverture de lignes sur le code applicatif (hors code généré et tests). *(Sections 34.1–34.4)*

**[RECOMMANDÉ]** **Benchmarks** avec Google Benchmark pour les composants sensibles à la performance. Les benchmarks sont exécutés en CI avec suivi de la régression de performance (comparaison avec la baseline). *(Section 35.1)*

**[AVANCÉ]** **Fuzzing** avec LibFuzzer ou AFL++ sur les parsers, les décodeurs, et toute entrée non fiable. Les corpus de fuzzing sont versionnés et réutilisés en CI. *(Section 45.5)*

---

## 7. Sanitizers

**[CRITIQUE]** Le preset `dev` active les sanitizers par défaut. Les tests s'exécutent avec les sanitizers activés en CI :

```cmake
# Preset dev / sanitizers
target_compile_options(my_target PRIVATE
    -fsanitize=address,undefined
    -fno-omit-frame-pointer
)
target_link_options(my_target PRIVATE
    -fsanitize=address,undefined
)
```

*(Sections 29.4, 5.5.2)*

**[CRITIQUE]** **AddressSanitizer** (ASan) activé sur les runs de tests CI. Détecte : heap buffer overflow, stack buffer overflow, use-after-free, use-after-return, memory leaks. *(Section 29.4.1)*

**[CRITIQUE]** **UndefinedBehaviorSanitizer** (UBSan) activé en complément d'ASan. Détecte : signed integer overflow, null pointer dereference, misaligned access, invalid shift. *(Section 29.4.2)*

**[RECOMMANDÉ]** **ThreadSanitizer** (TSan) activé sur un job CI séparé (incompatible avec ASan). Détecte les data races et les deadlocks potentiels. Indispensable pour tout code multi-threadé. *(Section 29.4.3)*

**[AVANCÉ]** **MemorySanitizer** (MSan) pour détecter les lectures de mémoire non initialisée. Nécessite que toutes les dépendances soient compilées avec MSan (contrainte forte). *(Section 29.4.4)*

**[AVANCÉ]** Sanitizers activés dans un mode léger en production (ASan avec `detect_leaks=0`, UBSan en mode `trap`) pour les services non critiques en latence. *(Section 45.6.3)*

---

## 8. Conteneurisation

**[CRITIQUE]** **Dockerfile multi-stage** :  
- Stage 1 (builder) : image avec toolchain complète (Ubuntu + GCC/Clang + CMake + Conan + Ninja)  
- Stage 2 (runtime) : image minimale avec uniquement le binaire et ses dépendances runtime

```dockerfile
# Stage 1 : Build
FROM ubuntu:24.04 AS builder  
RUN apt-get update && apt-get install -y \  
    g++-15 cmake ninja-build python3-pip
# ... installation Conan, build du projet

# Stage 2 : Runtime
FROM gcr.io/distroless/cc-debian12  
COPY --from=builder /app/build/my_binary /usr/local/bin/  
ENTRYPOINT ["/usr/local/bin/my_binary"]  
```

*(Sections 37.1–37.2)*

**[CRITIQUE]** Gestion explicite des librairies partagées. Vérifier avec `ldd` que toutes les dépendances dynamiques sont présentes dans l'image runtime. Privilégier le linkage statique pour les déploiements conteneurisés quand c'est possible. *(Sections 37.3, 27.4)*

**[RECOMMANDÉ]** **Image distroless** ou scratch pour le stage runtime. Pas de shell, pas de package manager, pas d'outils système non nécessaires. Réduit la surface d'attaque et la taille de l'image. *(Section 37.5)*

**[RECOMMANDÉ]** Image buildée avec un utilisateur non-root :

```dockerfile
USER nonroot:nonroot
```

*(Section 37.4)*

**[RECOMMANDÉ]** Taille de l'image runtime inférieure à **50 Mo** pour un binaire C++ statiquement linké dans une image distroless. Si l'image dépasse 200 Mo, investiguer les dépendances inutiles.

---

## 9. CI/CD

**[CRITIQUE]** Pipeline CI/CD automatisé (GitHub Actions ou GitLab CI) déclenché à chaque push et chaque pull/merge request. *(Sections 38.1–38.2)*

**[CRITIQUE]** Stages minimum du pipeline :

```
Build → Test (+ sanitizers) → Analyse statique → Package
```

Chaque stage bloque le suivant en cas d'échec.

**[CRITIQUE]** **Cache de compilation** (ccache ou sccache) configuré dans le pipeline pour réduire les temps de build. *(Section 38.3)*

**[RECOMMANDÉ]** **Matrix builds** testant au minimum :  
- 2 compilateurs : GCC (dernière stable) + Clang (dernière stable)  
- 2 modes : Debug (avec sanitizers) + Release

```yaml
# GitHub Actions
strategy:
  matrix:
    compiler: [gcc-15, clang-20]
    build_type: [Debug, Release]
```

*(Section 38.7)*

**[RECOMMANDÉ]** Couverture de code et rapport de benchmark intégrés au pipeline, avec notification en cas de régression. *(Sections 34.3, 35.1)*

**[RECOMMANDÉ]** **Cross-compilation** dans le pipeline si le projet cible plusieurs architectures (ARM, RISC-V). *(Section 38.6)*

**[RECOMMANDÉ]** Stage de **packaging** automatisé : construction de l'image Docker, push vers un registry, et/ou création de paquets DEB/RPM. *(Sections 37.1, 39.1)*

**[AVANCÉ]** Stage de **déploiement** automatisé (CD) : rolling update vers un cluster Kubernetes, canary deployment, ou promotion staging → production. *(Section 38.4)*

---

## 10. Observabilité

**[CRITIQUE]** **Logging structuré** avec spdlog. Logs au format JSON pour l'agrégation par un collecteur centralisé (Loki, Elasticsearch, CloudWatch Logs) :

```cpp
#include <spdlog/spdlog.h>

spdlog::info("Request processed: method={} path={} status={} duration_ms={}",
             method, path, status_code, duration.count());
```

*(Sections 40.1, 40.5)*

**[CRITIQUE]** **Niveaux de log** correctement utilisés :  
- `trace` / `debug` : détails de développement, désactivés en production par défaut  
- `info` : événements normaux (démarrage, requêtes traitées, opérations réussies)  
- `warn` : situations anormales mais récupérables (timeout, retry, dégradation)  
- `error` : erreurs nécessitant une attention (échec de requête, ressource indisponible)  
- `critical` : erreurs fatales nécessitant une intervention immédiate

*(Section 40.1.2)*

**[CRITIQUE]** **Métriques Prometheus** exposées via un endpoint HTTP `/metrics`. Métriques minimum :  
- Compteurs : requêtes totales (par type, par statut), erreurs totales  
- Histogrammes : latence des requêtes, durée des opérations critiques  
- Gauges : connexions actives, taille des queues internes, utilisation mémoire

*(Section 40.2)*

**[CRITIQUE]** **Health checks** :  
- **Liveness probe** (`/healthz`) : le processus est vivant et fonctionnel  
- **Readiness probe** (`/readyz`) : le service est prêt à recevoir du trafic (dépendances accessibles, warm-up terminé)

Ces endpoints sont nécessaires pour l'orchestration Kubernetes (`livenessProbe`, `readinessProbe`).

*(Section 40.4)*

**[RECOMMANDÉ]** **Tracing distribué** avec OpenTelemetry C++. Propagation de trace context entre services (via headers HTTP ou metadata gRPC). Indispensable dans une architecture microservices pour diagnostiquer les problèmes de latence inter-services. *(Section 40.3)*

**[RECOMMANDÉ]** Chaque log applicatif inclut un **identifiant de corrélation** (request ID, trace ID) permettant de suivre une requête à travers plusieurs composants.

**[AVANCÉ]** Dashboards Grafana préconfigurés et versionnés (infrastructure as code) avec les métriques clés du service : RED metrics (Rate, Errors, Duration) pour les services orientés requêtes, USE metrics (Utilization, Saturation, Errors) pour les ressources systèmes.

---

## 11. Networking et communication inter-services

**[CRITIQUE]** Pour les communications inter-services synchrones : **gRPC** avec Protocol Buffers. Définition des services dans des fichiers `.proto` versionnés, génération de code automatisée dans le build CMake. *(Section 22.6)*

**[CRITIQUE]** Gestion des erreurs réseau : timeouts configurés, retry avec backoff exponentiel, circuit breakers pour les appels vers des dépendances externes.

**[RECOMMANDÉ]** Pour les APIs publiques/REST : librairie HTTP (cpp-httplib ou cpr) ou gateway devant le service gRPC (grpc-gateway, Envoy). *(Section 22.5)*

**[RECOMMANDÉ]** Gestion explicite du **graceful shutdown** : interception de `SIGTERM`, arrêt de l'acceptation de nouvelles requêtes, attente de la fin des requêtes en cours (drain), puis arrêt propre. Indispensable pour les rolling updates Kubernetes sans perte de requêtes.

```cpp
#include <csignal>
#include <atomic>

std::atomic<bool> shutdown_requested{false};

void signal_handler(int) {
    shutdown_requested.store(true, std::memory_order_relaxed);
}

// Dans main() :
std::signal(SIGTERM, signal_handler);  
std::signal(SIGINT, signal_handler);  

while (!shutdown_requested.load(std::memory_order_relaxed)) {
    // Boucle principale du service
}
// Phase de drain...
```

*(Sections 20.1–20.2)*

---

## 12. Gestion des erreurs

**[CRITIQUE]** Stratégie de gestion d'erreurs définie et cohérente dans tout le projet :  
- `std::expected` (C++23) ou codes d'erreur pour les **erreurs attendues** (input invalide, ressource non trouvée, timeout) — ce sont des chemins de contrôle normaux  
- Exceptions pour les **erreurs exceptionnelles** (corruption de données, violation d'invariant, état irrécupérable) — ce sont des situations qui ne devraient pas se produire en fonctionnement normal  
- Les deux approches ne se mélangent pas arbitrairement : le choix est documenté dans les guidelines du projet

*(Sections 17.1–17.5)*

**[CRITIQUE]** Fonctions de mouvement, destructeurs et swap marqués `noexcept`. Le compilateur peut optimiser en conséquence, et les conteneurs STL l'utilisent pour choisir entre copie et déplacement. *(Section 17.4)*

**[RECOMMANDÉ]** `std::stacktrace` (C++23) capturé dans les handlers d'erreurs critiques et inclus dans les logs pour faciliter le diagnostic post-mortem. *(Section 12.12)*

**[AVANCÉ]** Contrats C++26 activés sur les interfaces publiques des bibliothèques internes. *(Section 12.14.1)*

---

## 13. Sécurité

**[CRITIQUE]** Compilation avec les **protections de sécurité** activées :

```cmake
target_compile_options(my_target PRIVATE
    -fstack-protector-strong           # Protection de la stack
    -D_FORTIFY_SOURCE=2                # Détection de buffer overflows
    -fPIE                              # Position Independent Executable
)
target_link_options(my_target PRIVATE
    -pie                               # ASLR
    -Wl,-z,relro,-z,now                # Relocation hardening
)
```

*(Sections 45.4.1–45.4.3)*

**[CRITIQUE]** Aucune donnée sensible (secrets, clés API, tokens) hardcodée dans le code source ou les images Docker. Les secrets sont injectés via des variables d'environnement ou un système de secrets management (Kubernetes Secrets, Vault).

**[CRITIQUE]** Toute entrée non fiable (requêtes réseau, fichiers, variables d'environnement) est **validée et bornée** avant traitement. Utiliser `std::span` avec des tailles explicites plutôt que des pointeurs nus pour les buffers.

**[RECOMMANDÉ]** **Fuzzing** avec LibFuzzer intégré au pipeline CI pour tout code exposé à des entrées externes (parsers, décodeurs, désérialiseurs). *(Section 45.5)*

**[RECOMMANDÉ]** Analyse régulière des dépendances pour les CVE connues. Conan et vcpkg permettent de mettre à jour les dépendances individuellement ; le tracking des vulnérabilités est de la responsabilité de l'équipe.

**[AVANCÉ]** Safety Profiles appliqués quand le support compilateur le permet, pour renforcer les garanties de sécurité mémoire à la compilation. *(Section 45.6.2)*

---

## 14. Performance

**[CRITIQUE]** Le profiling est fait **avant** l'optimisation. Aucune optimisation n'est appliquée sans mesure préalable démontrant un goulot d'étranglement. L'outil de référence est `perf` (record/report) avec visualisation par flamegraphs. *(Sections 31.1, 31.3)*

**[CRITIQUE]** Choix des conteneurs STL guidé par le profil d'accès réel, pas par habitude :  
- `std::vector` par défaut (localité mémoire, cache-friendly) *(Section 13.1)*  
- `std::flat_map` / `std::flat_set` pour les collections ordonnées de petite à moyenne taille avec lookups fréquents *(Section 14.4)*  
- `std::unordered_map` pour les lookups O(1) sur de grandes collections *(Section 14.2)*  
- `std::span` pour les vues sur données existantes sans allocation *(Section 13.5)*

**[RECOMMANDÉ]** Benchmarks Google Benchmark pour les chemins critiques, exécutés en CI avec détection de régressions de performance. *(Section 35.1)*

**[RECOMMANDÉ]** Awareness de la localité mémoire : structures de données compactes, accès séquentiel préféré, éviter les indirections de pointeurs sur les chemins chauds (préférer `std::vector<T>` à `std::vector<std::unique_ptr<T>>` quand le polymorphisme n'est pas nécessaire). *(Section 41.1)*

**[AVANCÉ]** Politiques d'exécution parallèle (`std::execution::par`, `par_unseq`) appliquées sur les algorithmes STL traitant de grands volumes de données, après validation que le parallélisme apporte un gain net. *(Section 15.7)*

**[AVANCÉ]** SIMD / auto-vectorisation vérifiée sur les boucles critiques via l'analyse de l'assembly (Compiler Explorer ou `perf annotate`). *(Section 41.3)*

---

## 15. Gestion de la configuration

**[CRITIQUE]** L'application suit le principe des **12-factor apps** : la configuration est injectée via l'environnement (variables d'environnement, fichiers montés, ConfigMaps Kubernetes), pas compilée dans le binaire.

**[CRITIQUE]** Fichiers de configuration en **YAML** ou **TOML** (pas en JSON — absence de commentaires). Parsing avec yaml-cpp ou toml++, avec validation au démarrage et messages d'erreur explicites en cas de configuration invalide. *(Sections 24.2, 24.3)*

**[RECOMMANDÉ]** Configuration rechargeable à chaud (hot reload) via signal `SIGHUP` ou endpoint d'administration, pour les paramètres qui ne nécessitent pas un redémarrage (niveaux de log, feature flags, timeouts).

**[RECOMMANDÉ]** Valeurs par défaut raisonnables pour tous les paramètres. L'application démarre avec une configuration minimale (juste le port et l'adresse de la base de données, par exemple) et utilise des défauts sensés pour le reste.

---

## 16. Concurrence et thread safety

**[CRITIQUE]** Modèle de concurrence défini et documenté dans l'architecture du projet : nombre de threads, responsabilité de chaque thread, flux de données entre threads, mécanismes de synchronisation utilisés.

**[CRITIQUE]** Synchronisation via les primitives modernes :  
- `std::jthread` (C++20) de préférence à `std::thread` (arrêt coopératif automatique) *(Section 21.7)*  
- `std::scoped_lock` pour le verrouillage de plusieurs mutex (deadlock-safe) *(Section 21.2.4)*  
- `std::atomic` pour les compteurs et flags partagés *(Section 21.4)*  
- `std::condition_variable` pour les patterns producteur/consommateur *(Section 21.3)*

**[CRITIQUE]** **ThreadSanitizer** activé en CI sur un job dédié pour détecter les data races. *(Section 29.4.3)*

**[RECOMMANDÉ]** Minimiser le partage de données entre threads. Préférer les architectures message-passing (queues thread-safe) au partage de mémoire avec mutex.

**[AVANCÉ]** Structures lock-free uniquement là où le profiling démontre que la contention sur les mutex est le goulot. Les structures lock-free sont plus difficiles à écrire correctement et plus difficiles à maintenir. *(Section 42.4)*

---

## 17. Documentation et maintenance

**[CRITIQUE]** **README.md** complet :  
- Description du projet et de son rôle  
- Prérequis (compilateur, CMake, Conan/vcpkg, Docker)  
- Instructions de build (copier-coller exécutable)  
- Instructions de test  
- Instructions de déploiement  
- Lien vers la documentation détaillée

**[CRITIQUE]** **CHANGELOG.md** maintenu selon le semantic versioning. Chaque release documente les ajouts, modifications et corrections. *(Section 47.6)*

**[RECOMMANDÉ]** Architecture Decision Records (ADR) pour les choix techniques significatifs : pourquoi gRPC plutôt que REST, pourquoi `std::expected` plutôt que des exceptions sur ce chemin, pourquoi ce conteneur plutôt qu'un autre. Les ADR sont versionnés avec le code dans un répertoire `docs/adr/`.

**[RECOMMANDÉ]** Doxygen généré automatiquement en CI et publié (GitHub Pages, GitLab Pages, ou site interne). *(Section 46.4)*

---

## Grille de maturité projet

Cette grille permet d'évaluer rapidement où en est un projet par rapport aux pratiques Cloud Native. Chaque niveau inclut les exigences du niveau précédent.

### Niveau 1 — Fonctionnel

Le projet compile, s'exécute et produit des résultats corrects.

- CMake fonctionnel avec targets propres  
- Standard C++20 minimum  
- Smart pointers, RAII, pas de `new`/`delete` nu  
- Tests unitaires Google Test avec couverture basique  
- README avec instructions de build

### Niveau 2 — Professionnel

Le projet peut être maintenu par une équipe et évoluer dans le temps.

- clang-format + clang-tidy configurés et enforcés  
- Pre-commit hooks installés  
- Sanitizers (ASan + UBSan) activés en mode dev  
- Gestion des dépendances via Conan ou vcpkg  
- CMake Presets (dev, release, ci)  
- Pipeline CI/CD basique (build + test)

### Niveau 3 — Production-ready

Le projet peut être déployé en production avec confiance.

- Dockerfile multi-stage, image distroless  
- Pipeline CI/CD complet (build → test → analyse statique → package)  
- Matrix builds (GCC + Clang, Debug + Release)  
- ccache/sccache en CI  
- Logging structuré (spdlog, JSON)  
- Métriques Prometheus (/metrics)  
- Health checks (/healthz, /readyz)  
- Graceful shutdown (SIGTERM)  
- Protections de sécurité à la compilation  
- ThreadSanitizer en CI pour le code concurrent

### Niveau 4 — Cloud Native mature

Le projet exploite pleinement l'écosystème Cloud Native.

- Tracing distribué (OpenTelemetry)  
- Benchmarks en CI avec détection de régression  
- Fuzzing intégré au pipeline  
- Cross-compilation multi-architectures  
- Configuration hot-reloadable  
- Packaging automatisé (DEB/RPM ou OCI image)  
- Déploiement continu automatisé  
- Dashboards Grafana versionnés  
- ADR documentés  
- PGO / LTO sur les builds de production

---

## Utilisation de cette checklist

**Pour un nouveau projet :** visez le Niveau 2 dès le premier commit. Le coût de mise en place est faible quand on part de zéro (créer un `.clang-format`, un `.clang-tidy`, un `CMakePresets.json` et un pipeline CI basique prend quelques heures) et le retour sur investissement est immédiat. Atteignez le Niveau 3 avant le premier déploiement en production. Le Niveau 4 se construit progressivement en fonction des besoins.

**Pour un projet existant :** utilisez la grille de maturité pour identifier votre niveau actuel, puis traitez les items manquants du niveau suivant comme une dette technique prioritaire. Commencez par les items **[CRITIQUE]** du niveau cible avant de passer aux **[RECOMMANDÉ]**.

**En entretien ou en code review :** cette checklist peut servir de grille d'évaluation pour juger de la maturité d'un projet C++ ou de la culture technique d'une équipe.

---


⏭️ Retour au [Sommaire](/SOMMAIRE.md)
