/* ============================================================================
   Section 29.4.2 : UndefinedBehaviorSanitizer (-fsanitize=undefined)
   Description : Déréférencement de pointeur null — UB détecté par UBSan
   Fichier source : 04.2-ubsan.md
   Compilation : g++-15 -fsanitize=undefined -g -O1 -fno-omit-frame-pointer -o null_deref 12_null_deref.cpp
   ============================================================================ */

#include <cstdio>

struct Config {
    int port;
    const char* host;
};

void print_config(Config* cfg) {
    std::printf("Host: %s, Port: %d\n", cfg->host, cfg->port);
}

int main() {
    Config* cfg = nullptr;
    print_config(cfg);    // UB : déréférencement de nullptr
    return 0;
}
