/* ============================================================================
   Section 45.3 : Use-after-free et temporal safety
   Description : Use-after-free volontaire — a compiler avec -fsanitize=address
   Fichier source : 03-use-after-free.md
   ============================================================================ */

// ATTENTION : ce programme contient un BUG VOLONTAIRE (use-after-free)
// Il est concu pour etre detecte par AddressSanitizer :
//   g++-15 -std=c++23 -O0 -fsanitize=address -g -o test 05_uaf_asan_demo.cpp
//   ./test   → ASan signale "heap-use-after-free"

#include <iostream>

struct Widget {
    int id;
    void render() { std::cout << "Widget #" << id << "\n"; }
};

int main() {
    Widget* w = new Widget{42};
    w->render();     // OK

    delete w;        // Liberation

    w->render();     // USE-AFTER-FREE — detecte par ASan
}
