🔝 Retour au [Sommaire](/SOMMAIRE.md)

# Chapitre 40 — Observabilité et Monitoring

## Comprendre ce que fait votre application en production

---

## Pourquoi ce chapitre ?

Votre application C++ compile, passe les tests, est packagée dans un `.deb` et déployée sur des serveurs de production. Le pipeline CI/CD fonctionne, le service systemd tourne. Et puis un matin, l'application consomme 98% du CPU d'un serveur, ou elle ne répond plus aux requêtes, ou elle plante silencieusement à 3h du matin et redémarre en boucle sans que personne ne comprenne pourquoi.

Le code correct ne suffit pas. En production, ce qui compte est la **visibilité** : savoir ce que fait l'application à chaque instant, détecter les anomalies avant qu'elles ne deviennent des incidents, et disposer des données nécessaires pour diagnostiquer un problème après coup. C'est ce que le domaine appelle l'**observabilité** — la capacité d'un système à rendre son état interne compréhensible depuis l'extérieur.

L'observabilité n'est pas un luxe réservé aux grandes infrastructures. Un outil CLI qui traite des fichiers en batch a besoin de logs lisibles. Un service réseau a besoin de métriques pour dimensionner ses ressources. Un système distribué a besoin de traces pour suivre une requête à travers plusieurs composants. Même un daemon mono-processus bénéficie d'un health check qui permet à un orchestrateur de le redémarrer proprement en cas de blocage.

Ce chapitre couvre les quatre dimensions de l'observabilité appliquées au C++, avec les librairies et les pratiques qui font la différence entre un logiciel qu'on peut exploiter sereinement et un logiciel qu'on subit.

---

## Les trois piliers de l'observabilité

Le modèle dominant de l'observabilité repose sur trois types de données complémentaires, souvent appelés les trois piliers.

### Logs : ce qui s'est passé

Les logs sont des enregistrements textuels horodatés des événements significatifs de l'application : démarrage, requêtes traitées, erreurs rencontrées, décisions prises. Ils sont la forme d'observabilité la plus ancienne et la plus intuitive — un `printf` avec un timestamp est déjà un log.

Mais un `printf` ne suffit pas en production. Les logs doivent être structurés (parsables par machine, pas seulement lisibles par un humain), nivelés (distinguer une information de routine d'une erreur critique), routables (vers un fichier, vers `stdout` pour Docker, vers un collecteur distant), et performants (ne pas ralentir l'application sous charge). La section 40.1 couvre en détail la librairie `spdlog`, le standard de fait du logging C++ moderne, et la compare à `std::print` (C++23).

### Métriques : comment ça se comporte

Les métriques sont des mesures numériques agrégées dans le temps : nombre de requêtes par seconde, latence au 99e percentile, utilisation mémoire, taille d'une file d'attente. Contrairement aux logs qui décrivent des événements individuels, les métriques décrivent des tendances — elles répondent à "est-ce que ça va ?" plutôt qu'à "que s'est-il passé ?".

Le standard de fait pour l'exposition de métriques est **Prometheus** : l'application expose un endpoint HTTP (`/metrics`) au format texte, et un serveur Prometheus le scrape à intervalle régulier. Les métriques sont ensuite visualisées dans Grafana et alimentent des alertes. La section 40.2 montre comment instrumenter une application C++ avec le client Prometheus.

### Traces : comment ça circule

Les traces distribuées suivent le parcours d'une requête à travers les différents composants d'un système — du load balancer au service API, de l'API à la base de données, de la base de données au cache. Chaque étape (appelée un *span*) est horodatée et annotée, ce qui permet de comprendre où le temps est passé et où les erreurs se produisent.

Le tracing distribué est surtout pertinent pour les architectures microservices et les systèmes où une requête traverse plusieurs processus. **OpenTelemetry** est le standard en cours de consolidation (fusion d'OpenTracing et OpenCensus), et il fournit un SDK C++. La section 40.3 couvre son intégration.

---

## Au-delà des trois piliers : la production réelle

Les trois piliers forment le cadre théorique, mais l'exploitation quotidienne d'un service nécessite deux mécanismes supplémentaires qui n'entrent pas proprement dans cette classification.

### Health checks et readiness probes

Un health check est un endpoint minimal (souvent `/healthz` ou `/health`) qui répond "je suis vivant et fonctionnel" ou "j'ai un problème". Les orchestrateurs — Kubernetes, Docker Swarm, systemd, ou même un simple load balancer — utilisent ces endpoints pour décider s'il faut redémarrer un conteneur, retirer un nœud du pool, ou attendre qu'un service soit prêt avant de lui envoyer du trafic.

La distinction entre **liveness** (le processus tourne-t-il ?) et **readiness** (est-il prêt à servir du trafic ?) est fondamentale en environnement orchestré. Un service peut être vivant mais pas prêt — par exemple pendant le chargement d'un cache en mémoire ou l'établissement d'une connexion à la base de données. La section 40.4 couvre l'implémentation de ces endpoints en C++.

### Logging structuré : le pont entre logs et métriques

Le logging structuré — émettre des logs au format JSON ou dans un autre format parsable — est le pont entre les logs traditionnels et les outils d'agrégation modernes (Elasticsearch, Loki, Datadog). Un log structuré peut être indexé, filtré, agrégé en métriques, et corrélé avec des traces — des opérations impossibles ou fragiles sur des logs textuels libres. La section 40.5 couvre les patterns de logging JSON et leur intégration dans les stacks d'agrégation.

---

## L'observabilité en C++ : défis spécifiques

L'instrumentation d'une application C++ présente des contraintes que les langages managés n'ont pas.

### Performance du code d'instrumentation

En C++, chaque microseconde compte — c'est souvent la raison pour laquelle le projet est écrit en C++ plutôt qu'en Python ou Java. L'instrumentation (logs, métriques, traces) ne doit pas dégrader les performances de l'application de manière mesurable. Cela signifie des allocations minimales dans le chemin critique, du buffering asynchrone pour les logs, et des compteurs atomiques plutôt que des mutex pour les métriques. Les librairies couvertes dans ce chapitre (`spdlog`, Prometheus client, OpenTelemetry) sont conçues avec cette contrainte en tête.

### Pas de runtime avec instrumentation intégrée

La JVM fournit nativement des métriques de mémoire (GC stats, heap usage), des thread dumps, et un framework de profiling (JMX). Python a le module `logging` dans sa librairie standard. Le C++ n'a rien de tout cela par défaut. Chaque dimension de l'observabilité doit être ajoutée explicitement via une librairie tierce et intégrée dans le code de l'application. Ce chapitre fournit les librairies et les patterns pour chaque dimension.

### Diagnostics post-mortem

Quand une application C++ plante (segfault, abort, exception non rattrapée), les core dumps et les stack traces sont les outils de diagnostic primaires — couverts au chapitre 29. L'observabilité complète ce dispositif en fournissant le contexte *avant* le crash : les logs qui précèdent l'erreur, les métriques qui montrent une dégradation progressive, et les traces qui identifient la requête fautive. Un crash sans logs ni métriques est un cauchemar opérationnel. Un crash avec un log structuré contenant le request ID, les valeurs des paramètres critiques, et la stack trace (`std::stacktrace` C++23, section 12.12) est un incident diagnosticable.

---

## L'observabilité dans un contexte Cloud Native

Ce chapitre s'inscrit dans la Partie V (DevOps et Cloud Native) de la formation. L'observabilité prend une importance particulière dans les architectures containerisées et orchestrées.

### Docker et stdout

La convention Docker est que les applications écrivent leurs logs sur `stdout` et `stderr` — pas dans des fichiers. Le runtime Docker capture ces flux et les rend accessibles via `docker logs`, et les drivers de logging (json-file, syslog, fluentd, etc.) les routent vers la destination configurée. Votre application C++ doit donc supporter l'envoi de logs vers `stdout`, ce que `spdlog` gère nativement.

### Kubernetes et les probes

Kubernetes utilise les health checks (liveness, readiness, startup probes) pour gérer le cycle de vie des pods. Un service C++ déployé sur Kubernetes qui ne fournit pas ces endpoints perd une grande partie des capacités d'auto-guérison de l'orchestrateur. La section 40.4 montre comment implémenter un serveur HTTP minimal pour ces probes sans ajouter un framework web complet.

### Prometheus et Grafana

L'écosystème Prometheus/Grafana est le standard de monitoring pour Kubernetes et plus largement pour les infrastructures Cloud Native. L'intégration de votre application dans cet écosystème — exposer un `/metrics`, configurer un ServiceMonitor, créer un dashboard Grafana — transforme votre binaire C++ d'une boîte noire en un composant observable au même titre qu'un microservice Java ou Go.

---

## Ce que couvre ce chapitre

Les cinq sections suivantes construisent progressivement une application C++ pleinement observable.

**Section 40.1 — Logging structuré : spdlog vs std::print** couvre la librairie `spdlog` en profondeur : installation, niveaux de log, sinks (fichier, stdout, rotatif, syslog), formatage, et performance. Elle compare `spdlog` à `std::print` (C++23) et explique pourquoi les deux ne répondent pas au même besoin.

**Section 40.2 — Métriques et instrumentation (Prometheus client)** montre comment exposer des compteurs, histogrammes et gauges au format Prometheus depuis une application C++, avec le client `prometheus-cpp`.

**Section 40.3 — Tracing distribué (OpenTelemetry C++)** introduit le SDK OpenTelemetry pour C++ : création de spans, propagation de contexte, et export vers des backends comme Jaeger ou Tempo.

**Section 40.4 — Health checks et readiness probes** détaille l'implémentation d'endpoints `/healthz` et `/readyz` en C++, avec des patterns pour vérifier les dépendances (base de données, files de messages) et s'intégrer aux probes Kubernetes.

**Section 40.5 — Structured logging : JSON logs pour agrégation** couvre les patterns de logging JSON avec `spdlog`, l'enrichissement contextuel (request ID, trace ID, métadonnées), et l'intégration avec les stacks d'agrégation (ELK, Loki, Datadog).

---

## Prérequis

Ce chapitre s'appuie sur plusieurs notions couvertes précédemment dans la formation :

- **Chapitre 22 (Networking)** — Les concepts de sockets TCP et de serveurs HTTP sont utilisés pour les endpoints `/metrics` et `/healthz`.  
- **Chapitre 24 (Sérialisation JSON)** — La librairie `nlohmann/json` est utilisée pour le logging structuré JSON.  
- **Chapitre 26 (CMake)** et **Chapitre 27 (Gestion des dépendances)** — L'installation des librairies d'observabilité passe par Conan ou FetchContent.  
- **Chapitre 37 (Docker)** — Les conventions de logging Docker (`stdout`/`stderr`) et l'intégration avec les orchestrateurs sont référencées.  
- **Chapitre 12.12 (std::stacktrace)** — La génération de stack traces dans les logs d'erreur est mentionnée.

Une familiarité avec les concepts de base du monitoring (métriques, alertes, dashboards) est utile mais pas indispensable — chaque section introduit les concepts nécessaires avant de montrer l'implémentation C++.

⏭️ [Logging structuré : spdlog (recommandé) vs std::print](/40-observabilite/01-logging-spdlog.md)
