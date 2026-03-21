/* ============================================================================
   Section 43.1.7 : Bibliotheques dynamiques et dlopen/dlsym
   Description : Plugin uppercase — compile en .so, charge par dlopen
   Fichier source : 01-cpp-et-c.md
   ============================================================================ */

#include "plugin_interface.h"
#include <string>
#include <algorithm>

struct plugin {
    std::string name = "uppercase_plugin";
    int version = 1;
};

extern "C" plugin_t* plugin_create(void) {
    return new plugin();
}

extern "C" void plugin_destroy(plugin_t* p) {
    delete p;
}

extern "C" const char* plugin_name(void) {
    return "uppercase_plugin";
}

extern "C" int plugin_version(void) {
    return 1;
}

extern "C" int plugin_execute(plugin_t* p, const char* input,
                               char* output, size_t out_size) {
    if (!p || !input || !output) return -1;
    std::string result(input);
    std::transform(result.begin(), result.end(), result.begin(), ::toupper);
    if (result.size() + 1 > out_size) return -2;
    std::copy(result.begin(), result.end(), output);
    output[result.size()] = '\0';
    return 0;
}
