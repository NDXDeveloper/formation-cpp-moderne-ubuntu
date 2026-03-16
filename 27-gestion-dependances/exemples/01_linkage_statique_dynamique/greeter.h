/* ============================================================================
   Section 27.4 : Linkage statique (.a) vs dynamique (.so)
   Description : Header de la bibliothèque — partagé entre statique et dynamique
   Fichier source : 04-linkage-statique-dynamique.md
   ============================================================================ */

#pragma once
#include <string>

namespace greeter {

/// Génère un message de bienvenue
std::string greet(const std::string& name);

/// Retourne la version de la bibliothèque
std::string version();

} // namespace greeter
