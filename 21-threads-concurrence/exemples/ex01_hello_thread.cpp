/* ============================================================================
   Section 21.1 : Création d'un thread - Syntaxe de base
   Description : Premier thread avec une fonction libre et join()
   Fichier source : 01-std-thread.md
   ============================================================================ */

#include <thread>
#include <print>

void say_hello() {
    std::println("Bonjour depuis un thread !");
}

int main() {
    std::thread t(say_hello);  // Le thread démarre immédiatement
    t.join();                  // On attend sa fin
}
