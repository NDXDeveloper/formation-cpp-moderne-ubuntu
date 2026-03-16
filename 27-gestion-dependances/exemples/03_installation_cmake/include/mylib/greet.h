/* ============================================================================
   Section 27.5 : Installation et distribution de librairies sur Linux
   Description : Header public de la bibliothèque installable
   Fichier source : 05-distribution-linux.md
   ============================================================================ */

#pragma once
#include <string>

namespace mylib {
std::string greet(const std::string& name);
}
