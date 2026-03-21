/* ============================================================================
   Section 45.1 : Buffer overflows et protection
   Description : std::format vs sprintf — formatage sur sans overflow
   Fichier source : 01-buffer-overflows.md
   ============================================================================ */

#include <string>
#include <string_view>
#include <format>
#include <print>

void greet_modern(std::string_view name) {
    std::string message = std::format("Bonjour, {} !", name);
    std::print("{}\n", message);
}

int main() {
    greet_modern("Nicolas");
    greet_modern("Un nom tres long qui aurait provoque un buffer overflow avec sprintf");
}
