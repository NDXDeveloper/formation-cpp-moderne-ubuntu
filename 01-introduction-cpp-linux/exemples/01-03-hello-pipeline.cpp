/* ============================================================================
   Section 1.3 : Le cycle de compilation
   Description : Programme minimal pour démontrer les 4 étapes du pipeline
                 Utilise printf et une macro #define pour illustrer le
                 préprocesseur, la compilation, l'assemblage et l'édition
                 de liens
   Fichier source : 03-cycle-compilation.md
   Compilation : g++ -o 01-03-hello-pipeline 01-03-hello-pipeline.cpp
   Étapes séparées :
     g++ -E 01-03-hello-pipeline.cpp -o hello.ii    (préprocesseur)
     g++ -S 01-03-hello-pipeline.cpp -o hello.s     (compilation)
     g++ -c 01-03-hello-pipeline.cpp -o hello.o     (assemblage)
     g++ hello.o -o 01-03-hello-pipeline             (édition de liens)
   Sortie attendue : Bonjour depuis le pipeline de compilation !
   ============================================================================ */
#include <cstdio>

#define MESSAGE "Bonjour depuis le pipeline de compilation !\n"

int main() {
    printf(MESSAGE);
    return 0;
}
