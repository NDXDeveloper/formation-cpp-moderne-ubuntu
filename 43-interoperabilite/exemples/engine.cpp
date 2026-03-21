/* ============================================================================
   Section 43.1 : C++ et C — extern "C" et ABI Compatibility
   Description : Implementation du handle opaque engine (code C++ interne)
   Fichier source : 01-cpp-et-c.md
   ============================================================================ */

#include "engine.h"
#include <string>
#include <vector>

struct engine {
    std::string               config_path;
    std::vector<uint8_t>      data;
    std::string               last_error;
};

extern "C" engine_t* engine_create(const char* config_path) {
    try {
        auto* e = new engine();
        e->config_path = config_path;
        return e;
    } catch (const std::exception& ex) {
        return nullptr;
    }
}

extern "C" void engine_destroy(engine_t* e) {
    delete e;
}

extern "C" int engine_load_data(engine_t* e, const uint8_t* buf, size_t len) {
    if (!e || !buf) return -1;
    try {
        e->data.assign(buf, buf + len);
        return 0;
    } catch (const std::exception& ex) {
        e->last_error = ex.what();
        return -1;
    }
}

extern "C" double engine_compute(engine_t* e) {
    if (!e || e->data.empty()) return 0.0;
    try {
        double sum = 0.0;
        for (auto byte : e->data) sum += byte;
        return sum / static_cast<double>(e->data.size());
    } catch (const std::exception& ex) {
        e->last_error = ex.what();
        return 0.0;
    }
}

extern "C" const char* engine_last_error(const engine_t* e) {
    if (!e) return "null engine";
    return e->last_error.c_str();
}
