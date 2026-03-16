/* ============================================================================
   Section 30.1 : Valgrind — Détection de fuites et erreurs mémoire
   Description : Quatre types d'erreurs détectées par Memcheck :
                 use-after-free, valeur non initialisée, mismatch new/delete,
                 double-free (décommentez une section à la fois pour tester)
   Fichier source : 01-valgrind.md
   Compilation : g++-15 -std=c++23 -g -O0 -o erreurs_memoire 04_erreurs_memoire.cpp
   Exécution  : valgrind ./erreurs_memoire
   ============================================================================ */

#include <cstdio>

// Décommentez UN SEUL bloc à la fois pour tester chaque erreur

// --- Use-after-free ---
void test_use_after_free() {
    int* p = new int(10);
    delete p;
    *p = 20;  // Invalid write of size 4
}

// --- Valeur non initialisée ---
void test_uninit() {
    int x;
    if (x > 0) {  // Conditional jump depends on uninitialised value
        std::printf("positif\n");
    }
}

// --- Mismatch new/delete ---
void test_mismatch() {
    int* tab = new int[10];
    delete tab;  // Mismatched free() / delete / delete[]
    // Correction : delete[] tab;
}

// --- Double-free ---
void test_double_free() {
    int* p = new int(5);
    delete p;
    delete p;  // Invalid free() / delete / delete[]
}

int main() {
    // Décommentez une seule ligne pour tester :
    test_use_after_free();
    // test_uninit();
    // test_mismatch();
    // test_double_free();
    return 0;
}
