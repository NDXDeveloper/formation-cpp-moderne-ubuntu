/* ============================================================================
   Section 28.4 : Meson — Build system montant
   Description : Header de la bibliothèque greeter (projet Meson)
   Fichier source : 04-meson.md, 04.1-syntaxe-meson.md
   ============================================================================ */

#pragma once
#include <string>

namespace greeter {
std::string greet(const std::string& name);
}
