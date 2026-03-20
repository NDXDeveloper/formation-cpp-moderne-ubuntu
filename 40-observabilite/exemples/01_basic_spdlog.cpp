/* ============================================================================
   Section 40.1.1 : Installation et configuration de spdlog
   Description : Premier programme spdlog — niveaux, formatage, logger par defaut
   Fichier source : 01.1-installation-spdlog.md
   ============================================================================ */

#include <spdlog/spdlog.h>
#include <string>

int main() {
    // Le logger par défaut écrit sur stdout en niveau info
    spdlog::info("Application démarrée");
    spdlog::debug("Ce message ne s'affiche pas (niveau info par défaut)");

    // Changer le niveau de logging
    spdlog::set_level(spdlog::level::debug);
    spdlog::debug("Maintenant ce message s'affiche");

    // Formatage positionnel (syntaxe fmt/std::format)
    int port = 8080;
    std::string host = "0.0.0.0";
    spdlog::info("Serveur en écoute sur {}:{}", host, port);

    // Niveaux de sévérité
    spdlog::warn("Fichier de config absent, utilisation des valeurs par défaut");
    spdlog::error("Connexion à la base de données échouée: timeout après {}ms", 5000);
    spdlog::critical("Espace disque insuffisant, arrêt du service");

    return 0;
}
