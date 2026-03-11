/* ============================================================================
   Section 14.4 : API de std::flat_map
   Description : Interface commune avec map, méthodes spécifiques (keys, values,
                 extract, replace), conteneurs sous-jacents personnalisables
   Fichier source : 04-flat-map-flat-set.md
   ============================================================================ */
#include <flat_map>
#include <deque>
#include <string>
#include <print>
#include <stdexcept>

int main() {
    // === Interface commune avec std::map ===
    std::flat_map<std::string, int> fm;

    // Insertion — mêmes méthodes
    fm["clé"] = 42;
    fm.insert({"autre", 10});
    fm.insert_or_assign("clé", 99);
    fm.emplace("test", 7);
    fm.try_emplace("nouveau", 33);

    // Recherche — identique
    auto it = fm.find("clé");
    bool exists = fm.contains("test");     // C++20
    int val = fm.at("clé");                // Lance out_of_range si absent
    std::size_t n = fm.count("inexistant"); // 0

    std::println("clé={}, contains test={}, at clé={}, count inexistant={}",
                 it->second, exists, val, n);

    // Requêtes par plage — identique
    auto lo = fm.lower_bound("b");
    auto hi = fm.upper_bound("t");
    std::print("Range [b, t]: ");
    for (auto i = lo; i != hi; ++i) {
        std::print("{}:{} ", i->first, i->second);
    }
    std::println("");

    // Parcours ordonné
    for (const auto& [k, v] : fm) {
        std::print("{}: {}\n", k, v);
    }

    // Suppression
    fm.erase("test");
    fm.erase(fm.find("autre"));
    std::println("after erase: size={}", fm.size());

    std::println("---");

    // === Méthodes spécifiques aux conteneurs flat ===
    const auto& keys   = fm.keys();    // Référence sur le vecteur de clés
    const auto& values = fm.values();  // Référence sur le vecteur de valeurs
    std::println("keys={}, values={}", keys.size(), values.size());

    // extract — récupérer les conteneurs sous-jacents par mouvement
    auto containers = std::move(fm).extract();
    std::println("extracted keys size={}", containers.keys.size());

    // replace — remplacer les conteneurs sous-jacents
    std::flat_map<std::string, int> fm2;
    fm2.replace(std::move(containers.keys), std::move(containers.values));
    std::println("fm2 size={}", fm2.size());

    std::println("---");

    // === Conteneurs sous-jacents personnalisables ===
    std::flat_map<int, std::string,
                  std::less<int>,
                  std::deque<int>,
                  std::deque<std::string>> deque_fm;
    deque_fm[1] = "a";
    deque_fm[2] = "b";
    std::println("deque_fm size={}", deque_fm.size());
}
