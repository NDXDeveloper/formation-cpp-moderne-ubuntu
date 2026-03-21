/* ============================================================================
   Section 47.4 : Code reviews efficaces
   Description : Exemples de bugs subtils C++ a detecter en code review
                 (chaque bug est commente et corrige)
   Fichier source : 04-code-reviews.md
   ============================================================================ */

#include <string>
#include <string_view>
#include <vector>
#include <print>
#include <memory>

// --- Bug 1 : dangling string_view ---
// MAUVAIS : retourne un string_view vers un temporaire detruit
// std::string_view get_greeting_bad(const std::string& name) {
//     std::string result = "Hello, " + name + "!";
//     return result;  // result detruit → dangling string_view
// }

// CORRECT : retourner std::string par valeur
std::string get_greeting(const std::string& name) {
    return "Hello, " + name + "!";
}

// --- Bug 2 : utilisation apres move ---
// MAUVAIS :
// void process_bad(std::vector<int> dataset) {
//     auto backup = std::move(dataset);
//     // dataset est maintenant vide (moved-from state)
//     for (auto& x : dataset) { x *= 2; }  // boucle sur un vecteur vide
// }

// CORRECT : ne pas utiliser l'objet apres move
void process_correct(std::vector<int> dataset) {
    // Travailler d'abord, deplacer ensuite
    for (auto& x : dataset) { x *= 2; }
    auto result = std::move(dataset);
    std::print("Processed {} elements\n", result.size());
}

// --- Bug 3 : capture de reference vers temporaire ---
// MAUVAIS :
// auto make_counter_bad(int& start) {
//     return [&start]() { return start++; };  // dangling si start detruit
// }

// CORRECT : capturer par valeur
auto make_counter(int start) {
    return [start]() mutable { return start++; };
}

int main() {
    // Test 1 : string safe
    auto greeting = get_greeting("Nicolas");
    std::println("Greeting: {}", greeting);

    // Test 2 : process correct
    process_correct({1, 2, 3, 4, 5});

    // Test 3 : counter safe
    auto counter = make_counter(10);
    std::println("Counter: {}, {}, {}", counter(), counter(), counter());
}
