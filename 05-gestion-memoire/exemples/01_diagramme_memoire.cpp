/* ============================================================================
   Section 5.1.1 : Diagramme mémoire d'un processus
   Description : Affiche les adresses des différentes zones mémoire (.text,
                 .rodata, .data, .bss, heap, stack) pour visualiser
                 l'organisation de l'espace d'adressage d'un processus
   Fichier source : 01.1-diagramme-memoire.md
   ============================================================================ */
#include <iostream>
#include <unistd.h>  // getpid()

int variable_bss[1000];                // .bss
int variable_data = 42;                // .data
const char* message = "Hello";         // pointeur dans .data, chaîne dans .rodata

int main() {
    int variable_stack = 10;            // stack
    int* variable_heap = new int(99);   // heap

    std::cout << "PID: " << getpid() << "\n";
    std::cout << "Adresses :\n";
    std::cout << "  code  (main)    : " << reinterpret_cast<void*>(&main) << "\n";
    std::cout << "  .rodata (msg)   : " << static_cast<const void*>(message) << "\n";
    std::cout << "  .data           : " << &variable_data << "\n";
    std::cout << "  .bss            : " << &variable_bss << "\n";
    std::cout << "  heap            : " << variable_heap << "\n";
    std::cout << "  stack           : " << &variable_stack << "\n";

    std::cout << "\nAppuyez sur Entrée pour quitter...\n";
    std::cin.get();

    delete variable_heap;
    return 0;
}
