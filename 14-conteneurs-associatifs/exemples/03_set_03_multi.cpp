/* ============================================================================
   Section 14.3 : std::multiset et std::unordered_multiset
   Description : Multiensembles avec doublons, count, equal_range, erase
   Fichier source : 03-set-unordered-set.md
   ============================================================================ */
#include <set>
#include <unordered_set>
#include <string>
#include <print>

int main() {
    // std::multiset
    std::multiset<int> grades {85, 92, 78, 92, 85, 85, 100};

    std::print("Nombre de 85 : {}\n", grades.count(85)); // 3

    // equal_range pour itérer sur toutes les occurrences
    auto [begin, end] = grades.equal_range(92);
    for (auto it = begin; it != end; ++it) {
        std::print("{} ", *it); // 92 92
    }
    std::println("");

    std::println("---");

    // std::unordered_multiset
    std::unordered_multiset<std::string> word_bag {"the", "cat", "the", "hat", "the"};

    std::print("Occurrences de 'the' : {}\n", word_bag.count("the")); // 3

    // erase par valeur — supprime TOUTES les occurrences
    word_bag.erase("the"); // Supprime les 3 occurrences
    std::println("after erase 'the': size={}", word_bag.size());

    // erase par itérateur — supprime une seule occurrence
    auto it = word_bag.find("cat");
    if (it != word_bag.end()) {
        word_bag.erase(it); // Supprime une seule occurrence
    }
    std::println("after erase one 'cat': size={}", word_bag.size());
}
