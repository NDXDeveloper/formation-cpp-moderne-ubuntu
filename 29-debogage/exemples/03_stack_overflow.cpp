/* ============================================================================
   Section 29.4.1 : AddressSanitizer (-fsanitize=address)
   Description : Stack buffer overflow — strcpy sans vérification de taille
   Fichier source : 04.1-addresssanitizer.md
   Compilation : g++-15 -fsanitize=address -g -O1 -fno-omit-frame-pointer -D_FORTIFY_SOURCE=0 -o stack_overflow 03_stack_overflow.cpp
   Note : -D_FORTIFY_SOURCE=0 nécessaire sur Ubuntu pour que ASan
          intercepte le strcpy (sinon glibc le détecte avant ASan)
   ============================================================================ */

#include <cstring>
#include <cstdio>

void process(const char* input) {
    char buffer[16];
    std::strcpy(buffer, input);    // Bug : pas de vérification de taille
    std::printf("Traité : %s\n", buffer);
}

int main() {
    process("cette-chaine-est-beaucoup-trop-longue-pour-le-buffer");
    return 0;
}
