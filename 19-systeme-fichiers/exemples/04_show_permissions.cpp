/* ============================================================================
   Section 19.4 : Lire les permissions avec std::filesystem
   Description : Affichage rwx avec fs::status() et fs::perms
   Fichier source : 04-permissions-droits.md
   ============================================================================ */
#include <filesystem>
#include <print>

namespace fs = std::filesystem;

void show_permissions(const fs::path& p) {
    std::error_code ec;
    auto st = fs::status(p, ec);
    if (ec) {
        std::println("Erreur status({}) : {}", p.string(), ec.message());
        return;
    }

    fs::perms perm = st.permissions();

    auto flag = [](bool set, char c) -> char { return set ? c : '-'; };

    std::println("{} : {}{}{}{}{}{}{}{}{}",
        p.filename().string(),
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
    show_permissions("/etc/passwd");
    show_permissions("/etc/shadow");
    show_permissions("/usr/bin/bash");
    show_permissions("/tmp");
}
