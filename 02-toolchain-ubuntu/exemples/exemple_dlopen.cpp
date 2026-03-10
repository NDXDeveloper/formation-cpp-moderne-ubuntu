/* ============================================================================
   Section 2.5.3 : Dépendances dynamiques et résolution
   Description : Chargement dynamique explicite avec dlopen/dlsym
   Fichier source : 05.3-dependances-dynamiques.md
   Compilation : g++ -std=c++23 -Wall -Wextra exemple_dlopen.cpp -o exemple_dlopen -ldl
   ============================================================================ */
#include <dlfcn.h>
#include <iostream>

int main() {
    // Charger la librairie mathématique à l'exécution
    void* handle = dlopen("libm.so.6", RTLD_LAZY);
    if (!handle) {
        std::cerr << "Erreur dlopen : " << dlerror() << std::endl;
        return 1;
    }

    // Chercher le symbole sqrt
    using sqrt_fn = double(*)(double);
    auto my_sqrt = reinterpret_cast<sqrt_fn>(dlsym(handle, "sqrt"));
    if (!my_sqrt) {
        std::cerr << "Erreur dlsym : " << dlerror() << std::endl;
        dlclose(handle);
        return 1;
    }

    std::cout << "sqrt(144) = " << my_sqrt(144.0) << std::endl;

    dlclose(handle);
    return 0;
}
