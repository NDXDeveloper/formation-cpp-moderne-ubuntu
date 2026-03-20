🔝 Retour au [Sommaire](/SOMMAIRE.md)

# Module 13 — C++ dans une Approche DevOps

> 🎯 Niveau : Avancé

Ce module pousse un binaire C++ à travers le cycle DevOps complet : conteneurisation Docker, pipeline CI/CD, packaging, distribution, et observabilité en production. C'est le module le plus transversal de la formation — il intègre les outils des modules précédents (CMake, Conan, tests, sanitizers, clang-tidy) dans une chaîne automatisée de bout en bout. Un développeur C++ qui ne sait pas conteneuriser, tester en CI, packager et observer son code en production est un développeur qui ne peut pas livrer de manière autonome.

---

## Objectifs pédagogiques

1. **Conteneuriser** une application C++ avec des Dockerfiles multi-stage (stage de compilation séparé du stage d'exécution) et produire des images distroless minimales.
2. **Mettre en place** un pipeline CI/CD complet pour C++ sur GitLab CI ou GitHub Actions : build, tests, analyse statique, packaging, avec accélération via ccache/sccache.
3. **Configurer** des matrix builds multi-compilateur (GCC/Clang), multi-version (C++20/C++23/C++26) et multi-architecture (x86_64, ARM, RISC-V).
4. **Créer** des paquets DEB et RPM, et distribuer des binaires via AppImage, apt ou snap.
5. **Instrumenter** une application C++ avec spdlog (logging structuré), un client Prometheus (métriques), et OpenTelemetry (tracing distribué).
6. **Exposer** des health checks et readiness probes pour le déploiement dans un environnement orchestré (Kubernetes).

---

## Prérequis

- **Module 9** : CMake, Conan, Ninja — le pipeline CI compile le projet avec ces outils. Les CMake Presets (section 27.6) sont utilisés pour standardiser les configurations entre développement local et CI.
- **Module 10, chapitre 32** : clang-tidy et clang-format — intégrés dans le pipeline CI comme étapes d'analyse statique automatisée.
- **Module 11, chapitre 33** : Google Test — les tests unitaires s'exécutent dans le pipeline CI ; la couverture de code (chapitre 34) peut être publiée comme artefact.
- **Module 1, chapitre 2, section 2.3** : ccache — l'accélération de compilation est critique en CI où chaque minute de build a un coût.
- **Module 7, chapitre 22** : networking — nécessaire pour les health checks HTTP et l'intégration Prometheus/OpenTelemetry.

---

## Chapitres

### Chapitre 37 — Dockerisation d'Applications C++

Conteneurisation d'un binaire C++ depuis l'image de base jusqu'à l'image de production. Le point central est le multi-stage build qui sépare l'environnement de compilation (GCC, CMake, Conan, headers) de l'environnement d'exécution (binaire + librairies dynamiques uniquement).

- **Images de base** : Ubuntu (familier, glibc standard, paquets nombreux) vs Alpine (musl libc, image petite mais incompatibilités possibles avec certaines librairies C++).
- **Multi-stage builds** : stage de compilation (image lourde avec toolchain complète), stage d'exécution (image minimale avec le binaire et ses dépendances runtime uniquement). Réduction de la taille d'image de ~1 Go à ~50-100 Mo.
- **Librairies partagées** : identification des `.so` nécessaires avec `ldd`, copie explicite dans l'image finale, ou linkage statique complet pour éliminer les dépendances.
- **Best practices** : utilisateur non-root, `COPY --from=builder`, `.dockerignore`, reproductibilité (version pinning des paquets), scan de vulnérabilités.
- **Distroless images** : images Google sans shell ni gestionnaire de paquets — surface d'attaque minimale, binaire statique ou avec les seules librairies nécessaires.

### Chapitre 38 — CI/CD pour C++

Automatisation du cycle build → test → package sur les deux plateformes CI dominantes. La compilation C++ en CI a des défis spécifiques : temps de build longs, dépendances complexes, et nécessité de tester sur plusieurs compilateurs et architectures.

- **GitLab CI** : structure `.gitlab-ci.yml`, stages (`build`, `test`, `analyze`, `package`, `deploy`), jobs, images Docker comme runners, cache et artefacts.
- **GitHub Actions** : structure workflow YAML, actions réutilisables pour C++ (`actions/cache` pour ccache, `lukka/run-cmake` pour CMake), matrix strategy.
- **Accélération CI** : ccache (cache de compilation local au runner, persisté via le cache CI) et sccache (cache distribué, partagé entre runners, stocké dans S3/GCS). Différence : ccache est local et simple, sccache est distribué et plus complexe à configurer mais amortit le cache entre les jobs et les branches.
- **Automatisation** : exécution des tests (CTest), analyse statique (clang-tidy), couverture de code (lcov), publication des rapports.
- **Artefacts et releases** : publication des binaires compilés, génération de changelogs, tagging.
- **Cross-compilation** : toolchains ARM et RISC-V depuis x86_64 dans le pipeline CI, images Docker avec les toolchains croisées.
- **Matrix builds** : compilation avec GCC et Clang, plusieurs versions du standard (C++20, C++23, C++26), plusieurs OS — détection des incompatibilités entre compilateurs avant le merge.

### Chapitre 39 — Packaging et Distribution

Création de paquets installables sur les distributions Linux. DEB pour Debian/Ubuntu, RPM pour RedHat/CentOS, AppImage pour la distribution universelle.

- **Paquets DEB** : structure (`DEBIAN/control`, `DEBIAN/postinst`, `DEBIAN/prerm`), scripts de contrôle (installation, suppression, upgrade), construction avec `dpkg-deb`, déclaration des dépendances runtime.
- **Paquets RPM** : fichier `.spec`, construction avec `rpmbuild`, différences avec DEB.
- **AppImage** : distribution universelle sans installation — un seul fichier exécutable contenant le binaire et toutes ses dépendances. Adapté aux outils CLI et aux applications desktop.
- **Distribution** : publication via dépôt APT privé, snap store, ou téléchargement direct des artefacts CI.

### Chapitre 40 — Observabilité et Monitoring

Les trois piliers de l'observabilité appliqués à une application C++ : logs, métriques, traces. Ce chapitre couvre les librairies et les patterns pour rendre un binaire C++ observable en production.

- **spdlog** : librairie de logging recommandée. Installation, configuration, niveaux de log (`trace`, `debug`, `info`, `warn`, `error`, `critical`), sinks (console, fichier, rotating file, syslog), pattern de formatage personnalisé. Comparaison avec `std::print` (pas un logger — pas de niveaux, pas de sinks, pas de rotation).
- **Métriques Prometheus** : exposition de compteurs, gauges, histogrammes via le client C++ Prometheus (`prometheus-cpp`). Endpoint HTTP `/metrics` pour le scraping.
- **OpenTelemetry C++** : tracing distribué — spans, contexte de propagation, exporters (Jaeger, Zipkin, OTLP). Corrélation des traces entre services.
- **Health checks et readiness probes** : endpoints HTTP `/healthz` et `/readyz` pour Kubernetes — vérification de la connectivité aux dépendances (base de données, services amont).
- **Structured logging** : logs en format JSON pour agrégation dans ELK, Loki, ou Datadog — champs structurés (`timestamp`, `level`, `service`, `trace_id`) au lieu de texte libre.

---

## Points de vigilance

- **Image Docker avec les outils de build en production.** Un Dockerfile sans multi-stage qui installe GCC, CMake, Conan et les headers de développement dans l'image finale produit une image de plus d'1 Go avec une surface d'attaque massive. En production, seul le binaire et ses librairies dynamiques doivent être présents. Utilisez systématiquement un multi-stage build (`COPY --from=builder /app/build/mybin /usr/local/bin/`) ou une image distroless avec un binaire linkée statiquement.

- **ccache en CI qui ne persiste pas entre les jobs.** Sans configuration du cache CI (GitLab : `cache:`, GitHub Actions : `actions/cache`), ccache est recréé à vide à chaque job — les compilations ne sont jamais accélérées. Configurez `CCACHE_DIR` vers un chemin persisté par le mécanisme de cache de votre plateforme CI, et vérifiez le hit rate avec `ccache -s` dans les logs du pipeline. Pour sccache, configurez le backend de stockage (S3, GCS, Redis) et partagez le cache entre branches.

- **Paquet DEB sans gestion des conflits de fichiers.** Un paquet DEB qui installe des fichiers dans `/usr/local/bin/` ou `/etc/` sans déclarer les conflits (`Conflicts:`) et les remplacements (`Replaces:`) dans `DEBIAN/control` peut écraser des fichiers d'autres paquets silencieusement, ou échouer à l'installation si un fichier existe déjà. Déclarez les dépendances (`Depends:`), les conflits, et utilisez des chemins d'installation appropriés. Testez l'installation, la mise à jour et la suppression du paquet dans un conteneur Docker propre avant de publier.

- **Logging sans niveau de log configurable à l'exécution.** Un binaire C++ compilé avec un niveau de log fixe (hardcodé à `info`) ne peut pas être passé en `debug` en production sans recompilation et redéploiement. spdlog permet de changer le niveau de log à l'exécution via `spdlog::set_level()`. Exposez cette configuration via une variable d'environnement (`LOG_LEVEL=debug`), un fichier de configuration, ou un signal Unix (`SIGUSR1` pour augmenter la verbosité). Le surcoût des logs `debug` désactivés est quasi nul avec spdlog (vérification de niveau avant formatage).

---

## Compétences acquises

À l'issue de ce module, vous savez :
- Conteneuriser une application C++ avec un Dockerfile multi-stage et produire des images distroless de production.
- Mettre en place un pipeline CI/CD complet (GitLab CI ou GitHub Actions) avec build, tests, analyse statique, couverture, et packaging.
- Accélérer les builds CI avec ccache (local) et sccache (distribué), et configurer des matrix builds multi-compilateur/multi-architecture.
- Créer et distribuer des paquets DEB, RPM et AppImage.
- Instrumenter une application C++ avec spdlog (logs structurés JSON), Prometheus (métriques), et OpenTelemetry (tracing distribué).
- Exposer des health checks pour un déploiement Kubernetes.

---


⏭️ [Dockerisation d'Applications C++](/37-dockerisation/README.md)
