/* ============================================================================
   Section 43.1.7 : Bibliotheques dynamiques et dlopen/dlsym
   Description : Application hote — charge et utilise le plugin via dlopen
   Fichier source : 01-cpp-et-c.md
   ============================================================================ */

#include "plugin_interface.h"
#include <dlfcn.h>
#include <cstdio>

int main() {
    void* handle = dlopen("./libplugin.so", RTLD_LAZY);
    if (!handle) {
        std::fprintf(stderr, "dlopen failed: %s\n", dlerror());
        return 1;
    }

    auto create  = reinterpret_cast<plugin_t*(*)()>(dlsym(handle, "plugin_create"));
    auto destroy = reinterpret_cast<void(*)(plugin_t*)>(dlsym(handle, "plugin_destroy"));
    auto execute = reinterpret_cast<int(*)(plugin_t*, const char*, char*, size_t)>(
                       dlsym(handle, "plugin_execute"));
    auto name    = reinterpret_cast<const char*(*)()>(dlsym(handle, "plugin_name"));

    if (!create || !destroy || !execute || !name) {
        std::fprintf(stderr, "dlsym failed: %s\n", dlerror());
        dlclose(handle);
        return 1;
    }

    std::printf("Loaded plugin: %s\n", name());

    plugin_t* p = create();
    char output[256];
    int rc = execute(p, "hello world", output, sizeof(output));
    if (rc == 0) {
        std::printf("Result: %s\n", output);
    }

    destroy(p);
    dlclose(handle);
    return 0;
}
