🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 40.4 — Health checks et readiness probes

## Dire à l'infrastructure si votre application va bien

---

## Introduction

Les logs, les métriques et les traces sont des outils d'observation pour les humains. Les health checks sont des outils d'observation pour les **machines** — les orchestrateurs, les load balancers, et les systèmes de supervision automatique qui décident, sans intervention humaine, s'il faut redémarrer un conteneur, retirer un nœud du pool de trafic, ou attendre avant de router des requêtes vers une nouvelle instance.

Un service C++ qui ne fournit pas de health checks est une boîte opaque pour l'infrastructure. Kubernetes ne sait pas s'il a planté silencieusement (deadlock, boucle infinie). Le load balancer continue de lui envoyer du trafic alors qu'il n'a pas fini de charger sa configuration. Le système de déploiement déclare un rollout réussi alors que l'application crashe en boucle au bout de 10 secondes.

Implémenter des health checks est un investissement minimal — quelques dizaines de lignes de code — pour un gain opérationnel considérable. Cette section couvre les trois types de probes, leur implémentation en C++ sans framework web lourd, et leur intégration avec Kubernetes et systemd.

---

## Les trois types de probes

Kubernetes a formalisé trois types de probes, et cette terminologie s'est imposée au-delà de Kubernetes comme vocabulaire standard de l'industrie.

### Liveness : le processus est-il vivant ?

La probe de **liveness** répond à une question binaire : "le processus fonctionne-t-il encore normalement, ou est-il dans un état irrécupérable ?". Un processus qui a subi un deadlock est techniquement vivant (le PID existe, le processus consomme de la mémoire) mais fonctionnellement mort — il ne traitera plus jamais de requête.

Quand une probe de liveness échoue de manière répétée, l'orchestrateur **tue et redémarre** le conteneur. C'est un mécanisme de dernier recours, l'équivalent automatisé du `kill -9` manuel suivi d'un `systemctl restart`.

Le endpoint conventionnel est `/healthz` ou `/livez`.

**Ce que la liveness doit vérifier** : le minimum vital. Le thread principal répond-il ? L'application n'est-elle pas dans une boucle infinie ou un deadlock ? Un simple "je suis capable de traiter une requête HTTP et de retourner 200" suffit dans la plupart des cas. La liveness ne doit *pas* vérifier les dépendances externes (base de données, cache, services aval) — sinon une panne de base de données provoquerait un redémarrage en cascade de tous les services qui en dépendent, aggravant la situation au lieu de l'améliorer.

### Readiness : le service est-il prêt à recevoir du trafic ?

La probe de **readiness** répond à une question plus nuancée : "est-il pertinent d'envoyer du trafic à cette instance en ce moment ?". Un service peut être vivant mais pas prêt — par exemple pendant le chargement d'un cache en mémoire, l'établissement d'un pool de connexions à la base de données, ou la synchronisation initiale avec un service de configuration.

Quand une probe de readiness échoue, l'orchestrateur **retire l'instance du pool de trafic** (le Service Kubernetes cesse de lui envoyer des requêtes) mais ne la tue pas. Dès que la probe réussit à nouveau, l'instance est réintégrée dans le pool.

Le endpoint conventionnel est `/readyz` ou `/ready`.

**Ce que la readiness doit vérifier** : la capacité effective à servir des requêtes. Les dépendances critiques sont-elles accessibles ? Le cache initial est-il chargé ? Le pool de connexions est-il établi ? La readiness *doit* vérifier les dépendances externes dont l'absence rend le service incapable de répondre correctement.

### Startup : le démarrage est-il terminé ?

La probe de **startup** est une variante de la liveness utilisée pendant la phase de démarrage. Certaines applications C++ ont un temps de démarrage long : chargement d'un modèle de machine learning en mémoire, lecture d'un fichier de données volumineux, préchauffage d'un cache. Pendant cette phase, la liveness probe échouerait et provoquerait un redémarrage prématuré.

La startup probe protège contre ce scénario : tant qu'elle n'a pas réussi au moins une fois, la liveness probe n'est pas évaluée. Une fois la startup probe réussie, elle est désactivée et la liveness prend le relais.

Le endpoint est souvent le même que la liveness (`/healthz`), mais avec des seuils de tolérance différents configurés côté Kubernetes.

---

## Implémentation en C++ : serveur HTTP minimal

Un health check est un endpoint HTTP qui retourne `200 OK` quand tout va bien et `503 Service Unavailable` quand quelque chose ne va pas. L'implémentation n'a pas besoin d'un framework web complet — un serveur HTTP minimal suffit.

### Avec cpp-httplib (header-only)

La librairie **cpp-httplib** (couverte en section 22.5) est un excellent choix pour les health checks : header-only, pas de dépendance externe, API simple.

```cpp
#include <httplib.h>
#include <spdlog/spdlog.h>
#include <atomic>
#include <thread>

class HealthServer {  
public:  
    explicit HealthServer(int port) : port_(port) {}

    void start() {
        thread_ = std::thread([this]() {
            httplib::Server svr;

            // Liveness : le processus est-il fonctionnel ?
            svr.Get("/healthz", [this](const httplib::Request&, httplib::Response& res) {
                if (alive_.load(std::memory_order_relaxed)) {
                    res.status = 200;
                    res.set_content(R"({"status":"ok"})", "application/json");
                } else {
                    res.status = 503;
                    res.set_content(R"({"status":"dead"})", "application/json");
                }
            });

            // Readiness : le service est-il prêt à recevoir du trafic ?
            svr.Get("/readyz", [this](const httplib::Request&, httplib::Response& res) {
                auto checks = run_readiness_checks();
                if (checks.all_passed) {
                    res.status = 200;
                    res.set_content(checks.to_json(), "application/json");
                } else {
                    res.status = 503;
                    res.set_content(checks.to_json(), "application/json");
                }
            });

            spdlog::info("Health server listening on :{}", port_);
            svr.listen("0.0.0.0", port_);
        });
    }

    void stop() {
        alive_.store(false, std::memory_order_relaxed);
        if (thread_.joinable()) thread_.join();
    }

    // Appelé par l'application quand elle est prête
    void set_ready(bool ready) {
        ready_.store(ready, std::memory_order_relaxed);
    }

    // Appelé pour signaler un état irrécupérable
    void set_alive(bool alive) {
        alive_.store(alive, std::memory_order_relaxed);
    }

private:
    struct CheckResults {
        bool all_passed = true;
        bool database_ok = false;
        bool cache_ok = false;
        bool config_loaded = false;

        std::string to_json() const {
            return fmt::format(
                R"({{"status":"{}","checks":{{"database":{},"cache":{},"config":{}}}}})",
                all_passed ? "ready" : "not_ready",
                database_ok ? "true" : "false",
                cache_ok ? "true" : "false",
                config_loaded ? "true" : "false"
            );
        }
    };

    CheckResults run_readiness_checks() {
        CheckResults results;
        results.config_loaded = ready_.load(std::memory_order_relaxed);
        results.database_ok = check_database();
        results.cache_ok = check_cache();
        results.all_passed = results.config_loaded
                          && results.database_ok
                          && results.cache_ok;
        return results;
    }

    bool check_database() {
        // Exécuter une requête triviale (SELECT 1, PING, etc.)
        // Retourner false si timeout ou erreur
        // Implémenter selon votre driver de base de données
        return db_connection_pool_ != nullptr
            && db_connection_pool_->is_healthy();
    }

    bool check_cache() {
        // Vérifier que le cache est accessible
        return cache_client_ != nullptr
            && cache_client_->ping();
    }

    int port_;
    std::atomic<bool> alive_{true};
    std::atomic<bool> ready_{false};
    std::thread thread_;

    // Références aux dépendances (injectées au démarrage)
    DatabasePool* db_connection_pool_ = nullptr;
    CacheClient* cache_client_ = nullptr;
};
```

### Utilisation dans l'application

```cpp
int main() {
    spdlog::info("Syswatch starting...");

    // Démarrer le serveur de santé immédiatement
    HealthServer health(8081);
    health.start();
    // À ce stade, /healthz retourne 200, /readyz retourne 503

    // Phase de démarrage (potentiellement longue)
    auto config = load_config();            // 1-2 secondes
    auto db_pool = init_database(config);   // 2-5 secondes
    auto cache = init_cache(config);        // 1-3 secondes
    warm_up_cache(cache);                   // 5-30 secondes

    // Signaler que le service est prêt
    health.set_ready(true);
    spdlog::info("Syswatch ready, accepting traffic");
    // Maintenant /readyz retourne aussi 200

    // Boucle principale
    run_server(config, db_pool, cache);

    health.stop();
    return 0;
}
```

La chronologie est importante. Le serveur de santé démarre **avant** l'initialisation de l'application. Pendant toute la phase de démarrage, `/healthz` retourne 200 (le processus est vivant) mais `/readyz` retourne 503 (le service n'est pas encore prêt à servir du trafic). L'orchestrateur voit un service vivant mais pas prêt — il ne le tue pas et ne lui envoie pas de trafic. Dès que l'initialisation est terminée, `set_ready(true)` fait basculer `/readyz` vers 200, et le trafic commence à arriver.

---

## Port dédié pour les probes

Comme pour les métriques Prometheus (section 40.2), les health checks doivent être exposés sur un **port séparé** du port applicatif principal. Si votre service écoute sur le port 8080 pour les requêtes client et sur le port 9090 pour les métriques, les probes sont sur un troisième port (par exemple 8081) — ou regroupées avec les métriques sur 9090.

Le regroupement des probes et des métriques sur un même port d'administration est un pattern courant et pragmatique :

```cpp
// Port 9090 : métriques Prometheus + health checks
httplib::Server admin_server;

admin_server.Get("/metrics", [&](const auto&, auto& res) {
    // Sérialiser les métriques Prometheus
    res.set_content(serialize_metrics(), "text/plain");
});

admin_server.Get("/healthz", [&](const auto&, auto& res) {
    res.status = 200;
    res.set_content(R"({"status":"ok"})", "application/json");
});

admin_server.Get("/readyz", [&](const auto&, auto& res) {
    // ... vérifications de readiness ...
});

admin_server.listen("0.0.0.0", 9090);
```

Si vous utilisez l'exposeur HTTP de prometheus-cpp (section 40.2), vous pouvez enregistrer les handlers de health checks sur le même serveur en utilisant son API de handlers personnalisés, ou bien garder deux serveurs séparés. La séparation a l'avantage de l'isolation : si le serveur de métriques a un problème, les health checks continuent de fonctionner.

---

## Vérifications de dépendances : patterns et pièges

La readiness probe doit vérifier les dépendances critiques. Mais toutes les dépendances ne sont pas égales, et les vérifications elles-mêmes ont des pièges.

### Vérifications légères, pas de travail réel

Un health check doit être rapide — idéalement moins de 100ms. La vérification de la base de données doit être un `SELECT 1` ou un `PING`, pas une requête métier complexe. La vérification du cache doit être un `PING`, pas une lecture d'une clé arbitraire. L'objectif est de confirmer que la connexion fonctionne, pas de valider le contenu des données.

```cpp
bool check_database() {
    try {
        auto conn = db_pool_->acquire(std::chrono::milliseconds(500));
        if (!conn) return false;
        conn->execute("SELECT 1");
        return true;
    } catch (...) {
        return false;
    }
}
```

Le timeout de 500ms est important : sans lui, un health check bloqué sur une connexion réseau pourrait prendre 30 secondes (le timeout TCP par défaut) — pendant lesquelles l'orchestrateur considère la probe comme non répondue et potentiellement tue le conteneur.

### Distinguer les dépendances critiques des dépendances optionnelles

Toutes les dépendances ne justifient pas un échec de readiness. Si votre service peut fonctionner en mode dégradé sans le cache (en allant directement à la base de données, avec des performances réduites), le cache est une dépendance optionnelle — son indisponibilité mérite un `warn` dans les logs et une métrique dégradée, mais pas un retrait du pool de trafic.

```cpp
CheckResults run_readiness_checks() {
    CheckResults results;

    // Dépendances critiques : l'échec retire du pool
    results.database_ok = check_database();
    results.config_loaded = config_loaded_.load();

    // Dépendances optionnelles : loguées mais n'affectent pas le statut
    results.cache_ok = check_cache();
    if (!results.cache_ok) {
        spdlog::warn("Cache unavailable, operating in degraded mode");
    }

    // Seules les dépendances critiques comptent
    results.all_passed = results.database_ok && results.config_loaded;
    return results;
}
```

### Ne pas vérifier les dépendances dans la liveness

C'est le piège le plus fréquent et le plus dangereux. Si votre liveness probe vérifie que la base de données est accessible, et que la base de données tombe, voici ce qui se passe : la liveness échoue → Kubernetes tue le conteneur → Kubernetes redémarre le conteneur → la liveness échoue à nouveau (la base est toujours en panne) → Kubernetes tue à nouveau → boucle infinie de redémarrages. Pendant ce temps, vos pools de connexions sont détruits et recréés en boucle, aggravant la charge sur la base de données quand elle revient.

La liveness doit vérifier uniquement l'état interne du processus. La readiness doit vérifier les dépendances externes.

---

## Réponses détaillées : aider au diagnostic

Un health check qui retourne uniquement 200 ou 503 remplit son contrat avec l'orchestrateur, mais n'aide pas les opérateurs qui investiguent un problème. Enrichir le corps de la réponse accélère le diagnostic :

```json
{
  "status": "not_ready",
  "uptime_seconds": 342,
  "checks": {
    "database": {
      "status": "ok",
      "latency_ms": 3
    },
    "cache": {
      "status": "failed",
      "error": "connection refused",
      "last_success": "2026-03-18T14:20:12Z"
    },
    "config": {
      "status": "ok",
      "version": "1.2.0",
      "loaded_at": "2026-03-18T14:17:45Z"
    }
  },
  "version": "1.2.0",
  "git_commit": "a1b2c3d"
}
```

Les champs `version` et `git_commit` sont particulièrement utiles pendant les déploiements : ils confirment quelle version est effectivement en train de tourner, ce qui aide à identifier les rollouts partiels ou échoués.

L'implémentation de cette réponse détaillée utilise `nlohmann/json` (chapitre 24) pour un formatage robuste :

```cpp
#include <nlohmann/json.hpp>

std::string build_health_response(const CheckResults& checks) {
    nlohmann::json response;
    response["status"] = checks.all_passed ? "ready" : "not_ready";
    response["uptime_seconds"] = get_uptime_seconds();
    response["version"] = APP_VERSION;
    response["git_commit"] = GIT_COMMIT;

    auto& c = response["checks"];
    c["database"]["status"] = checks.database_ok ? "ok" : "failed";
    c["cache"]["status"] = checks.cache_ok ? "ok" : "failed";
    c["config"]["status"] = checks.config_loaded ? "ok" : "failed";

    return response.dump();
}
```

---

## Intégration Kubernetes

### Configuration des probes dans le manifeste

```yaml
apiVersion: apps/v1  
kind: Deployment  
metadata:  
  name: syswatch
spec:
  template:
    spec:
      containers:
        - name: syswatch
          image: exemple/syswatch:1.2.0
          ports:
            - name: app
              containerPort: 8080
            - name: admin
              containerPort: 9090

          # Liveness : le processus est-il fonctionnel ?
          livenessProbe:
            httpGet:
              path: /healthz
              port: admin
            initialDelaySeconds: 5
            periodSeconds: 10
            timeoutSeconds: 3
            failureThreshold: 3

          # Readiness : prêt à recevoir du trafic ?
          readinessProbe:
            httpGet:
              path: /readyz
              port: admin
            initialDelaySeconds: 10
            periodSeconds: 5
            timeoutSeconds: 3
            failureThreshold: 2

          # Startup : démarrage terminé ?
          startupProbe:
            httpGet:
              path: /healthz
              port: admin
            periodSeconds: 5
            failureThreshold: 30   # 30 × 5s = 150s max de démarrage
```

Chaque paramètre a un impact direct sur le comportement :

**`initialDelaySeconds`** est le délai avant la première vérification. Pour la liveness, un délai court (5s) suffit — le processus devrait être capable de répondre à `/healthz` quasi immédiatement. Pour la readiness, un délai plus long laisse le temps au démarrage initial.

**`periodSeconds`** est l'intervalle entre deux vérifications. 10 secondes pour la liveness (pas besoin de détecter un deadlock à la seconde près), 5 secondes pour la readiness (réintégrer rapidement une instance redevenue saine).

**`timeoutSeconds`** est le temps d'attente maximal pour une réponse. 3 secondes est un bon défaut. Au-delà, la probe est considérée comme échouée.

**`failureThreshold`** est le nombre d'échecs consécutifs avant d'agir. 3 pour la liveness signifie que l'orchestrateur attend 3 échecs consécutifs (soit 30 secondes avec `periodSeconds: 10`) avant de tuer le conteneur — cela tolère un spike de latence ponctuel sans redémarrage intempestif.

**La startup probe** dans cet exemple donne 150 secondes au service pour démarrer (30 tentatives × 5 secondes). Tant qu'elle n'a pas réussi, les probes de liveness et readiness ne sont pas évaluées. C'est essentiel pour les applications C++ avec un temps de démarrage long (chargement de données, préchauffage de cache, compilation JIT).

---

## Intégration systemd

En dehors de Kubernetes, **systemd** est l'autre consommateur principal des health checks pour les services C++ déployés sur des serveurs bare metal ou des VM.

### Watchdog systemd

systemd offre un mécanisme de watchdog : le service doit envoyer un signal "je suis vivant" à intervalles réguliers. Si le signal cesse, systemd considère le service comme défaillant et le redémarre.

```ini
# /usr/lib/systemd/system/syswatch.service
[Unit]
Description=Syswatch Monitoring Agent  
After=network.target  

[Service]
Type=notify  
ExecStart=/usr/bin/syswatch  
WatchdogSec=30  
Restart=on-failure  
RestartSec=5  

[Install]
WantedBy=multi-user.target
```

Le paramètre `WatchdogSec=30` signifie que le service doit envoyer un signal au moins toutes les 30 secondes. La convention est d'envoyer le signal à la moitié de l'intervalle (toutes les 15 secondes) pour avoir une marge de sécurité.

L'implémentation en C++ utilise l'API `sd_notify` de libsystemd :

```cpp
#include <systemd/sd-daemon.h>
#include <thread>
#include <atomic>

class SystemdWatchdog {  
public:  
    void start() {
        // Lire l'intervalle configuré par systemd
        uint64_t usec = 0;
        if (sd_watchdog_enabled(0, &usec) <= 0) {
            spdlog::info("Systemd watchdog not enabled");
            return;
        }

        // Envoyer le signal à la moitié de l'intervalle
        auto interval = std::chrono::microseconds(usec / 2);
        spdlog::info("Systemd watchdog enabled, interval={}ms",
                     std::chrono::duration_cast<std::chrono::milliseconds>(interval).count());

        thread_ = std::thread([this, interval]() {
            while (running_.load(std::memory_order_relaxed)) {
                // Vérifier que l'application est saine avant de signaler
                if (is_healthy()) {
                    sd_notify(0, "WATCHDOG=1");
                }
                std::this_thread::sleep_for(interval);
            }
        });
    }

    void notify_ready() {
        sd_notify(0, "READY=1");  // Signaler que le service est prêt
        spdlog::info("Notified systemd: READY");
    }

    void notify_stopping() {
        sd_notify(0, "STOPPING=1");
        spdlog::info("Notified systemd: STOPPING");
    }

    void stop() {
        running_.store(false, std::memory_order_relaxed);
        if (thread_.joinable()) thread_.join();
    }

private:
    bool is_healthy() {
        // Même logique que la liveness probe : vérification interne uniquement
        return app_is_functional();
    }

    std::atomic<bool> running_{true};
    std::thread thread_;
};
```

L'utilisation dans le `main()` :

```cpp
int main() {
    SystemdWatchdog watchdog;
    watchdog.start();

    // Phase de démarrage
    auto config = load_config();
    auto db_pool = init_database(config);

    // Signaler à systemd que le service est prêt
    watchdog.notify_ready();

    // Boucle principale
    run_server(config, db_pool);

    watchdog.notify_stopping();
    watchdog.stop();
    return 0;
}
```

Le `Type=notify` dans le fichier unit systemd signifie que systemd attend le signal `READY=1` avant de considérer le service comme démarré. C'est l'équivalent exact de la startup/readiness probe Kubernetes : le service n'est pas considéré comme fonctionnel tant qu'il n'a pas explicitement dit qu'il était prêt.

Pour lier libsystemd dans votre projet CMake :

```cmake
find_package(PkgConfig REQUIRED)  
pkg_check_modules(SYSTEMD IMPORTED_TARGET libsystemd)  

if(SYSTEMD_FOUND)
    target_link_libraries(syswatch PRIVATE PkgConfig::SYSTEMD)
    target_compile_definitions(syswatch PRIVATE HAS_SYSTEMD)
endif()
```

Le guard `HAS_SYSTEMD` permet de compiler le code sur des systèmes sans systemd (macOS, conteneurs minimaux) en désactivant conditionnellement le watchdog :

```cpp
void SystemdWatchdog::start() {
#ifdef HAS_SYSTEMD
    // ... code sd_notify ...
#else
    spdlog::debug("Systemd watchdog not available on this platform");
#endif
}
```

---

## Pattern complet : health checks + métriques + watchdog

Voici comment les différents mécanismes s'articulent dans un service C++ de production :

```cpp
int main() {
    // 1. Logging (section 40.1)
    syswatch::logging::init({.level = "info"});

    // 2. Métriques Prometheus (section 40.2) + Health checks
    //    sur le même port d'administration
    HealthServer admin_server(9090);
    admin_server.start();

    // 3. Watchdog systemd
    SystemdWatchdog watchdog;
    watchdog.start();

    // 4. Phase de démarrage
    spdlog::info("Syswatch starting...");
    auto config = load_config();
    auto db_pool = init_database(config);
    auto cache = init_cache(config);

    // 5. Signaler la disponibilité
    admin_server.set_ready(true);      // /readyz → 200
    watchdog.notify_ready();           // systemd → READY=1
    spdlog::info("Syswatch ready");

    // 6. Boucle principale
    run_server(config, db_pool, cache);

    // 7. Arrêt propre
    spdlog::info("Syswatch shutting down...");
    admin_server.set_ready(false);     // /readyz → 503 (drain)
    watchdog.notify_stopping();        // systemd → STOPPING=1

    // Laisser le temps aux requêtes en cours de se terminer
    std::this_thread::sleep_for(std::chrono::seconds(5));

    admin_server.stop();
    watchdog.stop();
    syswatch::logging::shutdown();
    return 0;
}
```

La séquence d'arrêt mérite attention. Avant de couper le service, on signale à la readiness probe et à systemd que le service est en cours d'arrêt. Le load balancer ou Kubernetes retire l'instance du pool de trafic. La pause de 5 secondes (le *graceful shutdown period*) laisse le temps aux requêtes en cours de se terminer et au load balancer de propager le changement. Ce n'est qu'ensuite que les composants sont réellement arrêtés.

---

## Résumé

Les health checks sont le contrat entre votre application C++ et l'infrastructure qui l'héberge. La liveness probe confirme que le processus fonctionne (vérification interne uniquement, jamais les dépendances). La readiness probe confirme que le service est prêt à recevoir du trafic (vérification des dépendances critiques). La startup probe protège les applications avec un temps de démarrage long.

L'implémentation en C++ ne nécessite qu'un serveur HTTP minimal (cpp-httplib) sur un port d'administration, quelques atomiques pour suivre l'état, et des vérifications légères des dépendances. L'intégration avec Kubernetes (probes HTTP) et systemd (sd_notify + watchdog) transforme votre service d'une boîte opaque en un citoyen de première classe de l'infrastructure.

La section suivante (40.5) clôt le chapitre en revenant sur le logging pour couvrir le JSON structuré et l'intégration avec les stacks d'agrégation — le dernier maillon pour une observabilité complète.

⏭️ [Structured logging : JSON logs pour agrégation](/40-observabilite/05-json-logs.md)
