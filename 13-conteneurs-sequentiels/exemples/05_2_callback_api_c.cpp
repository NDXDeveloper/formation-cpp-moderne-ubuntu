/* ============================================================================
   Section 13.5.2 : Callbacks et interopérabilité API C
   Description : Callback moderne avec std::span au lieu de pointeur+taille,
                 et wrapper pour appeler une API C depuis du code moderne
   Fichier source : 05.2-remplacement-pointeur-taille.md
   ============================================================================ */
#include <span>
#include <vector>
#include <functional>
#include <print>

// Callback moderne
using callback_t = std::function<void(std::span<const float>)>;

void generer_signal(std::size_t nb_echantillons, callback_t on_data) {
    std::vector<float> buffer(nb_echantillons);
    for (std::size_t i = 0; i < nb_echantillons; ++i) {
        buffer[i] = static_cast<float>(i) * 0.1f;
    }
    on_data(buffer);
    std::span<const float> vue(buffer);
    on_data(vue.first(nb_echantillons / 2));
    on_data(vue.last(nb_echantillons / 2));
}

// API C existante
extern "C" {
    int c_process_buffer(const int* data, int count) {
        int total = 0;
        for (int i = 0; i < count; ++i) total += data[i];
        return total;
    }
}

int traiter(std::span<const int> donnees) {
    return c_process_buffer(donnees.data(), static_cast<int>(donnees.size()));
}

int main() {
    // Callback
    generer_signal(10, [](std::span<const float> data) {
        std::println("Reçu {} échantillons", data.size());
    });

    // API C wrapper
    std::vector<int> v{1, 2, 3, 4, 5};
    std::println("Résultat : {}", traiter(v));
}
