/* ============================================================================
   Section 14.2 : API essentielle
   Description : Insertion, recherche, suppression, parcours de unordered_map
   Fichier source : 02-unordered-map.md
   ============================================================================ */
#include <unordered_map>
#include <string>
#include <print>
#include <stdexcept>

int main() {
    // === Insertion ===
    std::unordered_map<std::string, double> prix;

    // operator[] — crée ou écrase
    prix["café"] = 1.50;

    // insert — n'écrase pas si la clé existe
    auto [it, ok] = prix.insert({"thé", 2.00});
    std::println("insert thé: ok={}", ok);

    // insert_or_assign (C++17) — insert ou mise à jour explicite
    prix.insert_or_assign("café", 1.80);

    // emplace — construction en place
    prix.emplace("jus", 3.50);

    // try_emplace (C++17) — ne construit la valeur que si la clé est absente
    prix.try_emplace("eau", 0.50);

    std::println("---");

    // === Recherche ===
    // find — retourne un itérateur
    auto it2 = prix.find("café");
    if (it2 != prix.end()) {
        std::print("Café : {:.2f}€\n", it2->second);
    }

    // contains (C++20) — test d'existence
    if (prix.contains("thé")) {
        std::print("Le thé est au menu\n");
    }

    // at — accès avec exception si absent
    try {
        [[maybe_unused]] double p = prix.at("limonade");
    } catch (const std::out_of_range& e) {
        std::print("Non trouvé : {}\n", e.what());
    }

    // count — 0 ou 1
    if (prix.count("jus") > 0) {
        std::println("jus est au menu");
    }

    std::println("---");

    // === Suppression ===
    prix.erase("thé");           // Par clé
    prix.erase(prix.find("jus")); // Par itérateur
    std::println("after erase: size={}", prix.size());

    // === Parcours ===
    for (const auto& [produit, p] : prix) {
        std::print("{} -> {:.2f}€\n", produit, p);
    }
    // L'ordre d'affichage est imprévisible
}
