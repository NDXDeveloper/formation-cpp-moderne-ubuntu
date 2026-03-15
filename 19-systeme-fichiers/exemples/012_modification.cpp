/* ============================================================================
   Section 19.1.2 : Modification dynamique des composantes
   Description : replace_filename(), replace_extension(), remove_filename()
   Fichier source : 01.2-manipulation-chemins.md
   ============================================================================ */
#include <filesystem>
#include <print>

namespace fs = std::filesystem;

int main() {
    fs::path source = "/home/user/projet/src/main.cpp";

    // replace_filename()
    fs::path header = source;
    header.replace_filename("main.h");
    std::println("{}", header.string());
    // /home/user/projet/src/main.h

    // replace_extension()
    fs::path objet = source;
    objet.replace_extension(".o");
    std::println("{}", objet.string());
    // /home/user/projet/src/main.o

    fs::path sans_ext = source;
    sans_ext.replace_extension("");
    std::println("{}", sans_ext.string());
    // /home/user/projet/src/main

    // Seule la dernière extension est remplacée
    fs::path archive = "/tmp/data.tar.gz";
    fs::path changed = archive;
    changed.replace_extension(".xz");
    std::println("{}", changed.string());
    // /tmp/data.tar.xz

    // remove_filename() vs parent_path()
    fs::path p = "/home/user/projet/src/main.cpp";
    p.remove_filename();
    std::println("remove_filename : '{}'", p.string());
    // /home/user/projet/src/

    fs::path q = "/home/user/projet/src/main.cpp";
    std::println("parent_path()   : '{}'", q.parent_path().string());
    // /home/user/projet/src
}
