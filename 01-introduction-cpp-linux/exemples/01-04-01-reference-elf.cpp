/* ============================================================================
   Section 1.4.1 : Structure du format ELF (headers, sections, segments)
   Description : Programme de référence pour l'analyse ELF. Contient des
                 éléments distribués dans différentes sections :
                 - chaîne littérale → .rodata
                 - pointeur global initialisé → .data
                 - entier global initialisé → .data (valeur 42 = 0x2a)
                 - tableau global zéro-initialisé → .bss (4000 octets)
                 - fonctions → .text
   Fichier source : 04.1-structure-elf.md (aussi utilisé par 04.2-inspection-elf.md)
   Compilation debug   : g++ -std=c++23 -g -O0 -o reference_debug 01-04-01-reference-elf.cpp
   Compilation release : g++ -std=c++23 -O2 -o reference_release 01-04-01-reference-elf.cpp
   Inspection :
     readelf -h ./reference_debug      (en-tête ELF)
     readelf -S ./reference_debug      (table des sections)
     readelf -l ./reference_debug      (table des segments)
     readelf -s ./reference_debug      (table des symboles)
     readelf -d ./reference_debug      (section .dynamic)
     size ./reference_debug            (taille text/data/bss)
     nm -C ./reference_debug           (symboles démanglés)
     objdump -d -M intel ./reference_debug  (désassemblage)
     objdump -s -j .rodata ./reference_debug (contenu .rodata)
     strings ./reference_debug | grep formation
   Sortie attendue : Formation C++ moderne (compteur: 42)
   ============================================================================ */
#include <cstdio>

const char* message = "Formation C++ moderne";
int compteur_global = 42;
int tableau_zero[1000];  // initialisé à zéro → .bss

void afficher() {
    printf("%s (compteur: %d)\n", message, compteur_global);
}

int main() {
    afficher();
    return 0;
}
