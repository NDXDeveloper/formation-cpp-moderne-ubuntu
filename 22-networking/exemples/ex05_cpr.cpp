/* ============================================================================
   Section 22.5 : Clients HTTP — cpr, cpp-httplib
   Description : Client HTTP cpr (wrapper curl) — GET/POST/PUT/DELETE,
                 paramètres, headers, timeout, gestion erreurs réseau
   Fichier source : 05-clients-http.md
   ============================================================================ */
#include <cpr/cpr.h>
#include "httplib.h"
#include <print>
#include <thread>
#include <chrono>

void run_test_server(httplib::Server& svr) {
    svr.Get("/get", [](const httplib::Request& req, httplib::Response& res) {
        std::string params;
        for (auto& [k, v] : req.params) {
            params += "\"" + k + "\": \"" + v + "\", ";
        }
        res.set_content(R"({"url":"http://localhost:18097/get","args":{)" + params + R"(}"headers":{}})", "application/json");
    });

    svr.Post("/post", [](const httplib::Request& req, httplib::Response& res) {
        res.set_content(R"({"data":")" + req.body + R"(","headers":{}})", "application/json");
    });

    svr.Put("/users/42", [](const httplib::Request& req, httplib::Response& res) {
        res.set_content(R"({"updated":true,"body":")" + req.body + R"("})", "application/json");
    });

    svr.Delete("/users/42", [](const httplib::Request&, httplib::Response& res) {
        res.set_content(R"({"deleted":true})", "application/json");
    });

    svr.Get("/slow", [](const httplib::Request&, httplib::Response& res) {
        std::this_thread::sleep_for(std::chrono::seconds(10));
        res.set_content("slow response", "text/plain");
    });

    svr.listen("0.0.0.0", 18097);
}

int main() {
    httplib::Server svr;
    std::jthread server_thread([&svr]{ run_test_server(svr); });
    std::this_thread::sleep_for(std::chrono::milliseconds(300));

    // === Test GET basique (from .md) ===
    std::println("=== GET basique ===");
    {
        cpr::Response r = cpr::Get(cpr::Url{"http://localhost:18097/get"});
        std::println("Status: {}", r.status_code);
        std::println("Content-Type: {}", r.header["Content-Type"]);
        std::println("Body: {}", r.text);
    }

    // === Test GET avec paramètres (from .md) ===
    std::println("\n=== GET avec paramètres ===");
    {
        cpr::Response r = cpr::Get(
            cpr::Url{"http://localhost:18097/get"},
            cpr::Parameters{{"name", "Alice"}, {"age", "30"}}
        );
        std::println("Status: {}", r.status_code);
        std::println("Body: {}", r.text);
    }

    // === Test POST JSON (from .md) ===
    std::println("\n=== POST JSON ===");
    {
        cpr::Response r = cpr::Post(
            cpr::Url{"http://localhost:18097/post"},
            cpr::Header{{"Content-Type", "application/json"}},
            cpr::Body{R"({"name": "Alice", "role": "admin"})"}
        );
        std::println("Status: {}", r.status_code);
        std::println("Réponse: {}", r.text);
    }

    // === Test PUT (from .md) ===
    std::println("\n=== PUT ===");
    {
        cpr::Response r = cpr::Put(
            cpr::Url{"http://localhost:18097/users/42"},
            cpr::Header{{"Content-Type", "application/json"}},
            cpr::Body{R"({"name": "Alice Updated"})"}
        );
        std::println("Status: {}", r.status_code);
        std::println("Réponse: {}", r.text);
    }

    // === Test DELETE (from .md) ===
    std::println("\n=== DELETE ===");
    {
        cpr::Response r = cpr::Delete(
            cpr::Url{"http://localhost:18097/users/42"}
        );
        std::println("Status: {}", r.status_code);
        std::println("Réponse: {}", r.text);
    }

    // === Test vérification erreurs (from .md) ===
    std::println("\n=== Erreur réseau (connexion refusée) ===");
    {
        cpr::Response r = cpr::Get(
            cpr::Url{"http://localhost:19999/nonexistent"},
            cpr::Timeout{2000}
        );
        if (r.error) {
            std::println("Erreur réseau: {} (code {})",
                         r.error.message,
                         static_cast<int>(r.error.code));
        }
    }

    // === Test timeout (from .md) ===
    std::println("\n=== Timeout ===");
    {
        cpr::Response r = cpr::Get(
            cpr::Url{"http://localhost:18097/slow"},
            cpr::Timeout{1000}
        );
        if (r.error.code == cpr::ErrorCode::OPERATION_TIMEDOUT) {
            std::println("Requête timeout après 1s ✓");
        } else {
            std::println("Status: {} (attendu: timeout)", r.status_code);
        }
    }

    // === Test Response fields (from .md) ===
    std::println("\n=== Champs Response ===");
    {
        cpr::Response r = cpr::Get(cpr::Url{"http://localhost:18097/get"});
        std::println("status_code: {}", r.status_code);
        std::println("elapsed: {:.3f}s", r.elapsed);
        std::println("url: {}", r.url.str());
        std::println("status_line: {}", r.status_line);
    }

    svr.stop();
    std::println("\nTest cpr OK !");
}
