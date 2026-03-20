🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 40.3 — Tracing distribué (OpenTelemetry C++)

## Suivre une requête à travers les composants

---

## Introduction

Les logs disent *ce qui s'est passé* dans un composant. Les métriques disent *comment ça se passe* globalement. Mais quand une requête traverse cinq services avant de retourner une réponse au client, ni les logs ni les métriques ne répondent facilement à la question : "où est passé le temps ?" et "quel composant a échoué ?".

Le **tracing distribué** comble ce vide. Il suit le parcours d'une requête de bout en bout — du point d'entrée au service final, à travers les appels réseau, les requêtes de base de données, les files d'attente — en enregistrant chaque étape avec sa durée, son statut, et ses métadonnées. Le résultat est une vue en cascade (souvent appelée *waterfall*) qui décompose visuellement le traitement d'une requête.

**OpenTelemetry** (OTel) est le standard en cours de consolidation pour l'observabilité. Né de la fusion d'OpenTracing et OpenCensus en 2019, il est aujourd'hui le projet le plus actif de la Cloud Native Computing Foundation. OpenTelemetry couvre les trois piliers — traces, métriques, et logs — avec une API unifiée et des SDKs dans la plupart des langages, dont le C++.

Cette section se concentre sur le **tracing** avec le SDK OpenTelemetry C++. Les métriques OTel existent mais prometheus-cpp (section 40.2) reste le choix dominant pour l'instrumentation C++ en 2026. L'intérêt d'OTel pour le C++ réside principalement dans le tracing et dans la propagation de contexte qui connecte les traces aux logs.

---

## Concepts fondamentaux

Avant de toucher au code, il faut comprendre quatre concepts qui structurent toute trace distribuée.

### Trace

Une **trace** représente le parcours complet d'une requête à travers le système. Elle est identifiée par un **Trace ID** unique (128 bits, représenté en hexadécimal : `4bf92f3577b34da6a3ce929d0e0e4736`). Tous les composants qui participent au traitement de cette requête partagent le même Trace ID.

### Span

Un **span** représente une unité de travail au sein d'une trace : un appel HTTP, une requête SQL, un traitement de fichier, une étape de calcul. Chaque span a un nom, un Span ID unique, une heure de début, une durée, un statut (OK, Error), et des attributs (paires clé-valeur). Les spans sont hiérarchiques : un span peut avoir un parent, formant un arbre qui reflète l'imbrication des appels.

Prenons un exemple concret. Un client envoie une requête à un service API. Ce service interroge une base de données et un cache. La trace contient trois spans :

```
Trace: 4bf92f3577b34da6a3ce929d0e0e4736

[API] handle_request (parent)       ├──── 45ms ────┤
  [DB] execute_query (enfant)          ├── 30ms ──┤
  [Cache] lookup (enfant)           ├─ 2ms ─┤
```

Le span parent `handle_request` englobe les spans enfants `execute_query` et `lookup`. La vue en waterfall montre immédiatement que la requête prend 45ms, dont 30ms sont passées dans la base de données — c'est là qu'il faut optimiser.

### Context Propagation

Quand un service appelle un autre service via HTTP ou gRPC, le Trace ID et le Span ID du parent doivent être transmis au service appelé pour que celui-ci puisse rattacher ses propres spans à la même trace. Cette transmission s'appelle la **propagation de contexte**.

Le standard de propagation est **W3C Trace Context**, qui utilise un header HTTP `traceparent` :

```
traceparent: 00-4bf92f3577b34da6a3ce929d0e0e4736-00f067aa0ba902b7-01
              │  │                                  │                │
              │  Trace ID (128 bits)                Span ID (64 bits) Flags
              Version
```

Le service appelé extrait ce header, crée un nouveau span enfant avec le même Trace ID et le Span ID reçu comme parent, et peut à son tour propager le contexte aux services qu'il appelle.

### Exporter

Les spans collectés dans l'application doivent être envoyés à un **backend** qui les stocke, les indexe, et les visualise. Les backends courants sont Jaeger, Zipkin, Grafana Tempo, et Datadog. L'**exporteur** est le composant du SDK qui sérialise les spans et les envoie au backend, typiquement via le protocole OTLP (OpenTelemetry Protocol) sur HTTP ou gRPC.

En pratique, les spans sont rarement envoyés directement au backend. L'architecture recommandée interpose un **OpenTelemetry Collector** — un agent intermédiaire qui reçoit les spans de l'application, les traite (filtrage, échantillonnage, enrichissement), et les transmet au backend final. Cette architecture découple l'application du backend et permet de changer de backend sans modifier le code.

---

## Le SDK OpenTelemetry C++

Le SDK OpenTelemetry C++ (`opentelemetry-cpp`) fournit l'API de tracing, les exporteurs, et les propagateurs. C'est un projet en développement actif, avec une API stable depuis la version 1.x.

### Installation via CMake FetchContent

```cmake
include(FetchContent)

FetchContent_Declare(
    opentelemetry-cpp
    GIT_REPOSITORY https://github.com/open-telemetry/opentelemetry-cpp.git
    GIT_TAG        v1.17.0
)

set(WITH_OTLP_HTTP ON CACHE BOOL "" FORCE)  
set(WITH_OTLP_GRPC OFF CACHE BOOL "" FORCE)  
set(BUILD_TESTING OFF CACHE BOOL "" FORCE)  

FetchContent_MakeAvailable(opentelemetry-cpp)

target_link_libraries(syswatch PRIVATE
    opentelemetry_api
    opentelemetry_sdk
    opentelemetry_exporter_otlp_http
    opentelemetry_resources
)
```

L'option `WITH_OTLP_HTTP` active l'exporteur OTLP sur HTTP — le plus simple à configurer. L'alternative gRPC (`WITH_OTLP_GRPC`) est plus performante mais ajoute une dépendance lourde (le stack gRPC complet).

### Installation via Conan

```python
def requirements(self):
    self.requires("opentelemetry-cpp/1.17.0")
```

```cmake
find_package(opentelemetry-cpp REQUIRED)

target_link_libraries(syswatch PRIVATE
    opentelemetry-cpp::api
    opentelemetry-cpp::sdk
    opentelemetry-cpp::otlp_http_exporter
)
```

### Installation via vcpkg

```bash
vcpkg install opentelemetry-cpp[otlp-http]
```

---

## Initialisation du tracing

Le SDK OpenTelemetry suit un pattern d'initialisation en trois étapes : configurer un exporteur, créer un TracerProvider, et l'enregistrer globalement.

```cpp
#include <opentelemetry/exporters/otlp/otlp_http_exporter_factory.h>
#include <opentelemetry/exporters/otlp/otlp_http_exporter_options.h>
#include <opentelemetry/sdk/trace/simple_processor.h>
#include <opentelemetry/sdk/trace/batch_span_processor_factory.h>
#include <opentelemetry/sdk/trace/batch_span_processor_options.h>
#include <opentelemetry/sdk/trace/tracer_provider_factory.h>
#include <opentelemetry/sdk/resource/resource.h>
#include <opentelemetry/trace/provider.h>

namespace trace_api = opentelemetry::trace;  
namespace trace_sdk = opentelemetry::sdk::trace;  
namespace otlp      = opentelemetry::exporter::otlp;  
namespace resource  = opentelemetry::sdk::resource;  

void init_tracing() {
    // 1. Configurer l'exporteur OTLP HTTP
    otlp::OtlpHttpExporterOptions exporter_opts;
    exporter_opts.url = "http://localhost:4318/v1/traces";  // OTel Collector
    auto exporter = otlp::OtlpHttpExporterFactory::Create(exporter_opts);

    // 2. Configurer le processeur de spans (batch pour la production)
    trace_sdk::BatchSpanProcessorOptions processor_opts;
    processor_opts.max_queue_size = 2048;
    processor_opts.schedule_delay_millis = std::chrono::milliseconds(5000);
    processor_opts.max_export_batch_size = 512;
    auto processor = trace_sdk::BatchSpanProcessorFactory::Create(
        std::move(exporter), processor_opts
    );

    // 3. Définir les attributs de la ressource (identité du service)
    auto resource_attrs = resource::Resource::Create({
        {"service.name", "syswatch"},
        {"service.version", "1.2.0"},
        {"deployment.environment", "production"}
    });

    // 4. Créer et enregistrer le TracerProvider
    auto provider = trace_sdk::TracerProviderFactory::Create(
        std::move(processor), resource_attrs
    );
    trace_api::Provider::SetTracerProvider(std::move(provider));
}

void shutdown_tracing() {
    auto provider = trace_api::Provider::GetTracerProvider();
    if (auto* sdk_provider = dynamic_cast<trace_sdk::TracerProvider*>(provider.get())) {
        sdk_provider->Shutdown();
    }
}
```

Détaillons les éléments clés de cette initialisation.

**L'exporteur OTLP HTTP** envoie les spans au format OTLP via HTTP POST. L'URL `http://localhost:4318/v1/traces` est le endpoint standard d'un OpenTelemetry Collector local. En production, cette URL pointe vers le Collector déployé comme sidecar ou comme service dédié.

**Le BatchSpanProcessor** accumule les spans en mémoire et les envoie par lots à l'exporteur, plutôt qu'un par un. C'est indispensable en production : envoyer un span individuellement à chaque création ajouterait une requête HTTP par span, ce qui serait catastrophique pour les performances. Le paramètre `schedule_delay_millis` contrôle l'intervalle maximum entre deux envois (5 secondes par défaut). `max_export_batch_size` limite la taille de chaque lot.

Pour le développement et le débogage, le `SimpleSpanProcessor` envoie chaque span immédiatement — utile pour voir les traces en temps réel, mais inadapté à la production :

```cpp
// Développement uniquement
auto processor = std::make_unique<trace_sdk::SimpleSpanProcessor>(
    std::move(exporter)
);
```

**La ressource** identifie le service qui produit les traces. `service.name` est l'attribut le plus important — c'est le nom affiché dans Jaeger, Tempo, ou Datadog pour identifier votre application parmi toutes les sources de traces.

---

## Créer des spans

Une fois le TracerProvider enregistré, la création de spans est simple :

```cpp
#include <opentelemetry/trace/provider.h>
#include <opentelemetry/trace/scope.h>
#include <opentelemetry/trace/span.h>

namespace trace_api = opentelemetry::trace;

void handle_request(const Request& req) {
    // Obtenir un tracer (nommé par composant)
    auto tracer = trace_api::Provider::GetTracerProvider()
        ->GetTracer("syswatch.http", "1.2.0");

    // Créer un span pour l'opération
    auto span = tracer->StartSpan("handle_request");

    // Activer le span dans le contexte courant (méthode statique)
    auto scope = trace_api::Tracer::WithActiveSpan(span);

    // Ajouter des attributs
    span->SetAttribute("http.method", req.method());
    span->SetAttribute("http.url", req.path());
    span->SetAttribute("http.client_ip", req.remote_addr());

    // ... traitement de la requête ...
    auto response = process(req);

    // Ajouter le résultat
    span->SetAttribute("http.status_code", response.status());

    if (response.status() >= 500) {
        span->SetStatus(trace_api::StatusCode::kError, "Server error");
    }

    // Le span se termine automatiquement quand il sort du scope
}
```

Plusieurs points méritent attention.

**Le tracer** est obtenu via le TracerProvider global. Le nom (`"syswatch.http"`) identifie la librairie d'instrumentation — c'est une convention OTel, pas un nom de logger. La version correspond à la version de votre code d'instrumentation.

**`WithActiveSpan`** attache le span au contexte courant du thread. Cela a deux effets : tout span créé ensuite dans ce thread sera automatiquement un enfant de ce span, et la propagation de contexte (voir plus bas) pourra extraire le Trace ID et le Span ID courants.

**Le span se termine automatiquement** quand l'objet `span` est détruit (RAII). Vous pouvez aussi le terminer explicitement avec `span->End()`. La durée du span est calculée entre `StartSpan` et `End` (ou la destruction).

**Les attributs** suivent les conventions sémantiques OpenTelemetry. Les attributs HTTP ont des noms standardisés (`http.method`, `http.status_code`, `http.url`, etc.) que les backends de tracing reconnaissent et utilisent pour le filtrage et l'affichage. Utiliser ces noms standards plutôt que des noms ad hoc permet une expérience cohérente entre les services instrumentés dans différents langages.

---

## Hiérarchie parent-enfant

La puissance du tracing vient de la hiérarchie. Quand un span est actif dans le contexte courant, tout nouveau span créé dans le même thread devient automatiquement son enfant :

```cpp
void handle_request(const Request& req) {
    auto tracer = trace_api::Provider::GetTracerProvider()
        ->GetTracer("syswatch.http");

    auto span = tracer->StartSpan("handle_request");
    auto scope = trace_api::Tracer::WithActiveSpan(span);

    span->SetAttribute("http.method", req.method());

    // Appels internes — chacun crée un span enfant automatiquement
    auto config = load_config();
    auto data = query_database(req.params());
    auto result = compute_response(data);

    span->SetAttribute("http.status_code", 200);
}

Config load_config() {
    auto tracer = trace_api::Provider::GetTracerProvider()
        ->GetTracer("syswatch.config");

    // Ce span est automatiquement enfant de handle_request
    auto span = tracer->StartSpan("load_config");
    auto scope = trace_api::Tracer::WithActiveSpan(span);

    // ...
    return config;
}

QueryResult query_database(const Params& params) {
    auto tracer = trace_api::Provider::GetTracerProvider()
        ->GetTracer("syswatch.db");

    auto span = tracer->StartSpan("db.query");
    auto scope = trace_api::Tracer::WithActiveSpan(span);

    span->SetAttribute("db.system", "postgresql");
    span->SetAttribute("db.statement", params.query());

    auto start = std::chrono::steady_clock::now();
    auto result = db_->execute(params.query());

    // Enregistrer un événement ponctuel dans le span
    span->AddEvent("query_executed", {
        {"db.rows_affected", static_cast<int64_t>(result.row_count())}
    });

    return result;
}
```

La trace résultante, visualisée dans Jaeger ou Tempo, montre la cascade :

```
[handle_request]          ├──────────── 48ms ────────────┤
  [load_config]           ├─ 3ms ─┤
  [db.query]                       ├────── 32ms ──────┤
  [compute_response]                                    ├─ 8ms ─┤
```

Sans écrire une seule ligne de code de mesure explicite, vous savez que 32ms sur 48ms sont passées dans la base de données. C'est la valeur fondamentale du tracing.

### Événements (Span Events)

`AddEvent` enregistre un événement ponctuel *au sein* d'un span — une occurrence notable qui ne mérite pas son propre span. Cas d'usage typiques : "cache miss", "retry attempt", "connection established", "rows returned". Les événements sont horodatés et peuvent porter des attributs.

---

## Propagation de contexte entre services

Dans un système distribué, le tracing n'a de valeur que si les spans de différents services sont connectés dans la même trace. La propagation de contexte est le mécanisme qui assure cette connexion.

### Côté client : injecter le contexte

Quand votre application appelle un autre service via HTTP, elle doit injecter le contexte de trace dans les headers de la requête sortante :

```cpp
#include <opentelemetry/context/propagation/global_propagator.h>
#include <opentelemetry/context/propagation/text_map_propagator.h>

// Adaptateur pour injecter les headers dans une requête HTTP
class HttpHeaderSetter
    : public opentelemetry::context::propagation::TextMapCarrier {
public:
    explicit HttpHeaderSetter(std::map<std::string, std::string>& headers)
        : headers_(headers) {}

    void Set(opentelemetry::nostd::string_view key,
             opentelemetry::nostd::string_view value) noexcept override {
        headers_[std::string(key)] = std::string(value);
    }

    // Get et Keys ne sont pas nécessaires pour l'injection
    opentelemetry::nostd::string_view Get(
        opentelemetry::nostd::string_view) const noexcept override { return ""; }
    bool Keys(std::function<bool(opentelemetry::nostd::string_view)>)
        const noexcept override { return true; }

private:
    std::map<std::string, std::string>& headers_;
};

void call_downstream_service(const std::string& url) {
    auto tracer = trace_api::Provider::GetTracerProvider()
        ->GetTracer("syswatch.http.client");

    auto span = tracer->StartSpan("http.client");
    auto scope = trace_api::Tracer::WithActiveSpan(span);

    span->SetAttribute("http.method", "GET");
    span->SetAttribute("http.url", url);

    // Injecter le contexte de trace dans les headers
    std::map<std::string, std::string> headers;
    HttpHeaderSetter setter(headers);
    auto propagator = opentelemetry::context::propagation::
        GlobalTextMapPropagator::GetGlobalPropagator();
    propagator->Inject(setter, opentelemetry::context::RuntimeContext::GetCurrent());

    // headers contient maintenant "traceparent: 00-<trace_id>-<span_id>-01"
    // Passer ces headers à votre client HTTP (cpr, cpp-httplib, Asio, etc.)
    auto response = http_client.Get(url, headers);

    span->SetAttribute("http.status_code", response.status);
}
```

### Côté serveur : extraire le contexte

Le service appelé extrait le contexte des headers de la requête entrante et crée un span enfant :

```cpp
class HttpHeaderGetter
    : public opentelemetry::context::propagation::TextMapCarrier {
public:
    explicit HttpHeaderGetter(const std::map<std::string, std::string>& headers)
        : headers_(headers) {}

    opentelemetry::nostd::string_view Get(
        opentelemetry::nostd::string_view key) const noexcept override {
        auto it = headers_.find(std::string(key));
        return it != headers_.end() ? it->second : "";
    }

    bool Keys(std::function<bool(opentelemetry::nostd::string_view)> callback)
        const noexcept override {
        for (const auto& [k, v] : headers_) {
            if (!callback(k)) return false;
        }
        return true;
    }

    void Set(opentelemetry::nostd::string_view,
             opentelemetry::nostd::string_view) noexcept override {}

private:
    const std::map<std::string, std::string>& headers_;
};

void on_incoming_request(const Request& req) {
    // Extraire le contexte de trace des headers entrants
    HttpHeaderGetter getter(req.headers());
    auto propagator = opentelemetry::context::propagation::
        GlobalTextMapPropagator::GetGlobalPropagator();
    auto ctx = propagator->Extract(getter, opentelemetry::context::RuntimeContext::GetCurrent());

    // Créer un span enfant dans le contexte extrait
    opentelemetry::trace::StartSpanOptions opts;
    opts.parent = trace_api::GetSpan(ctx)->GetContext();

    auto tracer = trace_api::Provider::GetTracerProvider()
        ->GetTracer("downstream.http");

    auto span = tracer->StartSpan("handle_request", {}, opts);
    auto scope = trace_api::Tracer::WithActiveSpan(span);

    // Ce span partage le même Trace ID que le service appelant
    // ...
}
```

La configuration du propagateur W3C Trace Context se fait à l'initialisation :

```cpp
#include <opentelemetry/context/propagation/global_propagator.h>
#include <opentelemetry/trace/propagation/http_trace_context.h>

void init_propagation() {
    opentelemetry::context::propagation::GlobalTextMapPropagator::
        SetGlobalPropagator(
            std::make_shared<opentelemetry::trace::propagation::HttpTraceContext>()
        );
}
```

---

## Corrélation traces-logs

La valeur du tracing augmente considérablement quand les traces sont corrélées avec les logs. Le principe est simple : inclure le Trace ID et le Span ID courants dans chaque message de log. Quand un opérateur trouve un log d'erreur, il peut cliquer sur le Trace ID pour voir la trace complète dans Jaeger ou Tempo. Inversement, quand il examine une trace lente, il peut filtrer les logs par Trace ID pour voir les détails.

La section 40.1.3 a présenté le pattern de flags personnalisés spdlog avec thread-local. Le même mécanisme s'applique pour le Trace ID :

```cpp
#include <opentelemetry/trace/provider.h>
#include <opentelemetry/trace/span.h>
#include <spdlog/pattern_formatter.h>

class trace_id_flag : public spdlog::custom_flag_formatter {  
public:  
    void format(const spdlog::details::log_msg&,
                const std::tm&,
                spdlog::memory_buf_t& dest) override {
        auto span = trace_api::Tracer::GetCurrentSpan();

        if (span && span->GetContext().IsValid()) {
            char buf[33];
            span->GetContext().trace_id().ToLowerBase16(buf);
            std::string trace_id(buf, 32);
            dest.append(trace_id.data(), trace_id.data() + trace_id.size());
        } else {
            const char* na = "--------------------------------";
            dest.append(na, na + 32);
        }
    }

    std::unique_ptr<custom_flag_formatter> clone() const override {
        return std::make_unique<trace_id_flag>();
    }
};

// Enregistrement dans le pattern spdlog (flag %*)
auto formatter = std::make_unique<spdlog::pattern_formatter>();  
formatter->add_flag<trace_id_flag>('*');  
formatter->set_pattern("[%Y-%m-%dT%H:%M:%S.%e] [%l] [trace:%*] %v");  
logger->set_formatter(std::move(formatter));  
```

Résultat :

```
[2026-03-18T14:23:07.412] [info] [trace:4bf92f3577b34da6a3ce929d0e0e4736] Request received: GET /api/metrics
[2026-03-18T14:23:07.415] [debug] [trace:4bf92f3577b34da6a3ce929d0e0e4736] Cache miss for key=metrics_5m
[2026-03-18T14:23:07.442] [info] [trace:4bf92f3577b34da6a3ce929d0e0e4736] Request completed: status=200 duration=30ms
```

Grafana Loki et Tempo exploitent cette corrélation nativement : un clic sur un Trace ID dans les logs ouvre la trace correspondante, et vice versa.

---

## Échantillonnage

En production, tracer 100% des requêtes génère un volume de données considérable et un coût de stockage élevé. L'**échantillonnage** réduit ce volume en ne conservant qu'une fraction des traces.

### Échantillonnage à taux fixe

```cpp
#include <opentelemetry/sdk/trace/samplers/trace_id_ratio_based.h>

// Échantillonner 10% des traces
auto sampler = std::make_unique<trace_sdk::TraceIdRatioBasedSampler>(0.1);

auto provider = trace_sdk::TracerProviderFactory::Create(
    std::move(processor), resource_attrs, std::move(sampler)
);
```

L'échantillonnage basé sur le Trace ID est déterministe : un même Trace ID est toujours échantillonné ou toujours rejeté, quel que soit le service. Cela garantit que si un service décide de tracer une requête, tous les services en aval la tracent aussi — pas de trace partielle.

### Échantillonnage à la tête vs en queue

L'échantillonnage **à la tête** (head-based, ci-dessus) décide au début de la trace si elle sera conservée. C'est simple et efficace, mais il écarte des traces qui auraient pu être intéressantes (par exemple, une requête qui échoue mais qui tombe dans les 90% non échantillonnés).

L'échantillonnage **en queue** (tail-based) attend que la trace soit complète avant de décider. Il permet des règles comme "conserver toutes les traces en erreur" ou "conserver toutes les traces de plus de 1 seconde". Cet échantillonnage est réalisé par le Collector OpenTelemetry (pas par le SDK dans l'application), ce qui nécessite un Collector avec suffisamment de mémoire pour buffériser les traces en cours.

Un compromis courant : échantillonner 100% des traces en développement et sur les environnements de test, 10-20% en production avec un tail-based sampling sur le Collector qui conserve systématiquement les traces en erreur et les traces lentes.

---

## Architecture de déploiement

L'architecture recommandée en production interpose un OpenTelemetry Collector entre l'application et le backend de traces :

```
┌─────────────┐     OTLP/HTTP     ┌───────────────┐     OTLP/gRPC     ┌─────────┐
│ Application │ ───────────────── │  OTel Collector │ ─────────────── │  Jaeger  │
│   C++ SDK   │    :4318          │   (sidecar ou   │                  │  Tempo   │
│             │                   │    service)     │                  │  Datadog │
└─────────────┘                   └───────────────┘                  └─────────┘
```

Le Collector apporte plusieurs avantages. Le **découplage** : changer de backend (Jaeger → Tempo → Datadog) ne nécessite pas de modifier ni redéployer l'application. Le **buffering** : si le backend est temporairement indisponible, le Collector bufferise les données. Le **traitement** : le Collector peut filtrer, échantillonner (tail-based), enrichir, et router les traces vers plusieurs backends simultanément. Le **batching** : le Collector optimise les envois vers le backend en regroupant les spans de plusieurs applications.

En Kubernetes, le Collector est typiquement déployé comme **DaemonSet** (un Collector par nœud, recevant les traces de tous les pods du nœud) ou comme **sidecar** (un Collector par pod, pour un contrôle plus fin).

Un fichier de configuration Collector minimal :

```yaml
# otel-collector-config.yaml
receivers:
  otlp:
    protocols:
      http:
        endpoint: 0.0.0.0:4318

processors:
  batch:
    timeout: 5s
    send_batch_size: 1024

exporters:
  otlp:
    endpoint: "tempo.monitoring:4317"
    tls:
      insecure: true

service:
  pipelines:
    traces:
      receivers: [otlp]
      processors: [batch]
      exporters: [otlp]
```

---

## Quand le tracing distribué est-il pertinent ?

Le tracing distribué ajoute de la complexité : dépendance au SDK OTel, Collector à déployer et maintenir, backend de traces à opérer. Cette complexité n'est pas toujours justifiée.

**Le tracing est pertinent** quand votre système est composé de plusieurs services qui communiquent entre eux (microservices, SOA), quand les problèmes de latence sont difficiles à localiser car le traitement traverse plusieurs composants, quand vous avez besoin de comprendre les dépendances entre services et leur impact sur la latence globale, ou quand les logs seuls ne suffisent plus à reconstituer le flux d'une requête.

**Le tracing est superflu** quand votre application est un monolithe ou un outil CLI qui ne fait pas d'appels réseau à d'autres services, quand les problèmes de performance sont localisables avec le profiling classique (perf, chapitre 31), ou quand l'infrastructure de monitoring est minimale et qu'un Collector/backend représenterait un surcoût disproportionné.

Pour un monolithe C++ qui fait des requêtes à une base de données et un cache, le tracing *interne* (spans pour les appels DB et cache, sans propagation inter-services) peut être utile pour visualiser la décomposition des latences — mais les mêmes informations peuvent souvent être obtenues via des métriques Prometheus (histogrammes de latence par composant) avec moins de complexité.

---

## Résumé

Le tracing distribué complète les logs et les métriques en fournissant une vue transversale du traitement des requêtes à travers les services. OpenTelemetry fournit le SDK C++ pour créer des spans, propager le contexte entre services, et exporter les données vers un backend via un Collector.

Les concepts clés sont les spans hiérarchiques (parent-enfant), la propagation de contexte W3C Trace Context, l'échantillonnage pour contrôler le volume, et la corrélation avec les logs via le Trace ID. L'architecture recommandée interpose un Collector entre l'application et le backend pour le découplage et le buffering.

Le tracing est l'investissement le plus rentable pour les architectures multi-services. Pour les applications mono-service, les logs (section 40.1) et les métriques (section 40.2) couvrent la majorité des besoins d'observabilité. La section 40.4 aborde un mécanisme plus simple mais tout aussi critique en production : les health checks.

⏭️ [Health checks et readiness probes](/40-observabilite/04-health-checks.md)
