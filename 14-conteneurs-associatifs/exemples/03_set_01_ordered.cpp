/* ============================================================================
   Section 14.3 : std::set — l'ensemble ordonné
   Description : Construction, insertion, recherche, plages, suppression,
                 extract, transparent comparators
   Fichier source : 03-set-unordered-set.md
   ============================================================================ */
#include <set>
#include <string>
#include <string_view>
#include <print>

int main() {
    // Construction — doublons ignorés, triés automatiquement
    std::set<int> primes {7, 2, 11, 3, 5, 2, 3};
    for (int p : primes) {
        std::print("{} ", p);
    }
    // Affiche : 2 3 5 7 11  (trié, sans doublons)
    std::print("\nTaille : {}\n", primes.size()); // 5, pas 7

    std::println("---");

    // Insertion
    std::set<std::string> tags;
    auto [it1, ok1] = tags.insert("urgent");    // ok1 == true
    auto [it2, ok2] = tags.insert("urgent");    // ok2 == false — déjà présent
    auto [it3, ok3] = tags.emplace("critical"); // ok3 == true
    std::println("ok1={}, ok2={}, ok3={}", ok1, ok2, ok3);

    std::println("---");

    // Recherche
    std::set<std::string> allowed {"admin", "editor", "viewer"};

    auto it = allowed.find("editor");
    if (it != allowed.end()) {
        std::print("Rôle trouvé : {}\n", *it);
    }

    if (allowed.contains("admin")) {
        std::print("Accès administrateur autorisé\n");
    }

    if (allowed.count("hacker") == 0) {
        std::print("Rôle inconnu\n");
    }

    std::println("---");

    // Requêtes par plage
    std::set<int> scores {10, 25, 30, 42, 55, 67, 78, 83, 91, 100};
    auto from = scores.lower_bound(30);  // Premier >= 30
    auto to   = scores.upper_bound(80);  // Premier > 80
    std::print("Scores entre 30 et 80 : ");
    for (auto i = from; i != to; ++i) {
        std::print("{} ", *i);
    }
    // Affiche : 30 42 55 67 78
    std::println("");

    std::println("---");

    // Suppression
    std::set<int> s {1, 2, 3, 4, 5};
    s.erase(3);             // Par valeur
    s.erase(s.find(5));     // Par itérateur
    s.erase(s.begin(), s.find(4)); // Par plage
    std::print("remaining: ");
    for (int x : s) std::print("{} ", x);
    std::println("");

    std::println("---");

    // extract (C++17) — modifier un élément
    std::set<std::string> names {"Alice", "Bob"};
    auto node = names.extract("Alice");
    if (!node.empty()) {
        node.value() = "Alicia"; // Modification autorisée sur un nœud extrait
        names.insert(std::move(node));
    }
    for (const auto& n : names) std::print("{} ", n);
    std::println("");

    std::println("---");

    // Transparent comparators
    std::set<std::string, std::less<>> words {"apple", "banana", "cherry"};
    bool found = words.contains("banana");
    std::string_view sv = "cherry";
    auto it4 = words.find(sv);
    std::println("found banana={}, cherry={}", found, it4 != words.end());
}
