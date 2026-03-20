🔝 Retour au [Sommaire](/SOMMAIRE.md)

# Partie V — DevOps et Cloud Native

Cette partie ancre C++ dans les pratiques DevOps modernes. Le code produit dans les parties précédentes doit maintenant être packagé, conteneurisé, compilé en CI, distribué, et observé en production. CLI11 pour les outils en ligne de commande, Docker multi-stage avec images distroless, pipelines GitLab CI / GitHub Actions avec ccache/sccache, cross-compilation ARM/RISC-V, packaging DEB/RPM, logging structuré avec spdlog, métriques Prometheus, tracing OpenTelemetry — le C++ n'est plus un langage qui s'arrête à la compilation locale.

---

## Ce que vous allez maîtriser

- Vous serez capable de concevoir un outil CLI professionnel avec CLI11 (sous-commandes, validation, aide automatique), en suivant les patterns architecturaux de `kubectl` ou `git`.
- Vous serez capable d'utiliser `fmt` pour le formatage avancé avec couleurs et styles dans un contexte terminal, et de gérer correctement la détection TTY.
- Vous serez capable de conteneuriser une application C++ avec des Dockerfiles multi-stage (stage de compilation séparé du stage d'exécution) et de produire des images distroless minimales.
- Vous serez capable de gérer les dépendances de librairies partagées (`.so`) dans un conteneur Docker et de choisir entre linkage statique et dynamique selon le contexte de déploiement.
- Vous serez capable de mettre en place un pipeline CI/CD complet pour C++ sur GitLab CI ou GitHub Actions, avec build, tests, analyse statique et packaging automatisés.
- Vous serez capable d'accélérer les builds CI avec ccache (local) et sccache (distribué/cloud), et de configurer des matrix builds multi-compilateur/multi-version.
- Vous serez capable de cross-compiler pour ARM et RISC-V depuis x86_64 dans un pipeline CI.
- Vous serez capable de créer des paquets DEB et RPM, et de distribuer des binaires via AppImage, apt ou snap.
- Vous serez capable d'instrumenter une application C++ avec spdlog (logging structuré JSON), un client Prometheus (métriques), et OpenTelemetry (tracing distribué).
- Vous serez capable d'exposer des health checks et readiness probes pour un déploiement dans un environnement orchestré (Kubernetes).

---

## Prérequis

- **Partie III — Programmation Système Linux** : networking (chapitre 22) et sérialisation JSON/YAML (chapitre 24) — nécessaires pour les health checks, la configuration d'outils CLI, et le logging structuré.
- **Partie IV — Tooling et Build Systems** : CMake et Conan (chapitres 26-27), tests Google Test (chapitre 33), analyse statique (chapitre 32) — le pipeline CI/CD automatise exactement ces outils.

---

## Modules de cette partie

| # | Titre | Niveau | Chapitres | Lien |
|---|-------|--------|-----------|------|
| Module 12 | Création d'Outils CLI | Avancé | 36 | [module-12-outils-cli.md](/module-12-outils-cli.md) |
| Module 13 | C++ dans une Approche DevOps | Avancé | 37, 38, 39, 40 | [module-13-devops.md](/module-13-devops.md) |

---

## Fil conducteur

Le Module 12 commence par la création d'outils CLI — le livrable le plus courant en contexte DevOps/SRE. CLI11 fournit le parsing d'arguments, `fmt` le formatage de sortie, et le chapitre se conclut sur l'architecture d'un outil CLI de qualité production. Le Module 13 prend ce livrable et le pousse à travers un cycle DevOps complet : conteneurisation Docker avec multi-stage builds et images distroless (chapitre 37), automatisation CI/CD sur GitLab CI et GitHub Actions avec accélération ccache/sccache et matrix builds (chapitre 38), packaging et distribution DEB/RPM/AppImage (chapitre 39), et enfin observabilité en production — logging structuré JSON via spdlog, métriques Prometheus, tracing OpenTelemetry, health checks (chapitre 40). La progression suit le cycle de vie réel d'un binaire C++ : code → build → test → package → deploy → observe. À la sortie de cette partie, vous savez livrer et opérer du C++ dans un environnement cloud native.

---


⏭️ [Module 12 : Création d'Outils CLI](/module-12-outils-cli.md)
