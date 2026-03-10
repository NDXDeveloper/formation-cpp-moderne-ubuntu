/* ============================================================================
   Section 2.6.3 : Debug (-g, -ggdb3)
   Description : Fonction simple pour illustrer l'impact de -O2 sur le débogage
   Fichier source : 06.3-debug.md
   Compilation :
     g++ -O0 -g compute_debug.cpp -o compute_O0    (toutes variables inspectables)
     g++ -O2 -g compute_debug.cpp -o compute_O2    (variables "optimized out")
   ============================================================================ */
#include <iostream>

int compute(int a, int b, int c) {
    int x = a + b;
    int y = x * c;
    int z = y - a;
    return z;
}

int main() {
    int result = compute(3, 4, 5);
    std::cout << "compute(3, 4, 5) = " << result << std::endl;
    return 0;
}
