/* ============================================================================
   Section 21.1 : Passage d'arguments - Copie, std::ref et std::move
   Description : Les trois modes de passage d'arguments à un thread
   Fichier source : 01-std-thread.md
   ============================================================================ */

#include <thread>
#include <print>
#include <string>
#include <functional>  // std::ref
#include <memory>

// Copie par défaut
void process(std::string data) {
    std::println("Traitement de : {}", data);
}

// Référence avec std::ref
void increment(int& value) {
    ++value;
}

// Déplacement avec std::move
void consume(std::unique_ptr<int> ptr) {
    std::println("Valeur reçue : {}", *ptr);
}

int main() {
    // 1. Copie par défaut
    std::string message = "Hello";
    std::thread t1(process, message);
    t1.join();

    // 2. Référence avec std::ref
    int counter = 0;
    std::thread t2(increment, std::ref(counter));
    t2.join();
    std::println("counter = {}", counter);  // Affiche 1

    // 3. Déplacement avec std::move
    auto ptr = std::make_unique<int>(42);
    std::thread t3(consume, std::move(ptr));
    t3.join();
}
