🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 22.5 — Clients HTTP : cpr (wrapper curl), cpp-httplib ⭐

## Chapitre 22 : Networking et Communication

---

## Introduction

La section 22.4 a couvert Boost.Beast pour les cas où HTTP s'intègre dans une boucle événementielle Asio — serveurs HTTP, proxies, applications temps réel. Mais la majorité des besoins HTTP en C++ sont beaucoup plus simples : **effectuer des requêtes vers une API externe** depuis un outil CLI, un service backend, un test d'intégration, ou un script d'automatisation.

Pour ces cas, Beast est surdimensionné. Vous n'avez pas besoin d'un `io_context`, de coroutines, ni d'un modèle asynchrone complet — vous avez besoin d'envoyer un GET, de récupérer la réponse, et de continuer votre programme. C'est exactement le rôle de **cpr** et **cpp-httplib**.

Ces deux librairies ciblent le même besoin mais avec des approches radicalement différentes :

- **cpr** — Un wrapper C++ élégant autour de **libcurl**, la librairie HTTP la plus éprouvée de l'écosystème Unix. Toute la puissance de curl (HTTPS, proxies, cookies, redirections, HTTP/2) avec une API C++ moderne.
- **cpp-httplib** — Une librairie **header-only** sans dépendance externe, qui implémente HTTP/1.1 en pur C++. Client **et** serveur minimaliste intégrés.

---

## Comparaison rapide

```
                    cpr                         cpp-httplib
                    ───                         ───────────
Dépendance          libcurl (dynamique/statique) Aucune (header-only)  
Protocoles          HTTP/1.1, HTTP/2, HTTP/3    HTTP/1.1  
TLS/SSL             Via libcurl + OpenSSL       Via OpenSSL (optionnel)  
Async               Oui (futures)               Non (bloquant)  
Mode serveur        Non (client uniquement)     Oui (client + serveur)  
Maturité backend    40+ ans (curl)              ~10 ans  
Taille intégration  Moyenne (CMake + curl)      Triviale (un seul header)  
API                 Fluent, Python-like         Simple, directe  
Cas d'usage idéal   Client HTTP de production   Prototypes, tests, outils  
                                                internes, serveurs simples
```

---

## cpr — libcurl en C++ moderne

### Présentation

**cpr** (C++ Requests) s'inspire directement de la librairie Python `requests` — considérée comme la référence en matière d'ergonomie HTTP. L'objectif est de rendre les requêtes HTTP en C++ aussi simples qu'en Python, tout en s'appuyant sur libcurl pour la robustesse et la couverture protocolaire.

libcurl, le backend de cpr, est utilisée par des milliers de projets (de `git` à `php`, de `cmake` à `youtube-dl`). Elle gère tous les cas limites de HTTP que vous ne voulez pas implémenter vous-même : chunked transfer-encoding, redirections en chaîne, authentification Digest/NTLM, proxies SOCKS, certificate pinning, HTTP/2 multiplexé, et bien plus.

### Installation

**Avec Conan 2 :**

```ini
# conanfile.txt
[requires]
cpr/1.11.0

[generators]
CMakeDeps  
CMakeToolchain  
```

**Avec vcpkg :**

```bash
vcpkg install cpr
```

**Avec CMake FetchContent :**

```cmake
include(FetchContent)  
FetchContent_Declare(  
    cpr
    GIT_REPOSITORY https://github.com/libcpr/cpr.git
    GIT_TAG        1.11.0
)
FetchContent_MakeAvailable(cpr)
```

**Configuration CMake :**

```cmake
find_package(cpr CONFIG REQUIRED)

add_executable(my_client client.cpp)  
target_link_libraries(my_client PRIVATE cpr::cpr)  
```

### Requête GET basique

```cpp
#include <cpr/cpr.h>
#include <print>

int main() {
    cpr::Response r = cpr::Get(cpr::Url{"https://httpbin.org/get"});

    std::println("Status: {}", r.status_code);
    std::println("Content-Type: {}", r.header["Content-Type"]);
    std::println("Body: {}", r.text);
}
```

C'est tout. Pas d'`io_context`, pas de résolution DNS manuelle, pas de gestion de buffer. cpr gère la connexion, le TLS, les headers, la lecture du body, et retourne un objet `Response` complet.

### L'objet `Response`

```cpp
cpr::Response r = cpr::Get(cpr::Url{"https://api.example.com/data"});

r.status_code;      // int — 200, 404, 500, etc.  
r.text;             // std::string — body en texte  
r.header;           // std::map<std::string, std::string> — headers de réponse  
r.url;              // cpr::Url — URL finale (après redirections)  
r.elapsed;          // double — durée totale en secondes  
r.cookies;          // cpr::Cookies — cookies reçus  
r.error;            // cpr::Error — détails si erreur réseau  
r.raw_header;       // std::string — headers bruts  
r.status_line;      // std::string — "HTTP/1.1 200 OK"  
```

### Vérification des erreurs

cpr distingue les **erreurs réseau** (connexion refusée, timeout, erreur DNS) des **erreurs HTTP** (404, 500). Les erreurs réseau sont dans `r.error`, les erreurs HTTP dans `r.status_code` :

```cpp
cpr::Response r = cpr::Get(cpr::Url{"https://api.example.com/data"});

// Erreur réseau ?
if (r.error) {
    std::println(stderr, "Erreur réseau: {} (code {})",
                 r.error.message,
                 static_cast<int>(r.error.code));
    // r.error.code peut être :
    //   cpr::ErrorCode::CONNECTION_FAILURE
    //   cpr::ErrorCode::OPERATION_TIMEDOUT
    //   cpr::ErrorCode::SSL_CONNECT_ERROR
    //   cpr::ErrorCode::HOST_RESOLUTION_FAILURE
    //   ...
    return 1;
}

// Erreur HTTP ?
if (r.status_code >= 400) {
    std::println(stderr, "Erreur HTTP {}: {}", r.status_code, r.text);
    return 1;
}

// Succès
std::println("Données reçues: {}", r.text);
```

### Paramètres de requête (query string)

```cpp
// https://httpbin.org/get?name=Alice&age=30
cpr::Response r = cpr::Get(
    cpr::Url{"https://httpbin.org/get"},
    cpr::Parameters{{"name", "Alice"}, {"age", "30"}}
);
```

cpr encode automatiquement les paramètres (espaces, caractères spéciaux) — pas de manipulation manuelle d'URL.

### Requête POST avec body JSON

```cpp
cpr::Response r = cpr::Post(
    cpr::Url{"https://httpbin.org/post"},
    cpr::Header{{"Content-Type", "application/json"}},
    cpr::Body{R"({"name": "Alice", "role": "admin"})"}
);

std::println("Status: {}", r.status_code);  
std::println("Réponse: {}", r.text);  
```

### POST formulaire (application/x-www-form-urlencoded)

```cpp
cpr::Response r = cpr::Post(
    cpr::Url{"https://httpbin.org/post"},
    cpr::Payload{{"username", "alice"}, {"password", "s3cret"}}
);
```

### PUT, PATCH, DELETE

L'API est uniforme pour tous les verbes HTTP :

```cpp
// PUT
cpr::Response r = cpr::Put(
    cpr::Url{"https://api.example.com/users/42"},
    cpr::Header{{"Content-Type", "application/json"}},
    cpr::Body{R"({"name": "Alice Updated"})"}
);

// PATCH
cpr::Response r = cpr::Patch(
    cpr::Url{"https://api.example.com/users/42"},
    cpr::Header{{"Content-Type", "application/json"}},
    cpr::Body{R"({"role": "superadmin"})"}
);

// DELETE
cpr::Response r = cpr::Delete(
    cpr::Url{"https://api.example.com/users/42"}
);
```

### Headers personnalisés et authentification

```cpp
// Headers custom
cpr::Response r = cpr::Get(
    cpr::Url{"https://api.example.com/data"},
    cpr::Header{
        {"Authorization", "Bearer eyJhbGciOi..."},
        {"Accept", "application/json"},
        {"X-Request-Id", "abc-123"}
    }
);

// Authentification Basic (cpr encode automatiquement en Base64)
cpr::Response r = cpr::Get(
    cpr::Url{"https://api.example.com/protected"},
    cpr::Authentication{"username", "password", cpr::AuthMode::BASIC}
);

// Authentification Bearer (raccourci)
cpr::Response r = cpr::Get(
    cpr::Url{"https://api.example.com/protected"},
    cpr::Bearer{"eyJhbGciOi..."}
);
```

### Timeouts

Les timeouts sont critiques en production. Un appel HTTP sans timeout peut bloquer votre programme indéfiniment si le serveur ne répond pas :

```cpp
cpr::Response r = cpr::Get(
    cpr::Url{"https://api.example.com/slow"},
    cpr::Timeout{5000}  // 5 secondes (timeout total)
);

if (r.error.code == cpr::ErrorCode::OPERATION_TIMEDOUT) {
    std::println(stderr, "Requête timeout après 5s");
}

// Timeout de connexion séparé du timeout total
cpr::Response r = cpr::Get(
    cpr::Url{"https://api.example.com/data"},
    cpr::ConnectTimeout{3000},  // 3s pour établir la connexion
    cpr::Timeout{10000}         // 10s au total
);
```

> ⚠️ **Mettez toujours un timeout sur vos requêtes HTTP en production.** Un serveur distant qui ne répond plus, une résolution DNS qui bloque, un firewall qui drop silencieusement les paquets — sans timeout, votre programme est à la merci du réseau.

### Redirections

Par défaut, cpr suit les redirections HTTP (301, 302, 307, 308). Vous pouvez contrôler ce comportement :

```cpp
// Suivre jusqu'à 5 redirections (défaut)
cpr::Response r = cpr::Get(
    cpr::Url{"https://example.com/old-page"},
    cpr::Redirect{5}  // Maximum 5 redirections
);

// Désactiver les redirections
cpr::Response r = cpr::Get(
    cpr::Url{"https://example.com/old-page"},
    cpr::Redirect{0}
);

// L'URL finale après redirections
std::println("URL finale: {}", r.url);
```

### Configuration TLS/SSL

```cpp
// Vérification du certificat serveur (activée par défaut)
cpr::Response r = cpr::Get(
    cpr::Url{"https://self-signed.example.com"},
    cpr::VerifySsl{false}  // Désactiver (JAMAIS en production !)
);

// Certificat CA personnalisé
cpr::Response r = cpr::Get(
    cpr::Url{"https://internal.corp.com/api"},
    cpr::CaInfo{"./ca-bundle.crt"}
);

// Certificat client (mTLS)
cpr::Response r = cpr::Get(
    cpr::Url{"https://secure.example.com/api"},
    cpr::SslOptions{
        cpr::ssl::CaInfo{"ca.pem"},
        cpr::ssl::CertFile{"client.pem"},
        cpr::ssl::KeyFile{"client.key"}
    }
);
```

### Requêtes asynchrones

cpr supporte les requêtes asynchrones via `std::future` :

```cpp
#include <cpr/cpr.h>
#include <future>
#include <print>

int main() {
    // Lancer plusieurs requêtes en parallèle
    auto future1 = cpr::GetAsync(
        cpr::Url{"https://httpbin.org/delay/2"});
    auto future2 = cpr::GetAsync(
        cpr::Url{"https://httpbin.org/delay/3"});
    auto future3 = cpr::GetAsync(
        cpr::Url{"https://httpbin.org/get"});

    // Attendre les résultats (les 3 requêtes s'exécutent en parallèle)
    cpr::Response r1 = future1.get();
    cpr::Response r2 = future2.get();
    cpr::Response r3 = future3.get();

    std::println("Requête 1: {} ({:.1f}s)", r1.status_code, r1.elapsed);
    std::println("Requête 2: {} ({:.1f}s)", r2.status_code, r2.elapsed);
    std::println("Requête 3: {} ({:.1f}s)", r3.status_code, r3.elapsed);
    // Total ≈ 3s (parallèle), pas 5s (séquentiel)
}
```

### Sessions réutilisables

Pour plusieurs requêtes vers le même serveur, une `Session` réutilise la connexion TCP (keep-alive), les cookies et les options communes :

```cpp
cpr::Session session;  
session.SetUrl(cpr::Url{"https://api.example.com"});  
session.SetHeader(cpr::Header{  
    {"Authorization", "Bearer my-token"},
    {"Accept", "application/json"}
});
session.SetTimeout(cpr::Timeout{10000});

// Réutiliser la session pour plusieurs requêtes
session.SetUrl(cpr::Url{"https://api.example.com/users"});  
cpr::Response users = session.Get();  

session.SetUrl(cpr::Url{"https://api.example.com/orders"});  
cpr::Response orders = session.Get();  

session.SetUrl(cpr::Url{"https://api.example.com/metrics"});  
cpr::Response metrics = session.Get();  
// Les 3 requêtes réutilisent la même connexion TCP si le serveur le supporte
```

### Exemple complet : client d'API REST

Voici un exemple réaliste d'un client consommant une API REST avec gestion d'erreurs et retries :

```cpp
#include <cpr/cpr.h>
#include <print>
#include <string>
#include <thread>
#include <chrono>
#include <optional>

class ApiClient {  
public:  
    explicit ApiClient(std::string base_url, std::string token)
        : base_url_{std::move(base_url)}, token_{std::move(token)} {}

    std::optional<std::string> get(const std::string& path) {
        return request_with_retry([&] {
            return cpr::Get(
                cpr::Url{base_url_ + path},
                cpr::Header{
                    {"Authorization", "Bearer " + token_},
                    {"Accept", "application/json"}
                },
                cpr::ConnectTimeout{3000},
                cpr::Timeout{10000}
            );
        });
    }

    std::optional<std::string> post(const std::string& path,
                                     const std::string& json_body) {
        return request_with_retry([&] {
            return cpr::Post(
                cpr::Url{base_url_ + path},
                cpr::Header{
                    {"Authorization", "Bearer " + token_},
                    {"Content-Type", "application/json"},
                    {"Accept", "application/json"}
                },
                cpr::Body{json_body},
                cpr::ConnectTimeout{3000},
                cpr::Timeout{10000}
            );
        });
    }

private:
    template<typename RequestFn>
    std::optional<std::string> request_with_retry(RequestFn fn,
                                                   int max_retries = 3) {
        for (int attempt = 0; attempt < max_retries; ++attempt) {
            if (attempt > 0) {
                // Backoff exponentiel : 1s, 2s, 4s...
                auto delay = std::chrono::seconds(1 << (attempt - 1));
                std::println("  Retry dans {}s...", delay.count());
                std::this_thread::sleep_for(delay);
            }

            cpr::Response r = fn();

            // Erreur réseau → retry
            if (r.error) {
                std::println(stderr, "  Erreur réseau (tentative {}): {}",
                             attempt + 1, r.error.message);
                continue;
            }

            // 5xx → retry (erreur serveur, possiblement transitoire)
            if (r.status_code >= 500) {
                std::println(stderr, "  Erreur serveur {} (tentative {})",
                             r.status_code, attempt + 1);
                continue;
            }

            // 4xx → ne pas retry (erreur client, permanente)
            if (r.status_code >= 400) {
                std::println(stderr, "Erreur client {}: {}",
                             r.status_code, r.text);
                return std::nullopt;
            }

            // Succès
            return r.text;
        }

        std::println(stderr, "Échec après {} tentatives", max_retries);
        return std::nullopt;
    }

    std::string base_url_;
    std::string token_;
};

int main() {
    ApiClient api("https://api.example.com", "my-token-123");

    if (auto users = api.get("/v1/users")) {
        std::println("Users: {}", *users);
    }

    if (auto created = api.post("/v1/users", R"({"name":"Alice"})")) {
        std::println("Created: {}", *created);
    }
}
```

---

## cpp-httplib — HTTP header-only sans dépendance

### Présentation

**cpp-httplib** prend l'approche opposée de cpr : au lieu de wrapper une librairie C externe, elle implémente HTTP/1.1 entièrement en C++, dans un **seul fichier header**. Pas de libcurl, pas de dépendance système, pas de configuration de build complexe — vous incluez `httplib.h` et c'est prêt.

L'autre particularité de cpp-httplib est qu'elle fournit à la fois un **client** et un **serveur** HTTP. Le serveur est minimaliste mais suffisant pour des outils internes, des mocks de test, ou des API simples.

### Installation

**Un seul fichier à copier :**

```bash
# Télécharger le header
curl -o httplib.h https://raw.githubusercontent.com/yhirose/cpp-httplib/master/httplib.h
```

**Avec Conan 2 :**

```ini
[requires]
cpp-httplib/0.18.0
```

**Avec vcpkg :**

```bash
vcpkg install cpp-httplib
```

**Avec CMake FetchContent :**

```cmake
FetchContent_Declare(
    httplib
    GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
    GIT_TAG        v0.18.0
)
FetchContent_MakeAvailable(httplib)

target_link_libraries(my_app PRIVATE httplib::httplib)
```

### Configuration CMake avec TLS

cpp-httplib supporte HTTPS si OpenSSL est disponible :

```cmake
find_package(OpenSSL REQUIRED)

add_executable(my_app main.cpp)  
target_link_libraries(my_app PRIVATE OpenSSL::SSL OpenSSL::Crypto)  
target_compile_definitions(my_app PRIVATE CPPHTTPLIB_OPENSSL_SUPPORT)  
```

La macro `CPPHTTPLIB_OPENSSL_SUPPORT` active le support TLS. Sans elle, seul HTTP fonctionne.

### Client HTTP : requête GET

```cpp
#include "httplib.h"
#include <print>

int main() {
    httplib::Client cli("https://httpbin.org");

    auto res = cli.Get("/get");

    if (res) {
        std::println("Status: {}", res->status);
        std::println("Body: {}", res->body);
    } else {
        std::println(stderr, "Erreur: {}",
                     httplib::to_string(res.error()));
    }
}
```

### L'objet `Result`

`cli.Get()` (et les autres méthodes) retourne un `httplib::Result` — un type qui combine un `std::optional`-like pour la réponse et un code d'erreur :

```cpp
auto res = cli.Get("/api/data");

if (!res) {
    // Erreur réseau
    httplib::Error err = res.error();
    // err peut être :
    //   httplib::Error::Connection
    //   httplib::Error::Read
    //   httplib::Error::Write
    //   httplib::Error::SSLConnection
    //   httplib::Error::ConnectionTimeout
    //   httplib::Error::ReadTimeout
    //   ...
    std::println(stderr, "Erreur: {}", httplib::to_string(err));
    return;
}

// Réponse reçue
res->status;  // int — code HTTP  
res->body;    // std::string — corps de la réponse  
res->headers; // httplib::Headers — multimap des headers  
```

### Requête POST

```cpp
httplib::Client cli("https://api.example.com");

// POST JSON
auto res = cli.Post(
    "/api/users",
    R"({"name": "Alice", "role": "admin"})",
    "application/json"
);

// POST formulaire
auto res = cli.Post(
    "/api/login",
    "username=alice&password=secret",
    "application/x-www-form-urlencoded"
);

// POST avec httplib::Params (encodage automatique)
httplib::Params params{{"username", "alice"}, {"password", "secret"}};  
auto res = cli.Post("/api/login", params);  
```

### PUT, PATCH, DELETE

```cpp
httplib::Client cli("https://api.example.com");

auto res = cli.Put(
    "/api/users/42",
    R"({"name": "Alice Updated"})",
    "application/json"
);

auto res = cli.Patch(
    "/api/users/42",
    R"({"role": "superadmin"})",
    "application/json"
);

auto res = cli.Delete("/api/users/42");
```

### Headers et authentification

```cpp
httplib::Client cli("https://api.example.com");

// Headers par défaut pour toutes les requêtes
cli.set_default_headers({
    {"Accept", "application/json"},
    {"X-Api-Version", "2"}
});

// Authentification Basic
cli.set_basic_auth("username", "password");

// Authentification Bearer
cli.set_bearer_token_auth("eyJhbGciOi...");

// Headers spécifiques à une requête
httplib::Headers headers = {
    {"X-Request-Id", "req-456"}
};
auto res = cli.Get("/api/data", headers);
```

### Timeouts

```cpp
httplib::Client cli("https://api.example.com");

cli.set_connection_timeout(3);    // 3 secondes pour la connexion  
cli.set_read_timeout(10);         // 10 secondes pour la lecture  
cli.set_write_timeout(5);         // 5 secondes pour l'écriture  

// Ou avec des durées chrono
cli.set_connection_timeout(std::chrono::seconds(3));  
cli.set_read_timeout(std::chrono::seconds(10));  
```

### Serveur HTTP intégré

La fonctionnalité distinctive de cpp-httplib est son **serveur HTTP intégré**. Il est bloquant et mono-threadé par défaut (avec option multi-thread), mais suffisant pour de nombreux cas :

```cpp
#include "httplib.h"
#include <print>

int main() {
    httplib::Server svr;

    // Route GET
    svr.Get("/", [](const httplib::Request&, httplib::Response& res) {
        res.set_content("Hello from cpp-httplib!", "text/plain");
    });

    // Route GET avec paramètre de chemin
    svr.Get(R"(/users/(\d+))", [](const httplib::Request& req,
                                    httplib::Response& res) {
        auto user_id = req.matches[1];  // Capture regex
        res.set_content("User ID: " + std::string(user_id), "text/plain");
    });

    // Route POST JSON
    svr.Post("/api/data", [](const httplib::Request& req,
                              httplib::Response& res) {
        std::println("Body reçu: {}", req.body);
        res.set_content(R"({"status":"created"})", "application/json");
        res.status = 201;
    });

    // Fichiers statiques
    svr.set_mount_point("/static", "./public");

    // Gestion d'erreur 404
    svr.set_error_handler([](const httplib::Request&, httplib::Response& res) {
        res.set_content("Not Found", "text/plain");
        res.status = 404;
    });

    std::println("Serveur HTTP sur http://localhost:8080/");
    svr.listen("0.0.0.0", 8080);  // Bloquant
}
```

### Serveur HTTPS

```cpp
// Nécessite CPPHTTPLIB_OPENSSL_SUPPORT
httplib::SSLServer svr("server.pem", "server.key");

svr.Get("/", [](const httplib::Request&, httplib::Response& res) {
    res.set_content("Hello HTTPS!", "text/plain");
});

svr.listen("0.0.0.0", 8443);
```

### Serveur multi-threadé

```cpp
httplib::Server svr;

// Utiliser un pool de threads pour traiter les requêtes
svr.new_task_queue = [] {
    return new httplib::ThreadPool(8);  // 8 workers
};

// ... définir les routes ...
svr.listen("0.0.0.0", 8080);
```

### Cas d'usage du serveur cpp-httplib

Le serveur cpp-httplib n'est pas un concurrent de Nginx ou de Beast. Il est idéal pour :

- **Tests d'intégration** — Mocker une API externe dans vos tests. Démarrez un serveur cpp-httplib dans votre fixture de test, configurez les réponses attendues, et faites vos requêtes dessus.
- **Outils de développement** — Un serveur web embarqué pour servir une interface de debug, des métriques, ou une page de status.
- **Prototypes** — Valider rapidement une architecture avant de migrer vers Beast ou un framework plus robuste.
- **Endpoints de health check** — Un `/health` ou `/ready` minimaliste pour Kubernetes, intégré directement dans votre application.

---

## Intégration avec les librairies JSON

Les deux librairies retournent le body comme `std::string`. L'intégration avec une librairie JSON est directe :

### Avec nlohmann/json (chapitre 24)

```cpp
#include <nlohmann/json.hpp>
using json = nlohmann::json;

// cpr
cpr::Response r = cpr::Get(cpr::Url{"https://api.example.com/users"});  
if (!r.error && r.status_code == 200) {  
    auto data = json::parse(r.text);
    for (auto& user : data["users"]) {
        std::println("User: {}", user["name"].get<std::string>());
    }
}

// cpp-httplib
httplib::Client cli("https://api.example.com");  
auto res = cli.Get("/users");  
if (res && res->status == 200) {  
    auto data = json::parse(res->body);
    // ...
}
```

### Avec Boost.JSON

```cpp
#include <boost/json.hpp>
namespace bjson = boost::json;

cpr::Response r = cpr::Get(cpr::Url{"https://api.example.com/data"});  
if (!r.error && r.status_code == 200) {  
    auto data = bjson::parse(r.text);
    auto& obj = data.as_object();
    // ...
}
```

---

## Guide de choix entre cpr et cpp-httplib

### Choisissez cpr quand :

- Vous êtes **client uniquement** (pas de serveur HTTP à écrire).
- Vous avez besoin de **HTTP/2** ou **HTTP/3**.
- Vous avez besoin de fonctionnalités avancées : proxies SOCKS, certificate pinning, authentification Digest/NTLM, cookies persistants.
- Vous avez besoin de **requêtes asynchrones** (futures).
- Vous êtes en **production** et vous voulez la robustesse de libcurl (40+ ans de corrections de bugs et de cas limites HTTP).
- Le fait de dépendre de libcurl n'est pas un problème (ce n'est pas un problème sur Linux — curl est omniprésent).

### Choisissez cpp-httplib quand :

- Vous voulez **zéro dépendance** externe (un seul header suffit).
- Vous avez besoin d'un **serveur HTTP simple** en plus du client.
- Vous faites du **prototypage rapide** ou des **tests d'intégration**.
- Votre cas d'usage est simple : quelques requêtes GET/POST, pas de HTTP/2, pas de proxy.
- Vous êtes dans un environnement contraint où installer libcurl est compliqué (cross-compilation, embedded).
- Vous voulez un temps de compilation minimal (un seul header vs la chaîne CMake de cpr + curl).

### Ni l'un ni l'autre quand :

- Vous construisez un **serveur HTTP haute performance** → Boost.Beast (section 22.4.2).
- Vous avez besoin d'intégration dans une **boucle événementielle Asio** → Boost.Beast.
- Vous faites du **WebSocket** → Boost.Beast ou une librairie dédiée.

---

## Résumé

Pour les besoins de client HTTP en C++ — interroger une API, télécharger des données, envoyer des métriques — deux librairies couvrent l'essentiel :

- **cpr** est le choix de production. Backed par libcurl, il offre une API C++ élégante avec une couverture protocolaire exhaustive (HTTPS, HTTP/2, proxies, auth, cookies, async). C'est le `requests` de Python porté en C++.
- **cpp-httplib** est le choix de simplicité. Header-only, zéro dépendance, client + serveur intégrés. Idéal pour le prototypage, les tests, et les outils internes.

Dans les deux cas, l'intégration avec une librairie JSON (`nlohmann/json`, Boost.JSON) est triviale, et les bonnes pratiques sont les mêmes : toujours mettre un timeout, vérifier les erreurs réseau **et** les codes HTTP, et implémenter des retries avec backoff pour les erreurs transitoires.

---

> **Prochaine étape** → Section 22.6 : gRPC et Protocol Buffers — la communication inter-services haute performance, au-delà de HTTP/REST.

⏭️ [gRPC et Protocol Buffers : RPC moderne haute performance](/22-networking/06-grpc-protobuf.md)
