🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 40.1 — Logging structuré : spdlog (recommandé) vs std::print

## Pourquoi `std::print` ne remplace pas un framework de logging

---

## Introduction

Le logging est la forme d'observabilité la plus fondamentale. Avant les métriques Prometheus, avant les traces OpenTelemetry, avant les dashboards Grafana, il y a un message quelque part qui dit ce qui s'est passé. Chaque développeur C++ a commencé par écrire `std::cout << "valeur = " << x << std::endl;` pour comprendre le comportement de son programme. Et chaque développeur qui a maintenu une application en production sait que cette approche ne tient pas.

Les exigences du logging en production sont radicalement différentes de celles du débogage en développement. Les messages doivent être horodatés, nivelés par sévérité, routés vers des destinations multiples, formatés de manière parsable par des outils d'agrégation, et émis sans impact mesurable sur les performances de l'application. Aucune de ces exigences n'est satisfaite par `std::cout`, et `std::print` (C++23) — malgré ses améliorations considérables sur le formatage — n'y répond pas non plus.

C'est le rôle d'un **framework de logging**. Et dans l'écosystème C++ en 2026, un framework domine très largement : **spdlog**.

---

## Ce que fait un framework de logging

Avant de comparer les outils, clarifions ce qu'on attend d'un système de logging en production. Les responsabilités se décomposent en cinq axes.

### Niveaux de sévérité

Tous les messages n'ont pas la même importance. Un framework de logging distingue au minimum les niveaux suivants, du moins au plus critique :

- **trace** — Détails ultra-fins pour le débogage intensif. Désactivé en production.  
- **debug** — Informations de débogage utiles pendant le développement et le diagnostic.  
- **info** — Événements normaux de fonctionnement : démarrage, requêtes traitées, configuration chargée.  
- **warn** — Situations anormales mais non bloquantes : timeout réessayé avec succès, fichier de configuration absent remplacé par les valeurs par défaut.  
- **error** — Erreurs qui affectent une opération mais pas le service entier : requête échouée, connexion à une dépendance perdue.  
- **critical** — Erreurs fatales qui compromettent le fonctionnement du service et nécessitent une intervention immédiate.

Le niveau de logging est configurable à l'exécution (ou au démarrage via un fichier de configuration). En production, on tourne typiquement en `info` ou `warn`. Quand un problème survient, on passe temporairement en `debug` ou `trace` pour collecter plus de données — sans recompiler, sans redéployer.

Ni `std::cout` ni `std::print` n'ont de notion de niveau. Chaque message est émis inconditionnellement, et filtrer revient à commenter des lignes de code.

### Destinations multiples (sinks)

Les logs doivent aller quelque part — et ce "quelque part" varie selon le contexte :

- **`stdout`/`stderr`** — La convention Docker et Kubernetes. Le runtime capture le flux et le route vers le driver de logging configuré.  
- **Fichiers rotatifs** — Pour les serveurs bare metal : écriture dans un fichier qui tourne automatiquement quand il atteint une taille ou un âge donné, avec suppression des plus anciens.  
- **Syslog** — Le système de logging Unix traditionnel, encore utilisé dans les infrastructures qui centralisent les logs via `rsyslog` ou `syslog-ng`.  
- **Destinations réseau** — Envoi direct vers un collecteur distant (TCP, UDP) pour les architectures qui ne passent pas par des fichiers intermédiaires.

Un framework de logging permet de configurer plusieurs destinations simultanément (par exemple `stdout` + fichier rotatif + syslog) et de filtrer par niveau pour chaque destination (par exemple `debug` vers le fichier, `error` vers syslog).

### Formatage contrôlé

Le format d'un message de log détermine ce qu'on peut en faire. Un log comme :

```
la requête a pris trop de temps
```

est lisible par un humain mais inutilisable par un outil d'agrégation. Un log comme :

```
[2026-03-18 14:23:07.412] [syswatch] [warn] Request timeout: endpoint=/api/metrics duration_ms=5032 client=10.0.1.42
```

contient un horodatage précis à la milliseconde, le nom du logger (pour distinguer les composants), le niveau de sévérité, et des paires clé-valeur structurées. Il est lisible par un humain *et* parsable par Elasticsearch, Loki, ou Datadog.

Un framework de logging offre un système de patterns configurable pour contrôler ce format, et support optionnel du JSON pour le logging entièrement structuré (couvert en section 40.5).

### Performance

Le logging en C++ a une contrainte que la plupart des autres langages n'ont pas : il ne doit pas ralentir le chemin critique. Une application qui traite 100 000 requêtes par seconde ne peut pas se permettre une allocation mémoire et un appel système `write()` synchrone pour chaque message de log.

Les frameworks de logging performants utilisent deux techniques. Le **logging asynchrone** découple l'émission du message (rapide : écriture dans un buffer en mémoire) de sa persistance (lente : écriture sur disque ou envoi réseau). Un thread dédié vide le buffer en arrière-plan. Le **formatage différé** ne construit la chaîne de caractères du message que si le niveau est activé — si le logger est configuré en `info`, un appel `log.debug(...)` ne formate rien et retourne immédiatement.

### Contexte et enrichissement

En production, un message de log isolé a une utilité limitée. Ce qui le rend précieux, c'est son contexte : quel thread l'a émis, quelle requête était en cours de traitement, quel utilisateur était concerné, quel est le trace ID pour corréler avec les traces distribuées.

Un framework de logging permet d'enrichir chaque message avec des métadonnées contextuelles sans les passer manuellement à chaque appel. Cette capacité est la base du logging structuré couvert en section 40.5.

---

## std::print (C++23) : ce que c'est et ce que ce n'est pas

`std::print`, introduit en C++23 et couvert en détail aux sections 2.7 et 12.7 de cette formation, est une amélioration majeure de l'affichage en C++. Il combine la sécurité de type de `std::cout` avec la lisibilité de syntaxe de `printf`, via la librairie de formatage `std::format` :

```cpp
std::print("Request processed: endpoint={} duration={}ms status={}\n",
           endpoint, duration_ms, status_code);
```

C'est un progrès considérable pour l'affichage général. Mais `std::print` est une **fonction d'affichage**, pas un système de logging. Voici ce qu'il ne fournit pas :

| Fonctionnalité | std::print | spdlog |
|---|---|---|
| Niveaux de sévérité | Non | trace, debug, info, warn, error, critical |
| Filtrage par niveau | Non | Oui, configurable à l'exécution |
| Horodatage automatique | Non | Oui, configurable (précision, timezone) |
| Destinations multiples | Non (stdout/stderr uniquement) | Fichiers, rotation, syslog, réseau, custom |
| Logging asynchrone | Non | Oui, thread-pool dédié |
| Rotation des fichiers | Non | Par taille et/ou par durée |
| Formatage différé | Non (formate toujours) | Oui (skip si niveau désactivé) |
| Nom du logger | Non | Oui (loggers multiples par composant) |
| Thread ID automatique | Non | Oui, dans le pattern |
| JSON structuré | Non | Oui (custom formatter ou sinks) |

`std::print` est l'outil idéal pour l'affichage utilisateur d'un outil CLI : messages de progression, résultats formatés, aide en ligne. Pour tout ce qui est instrumentation et observabilité d'un service en production, `spdlog` est l'outil approprié.

Les deux ne sont pas en concurrence — ils coexistent dans le même projet :

```cpp
// Affichage utilisateur (CLI) → std::print
std::print("Scan complete: {} files processed, {} issues found\n",
           file_count, issue_count);

// Logging opérationnel (observabilité) → spdlog
spdlog::info("Scan completed: files={} issues={} duration_ms={}",
             file_count, issue_count, elapsed.count());
```

---

## Pourquoi spdlog ?

L'écosystème C++ offre plusieurs librairies de logging : Boost.Log, glog (Google), log4cxx (Apache), easylogging++, et d'autres. En 2026, `spdlog` s'est imposé comme le choix par défaut pour plusieurs raisons convergentes.

### Performance

spdlog est conçu dès l'origine pour la performance. En mode synchrone, il produit typiquement plusieurs millions de messages par seconde sur du matériel standard. En mode asynchrone, le chemin critique (l'appel de logging dans le code applicatif) se résume à une écriture dans un ring buffer lock-free — quelques dizaines de nanosecondes. Les benchmarks publiés par les auteurs et confirmés par des tests indépendants placent spdlog parmi les loggers C++ les plus rapides, au coude à coude avec des implémentations spécialisées.

Cette performance vient de choix techniques précis : utilisation de la librairie `fmt` (la base de `std::format`) pour un formatage rapide sans allocations superflues, buffers pré-alloués, et flush contrôlé par politique plutôt qu'à chaque message.

### API moderne et ergonomique

L'API de spdlog est intuitive pour tout développeur familier avec `std::format` ou `fmt::format` — la syntaxe est identique :

```cpp
spdlog::info("Connection established: host={} port={}", host, port);  
spdlog::warn("Retry attempt {}/{} for endpoint {}", attempt, max_retries, endpoint);  
spdlog::error("Failed to open config file: {}", ec.message());  
```

Pas de macros obligatoires (contrairement à glog), pas de configuration XML (contrairement à log4cxx), pas de dépendance à Boost (contrairement à Boost.Log). La librairie est utilisable en une seule ligne d'include, avec un comportement par défaut sensé (log vers `stdout`, niveau `info`, format lisible).

### Header-only ou précompilé

spdlog peut être utilisé en mode header-only (un seul `#include <spdlog/spdlog.h>`, rien à linker) pour les petits projets, ou compilé en librairie statique pour les projets plus importants où le temps de compilation compte. Ce choix se fait via une option CMake (`SPDLOG_COMPILED_LIB`), sans changer le code applicatif.

### Écosystème et adoption

spdlog est le logger C++ le plus utilisé sur GitHub, avec une communauté active et une maintenance régulière. Il est intégré dans Conan, vcpkg, et la plupart des gestionnaires de paquets Linux. Les exemples de code, les réponses Stack Overflow, et les tutoriels sont abondants. Pour un projet professionnel, cette adoption massive réduit le risque de choisir une librairie abandonnée ou sous-documentée.

### Basé sur fmt (et compatible std::format)

spdlog utilise la librairie `fmt` comme moteur de formatage — la même librairie qui a servi de base à `std::format` dans C++20/23. Cela signifie que la syntaxe de formatage est identique entre vos logs spdlog et vos appels `std::print` ou `std::format`. Pas de syntaxe à apprendre en plus, pas de divergence à gérer.

Sur les compilateurs récents (GCC 15+, Clang 20+), spdlog peut être configuré pour utiliser `std::format` directement plutôt que `fmt`, éliminant une dépendance tierce. En pratique, la plupart des projets continuent d'utiliser `fmt` bundlée car elle offre quelques fonctionnalités supplémentaires et une compatibilité avec les compilateurs plus anciens.

---

## Alternatives et quand les considérer

Bien que spdlog soit le choix recommandé par cette formation, d'autres librairies existent et peuvent être pertinentes dans des contextes spécifiques.

**Boost.Log** est la solution de logging de l'écosystème Boost. Elle est extrêmement configurable et puissante, mais sa complexité de configuration (sinks, formatters, filters, attributes — chacun avec sa propre couche d'abstraction) et sa dépendance à Boost en font un choix lourd pour la plupart des projets. Si votre projet dépend déjà fortement de Boost, Boost.Log s'intègre naturellement. Sinon, spdlog offre 90% des fonctionnalités avec une fraction de la complexité.

**glog** (Google Logging) est utilisé en interne chez Google et dans certains projets open source de l'écosystème Google (gRPC, TensorFlow). Son API repose sur des macros (`LOG(INFO) << "message"`) et des opérateurs de flux — un style que spdlog a délibérément abandonné au profit du formatage positionnel `fmt`. glog est un choix raisonnable si vous travaillez dans un écosystème Google, mais il offre moins de flexibilité sur les sinks et le formatage.

**Quill** est un logger C++ plus récent qui se positionne comme alternative à spdlog avec un accent sur la latence ultra-faible. Son architecture repose sur un single-producer/single-consumer queue qui minimise la contention en multi-thread. Si votre application a des contraintes de latence extrêmes (trading haute fréquence, systèmes temps réel), Quill mérite un benchmark comparatif avec spdlog asynchrone.

Pour la grande majorité des projets C++ couverts par cette formation — outils CLI, services réseau, daemons système, microservices — spdlog est le choix qui offre le meilleur équilibre entre performance, ergonomie, et écosystème.

---

## Ce que couvrent les sous-sections

Les trois sous-sections suivantes détaillent l'utilisation de spdlog dans un projet C++ professionnel.

**Section 40.1.1 — Installation et configuration de spdlog** couvre l'intégration dans un projet CMake (via Conan, vcpkg, ou FetchContent), la configuration de base, et le choix entre mode header-only et précompilé.

**Section 40.1.2 — Niveaux de log et sinks** détaille les six niveaux de sévérité, les sinks disponibles (console, fichier, rotatif, syslog, callback), et la configuration de loggers multiples pour des composants distincts.

**Section 40.1.3 — Pattern de formatage** montre comment personnaliser le format des messages (horodatage, niveau, thread ID, nom du logger, source location), avec des exemples de patterns adaptés au débogage local, à la production, et au logging JSON.

⏭️ [Installation et configuration spdlog](/40-observabilite/01.1-installation-spdlog.md)
