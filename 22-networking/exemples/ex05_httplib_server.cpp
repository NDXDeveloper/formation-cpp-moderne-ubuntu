/* ============================================================================
   Section 22.5 : Clients HTTP — cpr, cpp-httplib
   Description : Serveur HTTP avec cpp-httplib — routes GET/POST, paramètres
                 regex, gestion 404, client intégré pour tests
   Fichier source : 05-clients-http.md
   ============================================================================ */
#include "httplib.h"
#include <print>
#include <thread>
#include <chrono>

int main() {
    httplib::Server svr;

    // Route GET
    svr.Get("/", [](const httplib::Request&, httplib::Response& res) {
        res.set_content("Hello from cpp-httplib!", "text/plain");
    });

    // Route GET avec paramètre de chemin
    svr.Get(R"(/users/(\d+))", [](const httplib::Request& req,
                                    httplib::Response& res) {
        auto user_id = req.matches[1];
        res.set_content("User ID: " + std::string(user_id), "text/plain");
    });

    // Route POST JSON
    svr.Post("/api/data", [](const httplib::Request& req,
                              httplib::Response& res) {
        std::println("Body reçu: {}", req.body);
        res.set_content(R"({"status":"created"})", "application/json");
        res.status = 201;
    });

    // Gestion d'erreur 404
    svr.set_error_handler([](const httplib::Request&, httplib::Response& res) {
        if (res.status == 404) {
            res.set_content("Not Found", "text/plain");
        }
    });

    // Client thread
    std::jthread client([&svr]{
        std::this_thread::sleep_for(std::chrono::milliseconds(300));

        httplib::Client cli("http://localhost:18096");

        // GET /
        auto r1 = cli.Get("/");
        if (r1) std::println("GET / → {} : {}", r1->status, r1->body);

        // GET /users/42
        auto r2 = cli.Get("/users/42");
        if (r2) std::println("GET /users/42 → {} : {}", r2->status, r2->body);

        // POST /api/data
        auto r3 = cli.Post("/api/data", R"({"key":"value"})", "application/json");
        if (r3) std::println("POST /api/data → {} : {}", r3->status, r3->body);

        // GET /nonexistent
        auto r4 = cli.Get("/nonexistent");
        if (r4) std::println("GET /nonexistent → {} : {}", r4->status, r4->body);

        svr.stop();
    });

    std::println("Serveur HTTP sur http://localhost:18096/");
    svr.listen("0.0.0.0", 18096);
    std::println("Test cpp-httplib server OK !");
}
