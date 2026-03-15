/* ============================================================================
   Section 19.4 : Modifier les permissions avec std::filesystem
   Description : fs::permissions() avec replace, add, remove
   Fichier source : 04-permissions-droits.md
   ============================================================================ */
#include <filesystem>
#include <print>
#include <fstream>

namespace fs = std::filesystem;

void show_perms(const fs::path& p) {
    auto st = fs::status(p);
    auto perm = st.permissions();
    auto flag = [](bool set, char c) -> char { return set ? c : '-'; };
    std::println("  {}{}{}{}{}{}{}{}{}",
        flag((perm & fs::perms::owner_read)   != fs::perms::none, 'r'),
        flag((perm & fs::perms::owner_write)  != fs::perms::none, 'w'),
        flag((perm & fs::perms::owner_exec)   != fs::perms::none, 'x'),
        flag((perm & fs::perms::group_read)   != fs::perms::none, 'r'),
        flag((perm & fs::perms::group_write)  != fs::perms::none, 'w'),
        flag((perm & fs::perms::group_exec)   != fs::perms::none, 'x'),
        flag((perm & fs::perms::others_read)  != fs::perms::none, 'r'),
        flag((perm & fs::perms::others_write) != fs::perms::none, 'w'),
        flag((perm & fs::perms::others_exec)  != fs::perms::none, 'x'));
}

int main() {
    fs::path p = "/tmp/test_perms_ex04.txt";
    std::ofstream{p};

    // REPLACE : rw-------
    fs::permissions(p, fs::perms::owner_read | fs::perms::owner_write);
    std::println("Après replace (rw-------):");
    show_perms(p);

    // ADD : rw-r--r--
    fs::permissions(p,
        fs::perms::group_read | fs::perms::others_read,
        fs::perm_options::add);
    std::println("Après add (rw-r--r--):");
    show_perms(p);

    // REMOVE : rw-r-----
    fs::permissions(p,
        fs::perms::others_read,
        fs::perm_options::remove);
    std::println("Après remove (rw-r-----):");
    show_perms(p);

    // Avec gestion d'erreur
    std::error_code ec;
    fs::permissions(p, fs::perms::all, ec);
    if (ec) {
        std::println("Erreur : {}", ec.message());
    } else {
        std::println("Après all (rwxrwxrwx):");
        show_perms(p);
    }

    // Nettoyage
    fs::remove(p);
}
