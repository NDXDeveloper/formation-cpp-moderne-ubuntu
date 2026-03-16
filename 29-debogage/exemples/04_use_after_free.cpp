/* ============================================================================
   Section 29.4.1 : AddressSanitizer (-fsanitize=address)
   Description : Use-after-free — accès via une référence à mémoire libérée
   Fichier source : 04.1-addresssanitizer.md
   Compilation : g++-15 -fsanitize=address -g -O1 -fno-omit-frame-pointer -o use_after_free 04_use_after_free.cpp
   ============================================================================ */

#include <cstdio>
#include <string>

int main() {
    std::string* msg = new std::string("hello world");
    std::string& ref = *msg;

    delete msg;

    // Bug : ref pointe vers de la mémoire libérée
    std::printf("Message : %s\n", ref.c_str());

    return 0;
}
