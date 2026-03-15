/* ============================================================================
   Section 21.1 : Création d'un thread - Les différents types de callables
   Description : Fonction libre, lambda, foncteur et fonction membre
   Fichier source : 01-std-thread.md
   ============================================================================ */

#include <thread>
#include <print>
#include <string>

// 1. Fonction libre
void greet(const std::string& name) {
    std::println("Bonjour, {} !", name);
}

// 2. Foncteur (objet avec operator())
struct Worker {
    int id;
    void operator()() const {
        std::println("Worker {} en cours d'exécution", id);
    }
};

// 3. Fonction membre
struct Logger {
    void log(const std::string& msg) const {
        std::println("[LOG] {}", msg);
    }
};

int main() {
    // Fonction libre avec argument
    std::thread t1(greet, "Alice");

    // Lambda (la forme la plus idiomatique en C++ moderne)
    std::thread t2([] {
        std::println("Lambda dans un thread");
    });

    // Foncteur — attention aux parenthèses (voir encadré ci-dessous)
    std::thread t3(Worker{42});

    // Fonction membre : on passe le pointeur de méthode puis l'objet
    Logger logger;
    std::thread t4(&Logger::log, &logger, "Démarrage du service");

    t1.join();
    t2.join();
    t3.join();
    t4.join();
}
