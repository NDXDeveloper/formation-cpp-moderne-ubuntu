/* ============================================================================
   Section 3.4 : Portee des variables (Scope) et duree de vie (Lifetime)
   Description : Portee de bloc, shadowing, duree de vie statique,
                 prolongation de duree de vie par reference const
   Fichier source : 04-portee-duree-vie.md
   ============================================================================ */
#include <print>
#include <string>
#include <vector>
#include <map>

// --- Variable locale statique (lignes 189-197) ---
int generate_id() {
    static int next_id = 0;
    return ++next_id;
}

// --- Prolongation de duree de vie (lignes 289-300) ---
std::string create_greeting() {
    return "Bonjour le monde";
}

int main() {
    // --- Portee de bloc (lignes 27-41) ---
    int x = 42;
    if (x > 0) {
        int y = 10;
        std::print("x={}, y={}\n", x, y);
    }
    // y n'existe plus ici
    std::print("x={}\n", x);

    // --- Boucle for (lignes 49-53) ---
    for (int i = 0; i < 3; ++i) {
        std::print("i={} ", i);
    }
    std::print("\n");
    // i n'existe plus ici

    // --- if avec initialisation C++17 (lignes 60-65) ---
    std::map<std::string, int> mp = {{"key", 42}};
    if (auto it = mp.find("key"); it != mp.end()) {
        std::print("Trouve : {}\n", it->second);
    } else {
        std::print("Non trouve\n");
    }

    // --- generate_id (variable statique) (lignes 189-197) ---
    std::print("id1={}\n", generate_id()); // 1
    std::print("id2={}\n", generate_id()); // 2
    std::print("id3={}\n", generate_id()); // 3

    // --- Prolongation de duree de vie (lignes 289-300) ---
    const std::string& ref = create_greeting();
    std::print("ref={}\n", ref);

    // --- Value-initialization (lignes 355-357) ---
    int zero{};
    std::print("int{{}} = {}\n", zero); // 0

    // --- Declarations au plus pres (lignes 380-384) ---
    std::vector<int> data = {1, 2, 3, 4, 5};
    int sum = 0;
    for (const auto& val : data) { sum += val; }
    auto average = static_cast<double>(sum) / data.size();
    std::print("sum={}, average={}\n", sum, average);

    return 0;
}
