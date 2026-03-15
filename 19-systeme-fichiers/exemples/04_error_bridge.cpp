/* ============================================================================
   Section 19.4 : Passerelle errno ↔ std::error_code
   Description : Conversion entre errno POSIX et std::error_code/std::errc
   Fichier source : 04-permissions-droits.md
   ============================================================================ */
#include <system_error>
#include <cerrno>
#include <print>

void demonstrate_error_bridge() {
    // errno → error_code
    errno = EACCES;  // Simuler une erreur
    std::error_code ec(errno, std::generic_category());
    std::println("Message : {}", ec.message());
    std::println("Valeur  : {}", ec.value());

    // error_code → test programmatique
    if (ec == std::errc::permission_denied) {
        std::println("Permission refusée détectée via std::errc");
    }

    // Autres exemples
    std::error_code ec2(ENOENT, std::generic_category());
    std::println("\nENOENT : {} (valeur {})", ec2.message(), ec2.value());
    if (ec2 == std::errc::no_such_file_or_directory) {
        std::println("Fichier inexistant détecté via std::errc");
    }

    std::error_code ec3(ENOSPC, std::generic_category());
    std::println("\nENOSPC : {} (valeur {})", ec3.message(), ec3.value());
    if (ec3 == std::errc::no_space_on_device) {
        std::println("Disque plein détecté via std::errc");
    }
}

int main() {
    demonstrate_error_bridge();
}
