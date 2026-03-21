/* ============================================================================
   Section 43.1.4 : Le pattern du handle opaque
   Description : Test du handle opaque engine via l'interface C
   Fichier source : 01-cpp-et-c.md
   ============================================================================ */

#include "engine.h"
#include <cstdio>
#include <cstdint>

int main() {
    engine_t* e = engine_create("/etc/myapp.conf");
    if (!e) {
        std::printf("Failed to create engine\n");
        return 1;
    }

    uint8_t data[] = {10, 20, 30, 40, 50};
    int rc = engine_load_data(e, data, sizeof(data));
    std::printf("load_data: %d\n", rc);  // 0

    double avg = engine_compute(e);
    std::printf("compute (average): %.1f\n", avg);  // 30.0

    engine_destroy(e);
    std::printf("Engine destroyed\n");
    return 0;
}
