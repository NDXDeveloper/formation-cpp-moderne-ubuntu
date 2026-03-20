🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 40.2 — Métriques et instrumentation (Prometheus client)

## Donner des chiffres à votre application, pas seulement des mots

---

## Introduction

Les logs racontent ce qui s'est passé. Les métriques racontent comment ça se passe *en ce moment* — et comment ça évolue dans le temps. Un log dit "la requête X a pris 5032ms". Une métrique dit "la latence au 99e percentile est passée de 12ms à 340ms au cours des 5 dernières minutes". Le premier est un fait ponctuel. Le second est un signal d'alerte.

Les métriques sont le fondement du monitoring proactif : elles alimentent les dashboards que les opérateurs surveillent, les alertes qui déclenchent les interventions, et les analyses de capacité qui guident le dimensionnement. Sans métriques, vous êtes aveugle jusqu'à ce qu'un utilisateur vous signale un problème. Avec des métriques, vous voyez la dégradation arriver et vous intervenez avant l'impact.

**Prometheus** est le standard de fait pour le monitoring dans l'écosystème Cloud Native. Adopté comme deuxième projet gradué de la Cloud Native Computing Foundation (après Kubernetes), il est utilisé par la majorité des organisations qui déploient sur Kubernetes — et bien au-delà. Cette section montre comment instrumenter une application C++ avec le client Prometheus pour exposer des métriques exploitables.

---

## Le modèle Prometheus en 60 secondes

Prometheus fonctionne sur un modèle **pull** : c'est le serveur Prometheus qui vient interroger vos applications à intervalle régulier (typiquement toutes les 15 ou 30 secondes), et non l'application qui pousse ses métriques vers un collecteur. Ce modèle a des conséquences architecturales importantes.

Votre application expose un endpoint HTTP (par convention `/metrics`) qui retourne l'état actuel de toutes ses métriques dans un format texte standardisé. Le serveur Prometheus scrape cet endpoint, stocke les valeurs horodatées dans sa base de données temporelle (TSDB), et les rend disponibles pour les requêtes PromQL, les dashboards Grafana, et les règles d'alerte.

Le format texte exposé est simple et lisible :

```
# HELP http_requests_total Total number of HTTP requests processed
# TYPE http_requests_total counter
http_requests_total{method="GET",path="/api/metrics",status="200"} 14523  
http_requests_total{method="POST",path="/api/data",status="201"} 3847  
http_requests_total{method="GET",path="/api/metrics",status="500"} 12  

# HELP http_request_duration_seconds Duration of HTTP requests
# TYPE http_request_duration_seconds histogram
http_request_duration_seconds_bucket{method="GET",le="0.005"} 12034  
http_request_duration_seconds_bucket{method="GET",le="0.01"} 13891  
http_request_duration_seconds_bucket{method="GET",le="0.025"} 14400  
http_request_duration_seconds_bucket{method="GET",le="+Inf"} 14523  
http_request_duration_seconds_sum{method="GET"} 87.32  
http_request_duration_seconds_count{method="GET"} 14523  
```

Chaque ligne est une série temporelle identifiée par un nom et un ensemble de labels (paires clé-valeur entre accolades). Prometheus stocke une valeur numérique pour chaque combinaison nom+labels à chaque scrape.

---

## Les quatre types de métriques

Prometheus définit quatre types de métriques, chacun adapté à un usage spécifique.

### Counter — Compteur monotone croissant

Un counter est une valeur qui ne fait qu'augmenter (ou revenir à zéro lors d'un redémarrage). Il mesure le nombre cumulé d'occurrences d'un événement : requêtes traitées, octets envoyés, erreurs rencontrées.

On n'affiche jamais la valeur brute d'un counter dans un dashboard — elle augmente indéfiniment. On affiche son **taux de variation** : `rate(http_requests_total[5m])` donne le nombre de requêtes par seconde moyenné sur 5 minutes. C'est PromQL (le langage de requête de Prometheus) qui fait ce calcul, pas votre application.

### Gauge — Valeur instantanée

Un gauge est une valeur qui peut monter et descendre : utilisation mémoire, nombre de connexions actives, taille d'une file d'attente, température CPU. C'est une photographie de l'état courant au moment du scrape.

### Histogram — Distribution de valeurs

Un histogram mesure la distribution d'une grandeur (typiquement une latence ou une taille) en comptant le nombre d'observations dans des "buckets" prédéfinis. Il permet de calculer des percentiles (P50, P95, P99) et des moyennes sans stocker chaque observation individuelle.

Les buckets sont définis à la création de la métrique. Prometheus recommande des buckets exponentiels pour les latences : `{0.005, 0.01, 0.025, 0.05, 0.1, 0.25, 0.5, 1.0, 2.5, 5.0, 10.0}` secondes. Le choix des buckets est un compromis entre précision et volume de données.

### Summary — Percentiles calculés côté client

Un summary est similaire à un histogram mais calcule les percentiles directement dans l'application plutôt que côté serveur. Il est moins courant car les percentiles calculés côté client ne sont pas agrégables entre instances — vous ne pouvez pas combiner le P99 de 10 instances pour obtenir un P99 global. Préférez les histograms dans la majorité des cas.

---

## prometheus-cpp : le client C++ de référence

La librairie **prometheus-cpp** est le client Prometheus officiel pour C++. Elle fournit les quatre types de métriques, un exposeur HTTP intégré pour le endpoint `/metrics`, et une API thread-safe.

### Installation via Conan

```python
# conanfile.py
def requirements(self):
    self.requires("spdlog/1.15.3")
    self.requires("prometheus-cpp/1.3.0")
```

```cmake
find_package(prometheus-cpp REQUIRED)

target_link_libraries(syswatch PRIVATE
    spdlog::spdlog
    prometheus-cpp::pull    # Exposeur HTTP (modèle pull)
    prometheus-cpp::core    # Types de métriques
)
```

Le composant `pull` fournit le serveur HTTP intégré qui expose `/metrics`. Le composant `core` fournit les types de métriques (Counter, Gauge, Histogram, etc.).

### Installation via FetchContent

```cmake
include(FetchContent)

FetchContent_Declare(
    prometheus-cpp
    GIT_REPOSITORY https://github.com/jupp0r/prometheus-cpp.git
    GIT_TAG        v1.3.0
)

set(ENABLE_PUSH OFF CACHE BOOL "" FORCE)   # Pas de push gateway  
set(ENABLE_COMPRESSION OFF CACHE BOOL "" FORCE)  

FetchContent_MakeAvailable(prometheus-cpp)

target_link_libraries(syswatch PRIVATE
    prometheus-cpp::pull
    prometheus-cpp::core
)
```

### Installation via vcpkg

```bash
vcpkg install prometheus-cpp
```

Le `CMakeLists.txt` est identique à la version Conan.

---

## Instrumentation d'une application C++

### Mise en place du registry et de l'exposeur

Le point de départ est un **registry** (registre de métriques) et un **exposeur** (serveur HTTP) :

```cpp
#include <prometheus/counter.h>
#include <prometheus/gauge.h>
#include <prometheus/histogram.h>
#include <prometheus/exposer.h>
#include <prometheus/registry.h>

int main() {
    // Créer l'exposeur HTTP sur le port 9090
    prometheus::Exposer exposer{"0.0.0.0:9090"};

    // Créer un registry (conteneur de métriques)
    auto registry = std::make_shared<prometheus::Registry>();

    // Enregistrer le registry auprès de l'exposeur
    exposer.RegisterCollectable(registry);

    // ... création des métriques et logique applicative ...
}
```

Dès ce point, `http://localhost:9090/metrics` répond avec les métriques enregistrées. Le serveur HTTP intégré tourne dans un thread séparé — il n'interfère pas avec le thread principal de l'application.

### Créer et utiliser un Counter

```cpp
// Construire une famille de counters avec des labels
auto& request_counter = prometheus::BuildCounter()
    .Name("http_requests_total")
    .Help("Total number of HTTP requests processed")
    .Register(*registry);

// Créer des instances avec des combinaisons de labels spécifiques
auto& get_200 = request_counter.Add({{"method", "GET"}, {"status", "200"}});  
auto& get_500 = request_counter.Add({{"method", "GET"}, {"status", "500"}});  
auto& post_201 = request_counter.Add({{"method", "POST"}, {"status", "201"}});  

// Dans le code de traitement des requêtes
void handle_request(const Request& req) {
    auto response = process(req);
    
    if (req.method() == "GET" && response.status() == 200) {
        get_200.Increment();
    } else if (req.method() == "GET" && response.status() == 500) {
        get_500.Increment();
    }
    // ...
}
```

L'appel `Increment()` est atomique et thread-safe — aucun mutex n'est nécessaire. Le coût est celui d'un `fetch_add` atomique, soit quelques nanosecondes.

En pratique, pour éviter l'explosion combinatoire des instances créées statiquement, on crée les combinaisons de labels dynamiquement :

```cpp
void handle_request(const Request& req) {
    auto response = process(req);
    
    request_counter.Add({
        {"method", req.method()},
        {"path", req.path()},
        {"status", std::to_string(response.status())}
    }).Increment();
}
```

Attention : chaque combinaison unique de labels crée une nouvelle série temporelle dans Prometheus. Un label à haute cardinalité (comme un user ID ou un request ID) génère des milliers de séries et peut saturer Prometheus. Les labels doivent avoir un nombre fini et raisonnable de valeurs (méthode HTTP, code de statut, nom d'endpoint — pas d'identifiant unique).

### Créer et utiliser un Gauge

```cpp
auto& system_gauge = prometheus::BuildGauge()
    .Name("syswatch_system_info")
    .Help("System resource usage")
    .Register(*registry);

auto& memory_usage = system_gauge.Add({{"resource", "memory_bytes"}});  
auto& active_conns = system_gauge.Add({{"resource", "active_connections"}});  
auto& queue_size   = system_gauge.Add({{"resource", "queue_size"}});  

// Mise à jour périodique (dans un thread de monitoring)
void update_system_metrics() {
    memory_usage.Set(get_memory_usage_bytes());
    active_conns.Set(connection_pool.active_count());
    queue_size.Set(work_queue.size());
}

// Ou incrément/décrément ponctuel
void on_connection_open() { active_conns.Increment(); }  
void on_connection_close() { active_conns.Decrement(); }  
```

Les méthodes `Set()`, `Increment()` et `Decrement()` sont toutes atomiques et thread-safe. `Set()` remplace la valeur courante. `Increment()` et `Decrement()` l'ajustent de 1 (ou d'une valeur spécifiée).

### Créer et utiliser un Histogram

```cpp
auto& latency_histogram = prometheus::BuildHistogram()
    .Name("http_request_duration_seconds")
    .Help("Duration of HTTP requests in seconds")
    .Register(*registry);

// Buckets adaptés aux latences HTTP (en secondes)
auto& get_latency = latency_histogram.Add(
    {{"method", "GET"}},
    prometheus::Histogram::BucketBoundaries{
        0.001, 0.005, 0.01, 0.025, 0.05, 0.1, 0.25, 0.5, 1.0, 5.0
    }
);

// Dans le code de traitement
void handle_request(const Request& req) {
    auto start = std::chrono::steady_clock::now();
    
    auto response = process(req);
    
    auto end = std::chrono::steady_clock::now();
    double duration_s = std::chrono::duration<double>(end - start).count();
    
    get_latency.Observe(duration_s);
}
```

L'appel `Observe(value)` incrémente atomiquement le compteur du bucket approprié, la somme totale, et le nombre total d'observations. Côté Prometheus, la requête PromQL `histogram_quantile(0.99, rate(http_request_duration_seconds_bucket[5m]))` calcule le 99e percentile de latence sur les 5 dernières minutes.

Le choix des buckets est critique. Des buckets trop larges perdent en précision. Des buckets trop nombreux augmentent le volume de données. Pour les latences HTTP, les valeurs ci-dessus couvrent de 1ms à 5s avec une résolution logarithmique — c'est un bon point de départ. Ajustez en fonction du profil de latence réel de votre application.

---

## Architecture d'instrumentation recommandée

Pour un projet structuré, centralisez la création des métriques dans un module dédié plutôt que de les disperser dans le code applicatif :

```cpp
// metrics.h
#pragma once

#include <prometheus/counter.h>
#include <prometheus/gauge.h>
#include <prometheus/histogram.h>
#include <prometheus/registry.h>

namespace syswatch::metrics {

struct Metrics {
    // Requêtes HTTP
    prometheus::Family<prometheus::Counter>& http_requests;
    prometheus::Family<prometheus::Histogram>& http_duration;
    
    // Ressources système
    prometheus::Family<prometheus::Gauge>& memory_usage;
    prometheus::Family<prometheus::Gauge>& cpu_usage;
    
    // Composant base de données
    prometheus::Family<prometheus::Counter>& db_queries;
    prometheus::Family<prometheus::Histogram>& db_duration;
    prometheus::Family<prometheus::Gauge>& db_connections;
};

// Initialiser les métriques et l'exposeur
Metrics init(std::shared_ptr<prometheus::Registry>& registry);

}  // namespace syswatch::metrics
```

```cpp
// metrics.cpp
#include "metrics.h"

namespace syswatch::metrics {

Metrics init(std::shared_ptr<prometheus::Registry>& registry) {
    return Metrics{
        .http_requests = prometheus::BuildCounter()
            .Name("syswatch_http_requests_total")
            .Help("Total HTTP requests processed")
            .Register(*registry),

        .http_duration = prometheus::BuildHistogram()
            .Name("syswatch_http_request_duration_seconds")
            .Help("HTTP request duration in seconds")
            .Register(*registry),

        .memory_usage = prometheus::BuildGauge()
            .Name("syswatch_memory_usage_bytes")
            .Help("Current memory usage in bytes")
            .Register(*registry),

        .cpu_usage = prometheus::BuildGauge()
            .Name("syswatch_cpu_usage_ratio")
            .Help("CPU usage ratio (0.0 to 1.0)")
            .Register(*registry),

        .db_queries = prometheus::BuildCounter()
            .Name("syswatch_db_queries_total")
            .Help("Total database queries executed")
            .Register(*registry),

        .db_duration = prometheus::BuildHistogram()
            .Name("syswatch_db_query_duration_seconds")
            .Help("Database query duration in seconds")
            .Register(*registry),

        .db_connections = prometheus::BuildGauge()
            .Name("syswatch_db_connections_active")
            .Help("Active database connections")
            .Register(*registry),
    };
}

}  // namespace syswatch::metrics
```

L'utilisation dans le code applicatif :

```cpp
#include "metrics.h"

// metrics est un membre de classe ou une variable globale initialisée au démarrage
void Server::handle_request(const Request& req) {
    auto start = std::chrono::steady_clock::now();

    auto response = process(req);

    double duration = std::chrono::duration<double>(
        std::chrono::steady_clock::now() - start
    ).count();

    // Instrumentation — deux lignes, coût négligeable
    metrics_.http_requests
        .Add({{"method", req.method()}, {"status", std::to_string(response.status())}})
        .Increment();
    metrics_.http_duration
        .Add({{"method", req.method()}},
             prometheus::Histogram::BucketBoundaries{0.001, 0.005, 0.01, 0.025, 0.05, 0.1, 0.25, 0.5, 1.0, 5.0})
        .Observe(duration);
}
```

Ce pattern offre une vue centralisée de toutes les métriques du projet (facile à auditer), des noms cohérents avec un préfixe commun (`syswatch_`), et une séparation nette entre la définition des métriques et leur utilisation.

---

## Conventions de nommage Prometheus

Les noms de métriques suivent des conventions strictes qui facilitent les requêtes PromQL et les dashboards standardisés.

**Préfixe par application.** Toutes les métriques d'une application partagent un préfixe : `syswatch_http_requests_total`, `syswatch_db_queries_total`. Cela évite les collisions avec les métriques d'autres applications dans le même Prometheus.

**Suffixe par unité.** Les métriques avec une unité physique incluent cette unité en suffixe : `_seconds` pour les durées, `_bytes` pour les tailles, `_ratio` pour les proportions (0.0 à 1.0). Cela élimine toute ambiguïté ("cette latence est en millisecondes ou en secondes ?"). Prometheus recommande les unités de base (secondes, octets) plutôt que les multiples (millisecondes, mégaoctets).

**Suffixe `_total` pour les counters.** Par convention, les noms de counters se terminent par `_total` : `http_requests_total`, `errors_total`. Cela les distingue immédiatement des gauges.

**Snake_case exclusivement.** Les noms de métriques et de labels utilisent le snake_case : `http_request_duration_seconds`, pas `httpRequestDurationSeconds`.

---

## Métriques de processus

prometheus-cpp peut exposer automatiquement des métriques sur le processus lui-même — utilisation CPU, mémoire résidente, nombre de threads, descripteurs de fichiers ouverts — sans code applicatif :

```cpp
#include <prometheus/exposer.h>
#include <prometheus/registry.h>

prometheus::Exposer exposer{"0.0.0.0:9090"};  
auto registry = std::make_shared<prometheus::Registry>();  

exposer.RegisterCollectable(registry);

// Les métriques de processus sont exposées automatiquement par l'exposeur
// process_cpu_seconds_total
// process_resident_memory_bytes
// process_virtual_memory_bytes
// process_open_fds
// process_max_fds
// process_start_time_seconds
// process_threads
```

Ces métriques sont précieuses pour le monitoring de base : une fuite mémoire apparaît comme une croissance continue de `process_resident_memory_bytes`, une fuite de descripteurs de fichiers apparaît dans `process_open_fds`, et une charge CPU anormale se lit dans `rate(process_cpu_seconds_total[5m])`.

---

## Port d'exposition : séparation du trafic

Un point d'architecture important : le endpoint `/metrics` doit être exposé sur un **port séparé** du port applicatif principal. Si votre application écoute sur le port 8080 pour les requêtes client, les métriques Prometheus sont exposées sur un port dédié (9090, 9100, ou tout autre port conventionnel).

Cette séparation a trois justifications. Premièrement, la sécurité : le endpoint `/metrics` expose des informations internes sur l'application (volume de trafic, latences, erreurs, utilisation mémoire) qui ne doivent pas être accessibles aux clients externes. Un port séparé peut être restreint au réseau interne par les règles de firewall. Deuxièmement, la disponibilité : si l'application est surchargée et ne répond plus aux requêtes client, le port de métriques (géré par un thread séparé dans prometheus-cpp) continue de répondre — ce qui permet à Prometheus de détecter le problème. Troisièmement, la convention Kubernetes : les ServiceMonitor (l'objet Kubernetes qui configure le scraping Prometheus) ciblent un port et un chemin spécifiques.

---

## Intégration avec Prometheus et Grafana

### Configuration du scraping Prometheus

Dans le fichier de configuration Prometheus (`prometheus.yml`), ajoutez votre application comme cible de scraping :

```yaml
scrape_configs:
  - job_name: 'syswatch'
    scrape_interval: 15s
    static_configs:
      - targets: ['syswatch-host:9090']
        labels:
          environment: 'production'
          team: 'sre'
```

`scrape_interval` définit la fréquence de collecte. 15 secondes est le défaut recommandé — assez fréquent pour détecter les problèmes rapidement, assez espacé pour ne pas surcharger ni l'application ni Prometheus.

### Service discovery Kubernetes

En environnement Kubernetes, le scraping est configuré automatiquement via les annotations du pod ou un `ServiceMonitor` :

```yaml
# Annotations sur le pod (approche simple)
metadata:
  annotations:
    prometheus.io/scrape: "true"
    prometheus.io/port: "9090"
    prometheus.io/path: "/metrics"
```

```yaml
# ServiceMonitor (approche recommandée avec l'opérateur Prometheus)
apiVersion: monitoring.coreos.com/v1  
kind: ServiceMonitor  
metadata:  
  name: syswatch
spec:
  selector:
    matchLabels:
      app: syswatch
  endpoints:
    - port: metrics
      interval: 15s
```

### Requêtes PromQL essentielles

Une fois les métriques collectées, PromQL permet de les interroger. Voici les requêtes les plus utiles pour une application C++ :

```promql
# Taux de requêtes par seconde (sur 5 minutes)
rate(syswatch_http_requests_total[5m])

# Taux d'erreurs (ratio erreurs / total)
sum(rate(syswatch_http_requests_total{status=~"5.."}[5m]))
/
sum(rate(syswatch_http_requests_total[5m]))

# Latence P99 (99e percentile)
histogram_quantile(0.99,
  sum(rate(syswatch_http_request_duration_seconds_bucket[5m])) by (le)
)

# Utilisation mémoire
syswatch_memory_usage_bytes / 1024 / 1024  # en Mo

# Connexions DB actives
syswatch_db_connections_active
```

### Alertes

Les métriques Prometheus alimentent des règles d'alerte. Voici des exemples pertinents pour une application C++ :

```yaml
# prometheus-rules.yml
groups:
  - name: syswatch
    rules:
      - alert: HighErrorRate
        expr: |
          sum(rate(syswatch_http_requests_total{status=~"5.."}[5m]))
          / sum(rate(syswatch_http_requests_total[5m]))
          > 0.05
        for: 5m
        labels:
          severity: critical
        annotations:
          summary: "Taux d'erreur HTTP supérieur à 5%"

      - alert: HighLatency
        expr: |
          histogram_quantile(0.99,
            sum(rate(syswatch_http_request_duration_seconds_bucket[5m])) by (le)
          ) > 1.0
        for: 5m
        labels:
          severity: warning
        annotations:
          summary: "Latence P99 supérieure à 1 seconde"

      - alert: MemoryLeak
        expr: |
          deriv(process_resident_memory_bytes[1h]) > 10 * 1024 * 1024
        for: 30m
        labels:
          severity: warning
        annotations:
          summary: "Croissance mémoire suspecte (>10 Mo/h depuis 30 min)"
```

L'alerte `MemoryLeak` illustre la puissance des métriques de processus : la fonction `deriv()` calcule le taux de variation de la mémoire résidente. Une croissance linéaire continue est le signal classique d'une fuite mémoire en C++ — détectable bien avant que le processus ne soit tué par l'OOM killer.

---

## Impact sur les performances

L'instrumentation Prometheus a un coût — mais il est conçu pour être négligeable.

**Incrémentation d'un counter ou observation d'un histogram** : une opération atomique, quelques nanosecondes. Le coût est comparable à un `std::atomic::fetch_add`. Même à 100 000 requêtes par seconde, l'instrumentation ajoute moins d'une milliseconde par seconde de temps CPU total.

**Scrape du endpoint `/metrics`** : la sérialisation de toutes les métriques en texte. Le coût dépend du nombre de séries temporelles (combinaisons métriques × labels). Pour une application typique exposant quelques dizaines de métriques avec quelques centaines de combinaisons de labels, le scrape prend moins d'une milliseconde. Si vous atteignez des milliers de séries, profilez le scrape et réduisez la cardinalité des labels.

**Mémoire** : chaque série temporelle consomme quelques centaines d'octets dans le registry. L'exposeur HTTP ajoute un thread et un socket. L'empreinte totale est typiquement inférieure à 1 Mo pour une application avec une instrumentation riche.

L'erreur la plus courante est la **cardinalité excessive** : un label dont les valeurs sont illimitées (user ID, session ID, IP source) crée une série temporelle par valeur unique. Mille utilisateurs distincts × 5 métriques = 5000 séries. Un million de requêtes avec des IDs uniques = un million de séries et un Prometheus à genoux. Les labels doivent avoir un nombre fini et petit de valeurs possibles.

---

## Exemple complet : main.cpp instrumenté

Voici un programme complet qui combine spdlog (section 40.1) et Prometheus dans une architecture prête pour la production :

```cpp
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include <prometheus/counter.h>
#include <prometheus/gauge.h>
#include <prometheus/histogram.h>
#include <prometheus/exposer.h>
#include <prometheus/registry.h>

#include <chrono>
#include <thread>
#include <atomic>
#include <csignal>

static std::atomic<bool> running{true};

void signal_handler(int) { running = false; }

int main() {
    // --- Logging ---
    spdlog::set_pattern("[%Y-%m-%dT%H:%M:%S.%e%z] [%l] [%n] %v");
    spdlog::set_level(spdlog::level::info);

    // --- Métriques Prometheus ---
    prometheus::Exposer exposer{"0.0.0.0:9090"};
    auto registry = std::make_shared<prometheus::Registry>();
    exposer.RegisterCollectable(registry);

    auto& tick_counter = prometheus::BuildCounter()
        .Name("syswatch_ticks_total")
        .Help("Total monitoring ticks executed")
        .Register(*registry);
    auto& ticks = tick_counter.Add({});

    auto& mem_gauge = prometheus::BuildGauge()
        .Name("syswatch_memory_usage_bytes")
        .Help("Current memory usage in bytes")
        .Register(*registry);
    auto& memory = mem_gauge.Add({{"type", "resident"}});

    auto& collect_hist = prometheus::BuildHistogram()
        .Name("syswatch_collect_duration_seconds")
        .Help("Duration of metric collection cycle")
        .Register(*registry);
    auto& collect_duration = collect_hist.Add(
        {}, prometheus::Histogram::BucketBoundaries{
            0.0001, 0.0005, 0.001, 0.005, 0.01, 0.05, 0.1
        }
    );

    spdlog::info("Syswatch started, metrics on :9090/metrics");

    // --- Boucle principale ---
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

    while (running) {
        auto start = std::chrono::steady_clock::now();

        // Simuler la collecte de métriques système
        memory.Set(42 * 1024 * 1024);  // Remplacer par la vraie valeur
        ticks.Increment();

        double elapsed = std::chrono::duration<double>(
            std::chrono::steady_clock::now() - start
        ).count();
        collect_duration.Observe(elapsed);

        spdlog::debug("Tick completed: collect_duration={:.6f}s", elapsed);
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    spdlog::info("Syswatch stopped gracefully");
    return 0;
}
```

Ce programme expose trois métriques sur `http://localhost:9090/metrics`, logue son activité via spdlog, et s'arrête proprement sur SIGINT/SIGTERM. C'est le squelette d'un agent de monitoring C++ prêt à être déployé dans un conteneur Kubernetes avec un ServiceMonitor.

---

## Résumé

Prometheus fournit le deuxième pilier de l'observabilité : les métriques numériques agrégées dans le temps. Le client prometheus-cpp s'intègre dans un projet CMake via Conan, vcpkg ou FetchContent, expose un endpoint HTTP thread-safe sur un port dédié, et offre les quatre types de métriques (Counter, Gauge, Histogram, Summary) avec un coût d'instrumentation de l'ordre de quelques nanosecondes par opération.

Les conventions de nommage (préfixe applicatif, suffixe d'unité, `_total` pour les counters) et la vigilance sur la cardinalité des labels sont les deux points qui séparent une instrumentation exploitable d'une instrumentation chaotique. Combinées aux logs spdlog (section 40.1) et aux traces OpenTelemetry (section 40.3), les métriques Prometheus complètent le tableau de l'observabilité d'une application C++ en production.

⏭️ [Tracing distribué (OpenTelemetry C++)](/40-observabilite/03-opentelemetry.md)
