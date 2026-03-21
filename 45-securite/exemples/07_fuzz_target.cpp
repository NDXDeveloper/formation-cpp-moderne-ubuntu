/* ============================================================================
   Section 45.5.2 : LibFuzzer et integration
   Description : Fuzz target minimal pour LibFuzzer (Clang uniquement)
   Fichier source : 05.2-libfuzzer.md
   ============================================================================ */

// Compilation :
//   clang++ -std=c++20 -fsanitize=fuzzer,address -o fuzz_test 07_fuzz_target.cpp
// Execution :
//   ./fuzz_test            (fuzzing continu)
//   ./fuzz_test corpus/    (avec corpus)

#include <cstdint>
#include <cstddef>

// Fonction a fuzzer : verifie si les donnees commencent par "PNG"
bool parse_header(const uint8_t* data, size_t size) {
    if (size < 4) return false;
    if (data[0] != 0x89) return false;
    if (data[1] != 'P') return false;
    if (data[2] != 'N') return false;
    if (data[3] != 'G') return false;
    return true;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    parse_header(data, size);
    return 0;
}
