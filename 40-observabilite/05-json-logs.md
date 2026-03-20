🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 40.5 — Structured logging : JSON logs pour agrégation

## Des logs que les machines savent lire

---

## Introduction

La section 40.1 a mis en place spdlog pour produire des logs lisibles par les humains. La section 40.1.3 a présenté un pattern JSON basique via le système de formatage. Cette section va plus loin : elle traite le **logging structuré** comme une discipline à part entière — la passerelle entre les logs que vous écrivez dans votre code C++ et les systèmes d'agrégation (Elasticsearch, Grafana Loki, Datadog, Splunk) qui les rendent exploitables à l'échelle.

Le logging structuré change le modèle mental. Un log traditionnel est une phrase destinée à un humain : `"Connection to database lost after 3 retries"`. Un log structuré est un enregistrement destiné à une machine, qui contient accessoirement une phrase lisible par un humain :

```json
{"time":"2026-03-18T14:23:07.412Z","level":"error","logger":"database","msg":"Connection lost","host":"db-primary.internal","retries":3,"last_error":"connection refused","trace_id":"4bf92f3577b34da6a3ce929d0e0e4736","service":"syswatch","pid":8847}
```

La différence n'est pas cosmétique. Le log traditionnel nécessite une expression régulière fragile pour en extraire le nombre de retries ou le nom de l'hôte. Le log JSON est indexé champ par champ dès l'ingestion : filtrer par `host="db-primary.internal"` ou alerter quand `retries > 5` est une requête triviale dans n'importe quel agrégateur.

---

## Pourquoi le JSON s'est imposé

Plusieurs formats structurés existent (logfmt, GELF, CEF), mais JSON s'est imposé comme le standard de fait pour le logging structuré. Les raisons sont pragmatiques.

Tous les agrégateurs de logs parsent le JSON nativement — Elasticsearch, Loki, Datadog, CloudWatch, Splunk, Fluentd, Filebeat, Promtail. Aucune configuration de parsing supplémentaire n'est nécessaire. Le format est auto-descriptif : les noms de champs sont dans le message lui-même. La librairie `nlohmann/json` (chapitre 24) fait partie de la boîte à outils standard de tout projet C++. Et le format est extensible : ajouter un champ à un log JSON ne casse pas les parsers existants.

Le surcoût de taille par rapport à un format texte compact (comme logfmt : `level=error host=db-primary retries=3`) est réel mais généralement négligeable face au coût de transmission réseau et de stockage dans l'agrégateur. La compression (gzip, zstd) absorbe l'essentiel de la verbosité JSON.

---

## Le problème du pattern JSON de spdlog

La section 40.1.3 a présenté un pattern spdlog qui produit du JSON :

```cpp
sink->set_pattern(R"({"time":"%Y-%m-%dT%H:%M:%S.%e%z","level":"%l","logger":"%n","msg":"%v"})");
```

Ce pattern a une limitation fondamentale : le contenu du message (`%v`) est inséré verbatim dans la chaîne JSON. Si le message contient un guillemet double, un retour à la ligne, un backslash, ou tout autre caractère spécial JSON, le résultat est du JSON invalide :

```cpp
spdlog::error("Failed to parse file \"config.yaml\": unexpected token");
// Produit : {"msg":"Failed to parse file "config.yaml": unexpected token"}
// ← JSON cassé : les guillemets internes ne sont pas échappés
```

Pour du logging interne où vous contrôlez tous les messages et savez qu'ils ne contiennent pas de caractères spéciaux, le pattern simple fonctionne. Pour un logging de production robuste — où les messages peuvent contenir des noms de fichiers, des messages d'erreur système, des entrées utilisateur, ou des extraits de données — il faut une solution qui garantit un JSON valide en toutes circonstances.

Trois approches résolvent ce problème.

---

## Approche 1 : Custom sink avec nlohmann/json

La solution la plus robuste est de créer un sink spdlog personnalisé qui construit chaque message comme un objet JSON via `nlohmann/json`, garantissant l'échappement correct de tous les champs :

```cpp
#pragma once

#include <spdlog/sinks/base_sink.h>
#include <spdlog/details/log_msg.h>
#include <nlohmann/json.hpp>
#include <mutex>
#include <fstream>
#include <iostream>

template<typename Mutex>  
class json_sink : public spdlog::sinks::base_sink<Mutex> {  
public:  
    enum class Output { Stdout, Stderr, File };

    explicit json_sink(Output output = Output::Stdout,
                       const std::string& filepath = "")
        : output_(output) {
        if (output == Output::File && !filepath.empty()) {
            file_.open(filepath, std::ios::app);
        }
    }

    // Ajouter des champs statiques présents dans chaque message
    void add_static_field(const std::string& key, const std::string& value) {
        static_fields_[key] = value;
    }

protected:
    void sink_it_(const spdlog::details::log_msg& msg) override {
        nlohmann::json j;

        // Horodatage ISO 8601
        auto time = msg.time;
        auto epoch = time.time_since_epoch();
        auto seconds = std::chrono::duration_cast<std::chrono::seconds>(epoch);
        auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(epoch)
                      - std::chrono::duration_cast<std::chrono::milliseconds>(seconds);
        std::time_t t = std::chrono::system_clock::to_time_t(time);
        std::tm tm{};
        gmtime_r(&t, &tm);
        char time_buf[64];
        std::snprintf(time_buf, sizeof(time_buf),
                      "%04d-%02d-%02dT%02d:%02d:%02d.%03ldZ",
                      tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
                      tm.tm_hour, tm.tm_min, tm.tm_sec,
                      static_cast<long>(millis.count()));
        j["time"] = time_buf;

        // Niveau
        j["level"] = spdlog::level::to_string_view(msg.level).data();

        // Logger
        j["logger"] = std::string(msg.logger_name.data(), msg.logger_name.size());

        // Message (correctement échappé par nlohmann/json)
        j["msg"] = std::string(msg.payload.data(), msg.payload.size());

        // Thread ID et PID
        j["tid"] = msg.thread_id;
        j["pid"] = static_cast<int>(getpid());

        // Source location (si disponible, via les macros SPDLOG_*)
        if (!msg.source.empty()) {
            j["source"]["file"] = msg.source.filename;
            j["source"]["line"] = msg.source.line;
            j["source"]["func"] = msg.source.funcname;
        }

        // Champs statiques (service, version, environment, etc.)
        for (const auto& [key, value] : static_fields_) {
            j[key] = value;
        }

        // Sérialiser et écrire (une ligne JSON = un message)
        std::string line = j.dump(-1, ' ', false, nlohmann::json::error_handler_t::replace);
        line.push_back('\n');

        write_output(line);
    }

    void flush_() override {
        if (file_.is_open()) {
            file_.flush();
        }
    }

private:
    void write_output(const std::string& line) {
        switch (output_) {
            case Output::Stdout: std::cout << line; break;
            case Output::Stderr: std::cerr << line; break;
            case Output::File:   file_ << line; break;
        }
    }

    Output output_;
    std::ofstream file_;
    std::map<std::string, std::string> static_fields_;
};

// Alias pour multi-threaded et single-threaded
using json_sink_mt = json_sink<std::mutex>;  
using json_sink_st = json_sink<spdlog::details::null_mutex>;  
```

L'utilisation :

```cpp
auto sink = std::make_shared<json_sink_mt>();  
sink->add_static_field("service", "syswatch");  
sink->add_static_field("version", "1.2.0");  
sink->add_static_field("environment", "production");  

auto logger = std::make_shared<spdlog::logger>("syswatch", sink);  
spdlog::set_default_logger(logger);  

SPDLOG_ERROR("Failed to parse file \"config.yaml\": unexpected token at line {}", 42);
```

Résultat — du JSON garanti valide, quel que soit le contenu du message :

```json
{"time":"2026-03-18T14:23:07.412Z","level":"error","logger":"syswatch","msg":"Failed to parse file \"config.yaml\": unexpected token at line 42","tid":14352,"pid":8847,"source":{"file":"config.cpp","line":87,"func":"parse_config"},"service":"syswatch","version":"1.2.0","environment":"production"}
```

Les guillemets dans le message sont correctement échappés par `nlohmann::json::dump()`. Le paramètre `error_handler_t::replace` remplace les séquences UTF-8 invalides par le caractère de remplacement Unicode plutôt que de lancer une exception — une protection contre les données binaires accidentelles dans les messages.

---

## Approche 2 : Champs structurés dans le message

Le sink JSON ci-dessus produit des champs standardisés (time, level, logger, msg) mais le contenu du message reste une chaîne de texte libre. Pour un logging pleinement structuré, chaque donnée contextuelle devrait être un champ JSON distinct — pas une valeur interpolée dans une phrase.

Comparons les deux styles :

```cpp
// Style interpolé (semi-structuré)
spdlog::info("Request processed: method=GET path=/api/metrics status=200 duration=12ms");
// → {"msg": "Request processed: method=GET path=/api/metrics status=200 duration=12ms"}

// Style pleinement structuré
// → {"msg": "Request processed", "method": "GET", "path": "/api/metrics", "status": 200, "duration_ms": 12}
```

Le second style est supérieur pour l'agrégation : chaque champ est typé (status est un entier, duration est un nombre) et directement indexable. Mais spdlog n'a pas de mécanisme natif pour ajouter des champs structurés à un message individuel — son API est `spdlog::info(format_string, args...)`, qui produit une chaîne.

Deux stratégies permettent de contourner cette limitation.

### Stratégie A : Logger wrapper avec contexte

Encapsulez les appels spdlog dans une couche qui accumule les champs structurés et les passe au sink JSON :

```cpp
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

// Stockage thread-local pour les champs contextuels
inline thread_local nlohmann::json t_log_fields;

class StructuredLog {  
public:  
    explicit StructuredLog(spdlog::level::level_enum level)
        : level_(level) {}

    StructuredLog& field(const std::string& key, const nlohmann::json& value) {
        fields_[key] = value;
        return *this;
    }

    void msg(const std::string& message) {
        // Fusionner les champs thread-local et les champs de ce message
        nlohmann::json all_fields = t_log_fields;
        all_fields.merge_patch(fields_);
        all_fields["_msg"] = message;

        // Sérialiser en une seule chaîne passée à spdlog
        spdlog::default_logger()->log(level_, "{}", all_fields.dump());
    }

private:
    spdlog::level::level_enum level_;
    nlohmann::json fields_;
};

// Helpers
inline StructuredLog log_info()  { return StructuredLog(spdlog::level::info); }  
inline StructuredLog log_warn()  { return StructuredLog(spdlog::level::warn); }  
inline StructuredLog log_error() { return StructuredLog(spdlog::level::err); }  

// RAII pour le contexte thread-local
class LogScope {  
public:  
    LogScope(const std::string& key, const nlohmann::json& value) : key_(key) {
        t_log_fields[key_] = value;
    }
    ~LogScope() { t_log_fields.erase(key_); }
private:
    std::string key_;
};
```

L'utilisation est fluide :

```cpp
void handle_request(const Request& req) {
    // Champs contextuels pour toute la durée du traitement
    LogScope req_scope("request_id", req.id());
    LogScope user_scope("user_id", req.user());

    log_info()
        .field("method", req.method())
        .field("path", req.path())
        .msg("Request received");

    auto response = process(req);

    log_info()
        .field("method", req.method())
        .field("path", req.path())
        .field("status", response.status())
        .field("duration_ms", elapsed_ms)
        .msg("Request completed");
}
```

Le sink JSON (approche 1) reçoit une chaîne JSON dans `%v`. Pour éviter le double-encodage (JSON dans JSON), le sink peut détecter si le message est lui-même un objet JSON et le fusionner :

```cpp
void sink_it_(const spdlog::details::log_msg& msg) override {
    nlohmann::json j;
    j["time"] = format_time(msg.time);
    j["level"] = spdlog::level::to_string_view(msg.level).data();
    j["logger"] = std::string(msg.logger_name.data(), msg.logger_name.size());

    // Tenter de parser le message comme JSON pour fusionner les champs
    std::string payload(msg.payload.data(), msg.payload.size());
    try {
        auto parsed = nlohmann::json::parse(payload);
        if (parsed.is_object()) {
            // Extraire le message textuel et fusionner les champs
            if (parsed.contains("_msg")) {
                j["msg"] = parsed["_msg"];
                parsed.erase("_msg");
            }
            j.merge_patch(parsed);
        } else {
            j["msg"] = payload;
        }
    } catch (...) {
        // Pas du JSON — traiter comme message texte
        j["msg"] = payload;
    }

    // Champs statiques
    for (const auto& [key, value] : static_fields_) {
        j[key] = value;
    }

    write_output(j.dump() + "\n");
}
```

### Stratégie B : Paires clé-valeur dans le message

Une approche plus légère consiste à structurer les informations contextuelles comme des paires clé-valeur lisibles dans le message, et à laisser l'agrégateur les parser :

```cpp
spdlog::info("Request completed method={} path={} status={} duration_ms={}",
             req.method(), req.path(), response.status(), elapsed_ms);
```

Ce style, parfois appelé **logfmt**, produit :

```json
{"msg":"Request completed method=GET path=/api/metrics status=200 duration_ms=12"}
```

C'est moins structuré que du JSON pur — les champs sont dans le message, pas à la racine de l'objet — mais les agrégateurs comme Loki et Datadog savent extraire les paires `key=value` des messages textuels via des pipelines de parsing configurables. C'est le compromis pragmatique le plus courant : la discipline de nommage `key=value` dans les messages spdlog standard, combinée au sink JSON pour les métadonnées (time, level, logger, service), donne des logs exploitables sans la complexité d'un wrapper complet.

---

## Champs standards : que mettre dans chaque log

Indépendamment de l'approche technique, certains champs doivent être présents dans chaque message pour une exploitation efficace.

### Champs automatiques (gérés par le sink)

Ces champs sont ajoutés par le sink JSON sans intervention du développeur :

- **`time`** — Horodatage ISO 8601 avec millisecondes et timezone (`2026-03-18T14:23:07.412Z`). UTC est fortement recommandé pour les systèmes distribués.  
- **`level`** — Niveau de sévérité (`debug`, `info`, `warn`, `error`, `critical`).  
- **`logger`** — Nom du logger spdlog, identifiant le composant.  
- **`tid`** — Thread ID, essentiel pour reconstituer les flux dans les applications multi-thread.  
- **`pid`** — Process ID, utile pour distinguer les instances.  
- **`service`** — Nom du service (champ statique).  
- **`version`** — Version de l'application (champ statique).  
- **`environment`** — Environnement de déploiement : `production`, `staging`, `development` (champ statique).

### Champs contextuels (gérés par le code applicatif)

Ces champs sont ajoutés par le code au moment de l'émission du log :

- **`trace_id`** — Identifiant de trace OpenTelemetry (section 40.3), pour la corrélation traces-logs.  
- **`request_id`** — Identifiant unique de la requête en cours de traitement.  
- **`user_id`** — Identifiant de l'utilisateur (si applicable et conforme RGPD).  
- **`duration_ms`** — Durée de l'opération loguée.  
- **`error`** — Message d'erreur ou code d'erreur quand le log concerne un échec.

### Champs ajoutés par l'infrastructure

Les agrégateurs et les collecteurs ajoutent souvent des champs supplémentaires au moment de l'ingestion :

- **`hostname`** — Nom de la machine (ajouté par Filebeat, Fluentd, ou le Collector OTel).  
- **`container_id`** — Identifiant du conteneur Docker.  
- **`k8s.pod.name`**, **`k8s.namespace`** — Métadonnées Kubernetes.

Ces champs n'ont pas besoin d'être dans votre code — les laisser à l'infrastructure évite la duplication et garantit leur exactitude.

---

## Intégration avec les stacks d'agrégation

### Grafana Loki

Loki est l'agrégateur de logs le plus léger de l'écosystème Grafana. Contrairement à Elasticsearch, il n'indexe pas le contenu des messages — il indexe uniquement les labels (métadonnées) et fait une recherche plein texte à la demande. Cela le rend moins coûteux en stockage et en ressources.

L'intégration avec un service C++ loguant en JSON sur stdout suit le chemin suivant :

```
Application C++ → stdout (JSON) → Docker log driver → Promtail → Loki → Grafana
```

**Promtail** est l'agent de collecte de Loki. Il lit les logs des conteneurs Docker, les parse, et les envoie à Loki. La configuration pour parser du JSON :

```yaml
# promtail-config.yaml (extrait)
scrape_configs:
  - job_name: syswatch
    docker_sd_configs:
      - host: unix:///var/run/docker.sock
    pipeline_stages:
      - json:
          expressions:
            level: level
            logger: logger
            service: service
            trace_id: trace_id
      - labels:
          level:
          logger:
          service:
      - output:
          source: msg
```

Le stage `json` extrait les champs du JSON. Le stage `labels` transforme certains champs en labels Loki (indexés, utilisés pour le filtrage rapide). Le stage `output` définit quel champ constitue le message affiché dans Grafana.

Dans Grafana, les requêtes LogQL exploitent à la fois les labels et le contenu JSON :

```logql
{service="syswatch", level="error"} | json | duration_ms > 1000
```

Cette requête filtre les logs du service syswatch au niveau error, parse le JSON de chaque ligne, et ne conserve que ceux où `duration_ms` dépasse 1000.

### Elasticsearch (ELK Stack)

L'architecture classique ELK (Elasticsearch, Logstash, Kibana) ou son évolution EFK (Elasticsearch, Filebeat/Fluentd, Kibana) ingère les logs JSON et indexe chaque champ automatiquement.

Le chemin d'ingestion :

```
Application C++ → stdout (JSON) → Filebeat → Elasticsearch → Kibana
```

**Filebeat** est l'agent de collecte léger d'Elastic. La configuration pour les logs JSON Docker :

```yaml
# filebeat.yml (extrait)
filebeat.inputs:
  - type: container
    paths:
      - /var/lib/docker/containers/*/*.log
    processors:
      - decode_json_fields:
          fields: ["message"]
          target: ""
          overwrite_keys: true
```

Le processeur `decode_json_fields` parse le JSON et place chaque champ à la racine du document Elasticsearch. Dans Kibana, chaque champ (`service`, `level`, `duration_ms`, `trace_id`) est directement filtrable et agrégeable.

### Datadog

Datadog détecte et parse automatiquement les logs JSON sans configuration supplémentaire. Les champs standards sont reconnus par convention :

- `level` ou `status` → niveau de sévérité affiché  
- `msg` ou `message` → message principal  
- `dd.trace_id` → corrélation avec les traces Datadog APM

Si vous utilisez les noms de champs standards, les logs apparaissent correctement formatés dans l'interface Datadog dès l'ingestion.

---

## Performance du logging JSON

Le logging JSON a un surcoût par rapport au logging texte. Ce surcoût a trois composantes.

**La sérialisation JSON** — `nlohmann::json::dump()` construit la chaîne JSON avec l'échappement correct. Pour un objet avec une dizaine de champs, c'est de l'ordre de 1 à 5 microsecondes sur du matériel moderne. C'est plus lent que le formatage `fmt` d'un pattern texte (quelques centaines de nanosecondes), mais reste négligeable pour la plupart des applications.

**La taille des messages** — Un message JSON est typiquement 2 à 3 fois plus volumineux qu'un message texte équivalent (à cause des noms de champs, des guillemets, des accolades). Cela augmente le débit d'écriture sur disque et le volume de données transmises à l'agrégateur. Pour les services qui émettent des dizaines de milliers de messages par seconde, c'est un facteur à considérer pour le dimensionnement des disques et du réseau.

**Les allocations mémoire** — `nlohmann/json` alloue sur le heap pour chaque objet JSON. Dans un chemin critique à très haute fréquence, ces allocations peuvent devenir mesurables. Le mode asynchrone de spdlog (section 40.1.2) atténue ce problème en déplaçant la sérialisation dans le thread de logging.

Pour la majorité des applications, ces surcoûts sont acceptables. Si le profiling montre que le logging JSON est un goulot d'étranglement, plusieurs optimisations sont possibles : utiliser `fmt::format` pour construire le JSON manuellement (plus rapide que nlohmann pour un schéma fixe), réduire le nombre de champs par message, ou augmenter le seuil de niveau de logging.

Un compromis courant pour les applications à très haut débit est de logger en JSON uniquement vers les sinks fichier et réseau (pour l'agrégation), et de garder un format texte compact pour le sink console (pour le débogage interactif).

---

## Configuration recommandée par contexte

### Développement local

```cpp
// Console : texte lisible, pas de JSON
auto console = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();  
console->set_pattern("[%T.%e] [%^%-8l%$] [%n] %v");  
console->set_level(spdlog::level::debug);  
```

En développement, le JSON est un fardeau pour la lecture. Gardez un format texte colorisé et compact. Le développeur lit les logs dans le terminal, pas dans Kibana.

### Production Docker/Kubernetes

```cpp
// Stdout : JSON pour Promtail/Filebeat/Fluentd
auto json_stdout = std::make_shared<json_sink_mt>();  
json_stdout->add_static_field("service", "syswatch");  
json_stdout->add_static_field("version", APP_VERSION);  
json_stdout->add_static_field("environment", "production");  
json_stdout->set_level(spdlog::level::info);  
```

En production containerisée, stdout est la seule destination. Le format JSON est parsé automatiquement par l'agent de collecte. Tous les champs d'enrichissement sont dans le log lui-même car le conteneur est éphémère — les métadonnées ne sont pas dans un fichier de configuration local.

### Production bare metal

```cpp
// Fichier rotatif : JSON pour l'agrégation
auto json_file = std::make_shared<json_file_sink_mt>(
    "logs/syswatch.json.log", 50 * 1024 * 1024, 10
);
json_file->add_static_field("service", "syswatch");  
json_file->set_level(spdlog::level::info);  

// Console : texte pour journald / débogage interactif
auto console = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();  
console->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] [%n] %v");  
console->set_level(spdlog::level::warn);  
```

Sur un serveur bare metal, deux sinks coexistent : JSON dans les fichiers rotatifs (collectés par Filebeat ou Promtail), texte sur stdout/journald pour le diagnostic interactif en SSH.

---

## Choix de l'approche : arbre de décision

Le choix entre les différentes approches de logging structuré dépend du contexte du projet.

**Pattern JSON spdlog** (section 40.1.3) — Suffisant si vous contrôlez tous les messages et qu'aucun ne contient de caractères spéciaux JSON. C'est l'approche la plus simple : zéro code supplémentaire, juste un pattern. Adaptée aux outils CLI internes et aux prototypes.

**Custom sink JSON avec nlohmann** (approche 1) — Recommandé pour les services de production. Garantit un JSON valide en toutes circonstances, ajoute automatiquement les champs statiques (service, version, environment), et gère la source location. C'est l'investissement initial le plus rentable.

**Logger wrapper avec champs structurés** (approche 2, stratégie A) — Pour les projets qui veulent un logging pleinement structuré avec des champs typés. Complexité accrue mais exploitation optimale dans les agrégateurs. Justifié pour les services critiques avec des exigences d'observabilité élevées.

**Paires clé-valeur dans le message** (approche 2, stratégie B) — Le compromis pragmatique. L'API spdlog standard est utilisée telle quelle, avec la discipline de nommer les valeurs contextuelles en `key=value`. Le sink JSON emballe le message comme une chaîne et l'agrégateur parse les paires. C'est l'approche la plus courante dans l'industrie.

---

## Résumé du chapitre

Ce chapitre a couvert les cinq dimensions de l'observabilité pour une application C++ en production. Le logging structuré avec spdlog (sections 40.1 et 40.5) fournit le récit détaillé de ce qui se passe. Les métriques Prometheus (section 40.2) fournissent les indicateurs numériques pour les dashboards et les alertes. Le tracing distribué OpenTelemetry (section 40.3) suit les requêtes à travers les services. Les health checks (section 40.4) permettent à l'infrastructure de gérer le cycle de vie de l'application.

Ces cinq dimensions ne sont pas indépendantes — elles se renforcent mutuellement. Le Trace ID dans les logs (40.5) connecte les logs aux traces (40.3). Les métriques dérivées des logs (`rate` de messages error) complètent les compteurs Prometheus (40.2). Les health checks (40.4) utilisent les mêmes vérifications de dépendances que les métriques de disponibilité.

Combiné aux chapitres 37 (Docker), 38 (CI/CD), et 39 (Packaging), ce chapitre complète la boîte à outils du développeur C++ Cloud Native. L'application est compilée, testée, packagée, déployée, et maintenant — observable.

⏭️ [PARTIE VI : SUJETS AVANCÉS](/partie-06-sujets-avances.md)
