/* ============================================================================
   Section 13.1.2 : Accès aux éléments
   Description : operator[] vs at(), front()/back(), data() et interopérabilité
                 avec les API C
   Fichier source : 01.2-methodes-essentielles.md
   ============================================================================ */
#include <vector>
#include <cstring>
#include <print>
#include <stdexcept>

// Simulation d'une API C qui attend un buffer brut
void api_c_legacy(const int* buffer, int taille) {
    for (int i = 0; i < taille; ++i) {
        std::print("{} ", buffer[i]);
    }
    std::println("");
}

int main() {
    // operator[] vs at()
    {
        std::vector<int> v{10, 20, 30};
        std::println("v[1] = {}", v[1]);
        std::println("v.at(1) = {}", v.at(1));

        try {
            [[maybe_unused]] auto val = v.at(10);
        } catch (const std::out_of_range& e) {
            std::println("Exception : {}", e.what());
        }
    }

    // front() et back()
    {
        std::vector<int> v{10, 20, 30};
        std::println("Premier : {}, Dernier : {}", v.front(), v.back());
        // Sortie : Premier : 10, Dernier : 30

        v.front() = 99;
        v.back() = 77;
        // v = {99, 20, 77}
    }

    // data()
    {
        std::vector<int> v{1, 2, 3, 4, 5};
        api_c_legacy(v.data(), static_cast<int>(v.size()));
        // Sortie : 1 2 3 4 5

        int source[] = {100, 200, 300};
        std::vector<int> dest(3);
        std::memcpy(dest.data(), source, sizeof(source));
        for (auto val : dest) std::print("{} ", val);
        std::println("");
        // Sortie : 100 200 300
    }

    // clear
    {
        std::vector<int> v(1000, 42);
        v.clear();
        std::println("size={}, capacity={}", v.size(), v.capacity());
        // Sortie : size=0, capacity=1000
    }
}
