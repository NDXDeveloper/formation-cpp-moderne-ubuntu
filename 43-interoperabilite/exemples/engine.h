/* ============================================================================
   Section 43.1 : C++ et C — extern "C" et ABI Compatibility
   Description : Header bilingue C/C++ — API publique du handle opaque engine
   Fichier source : 01-cpp-et-c.md
   ============================================================================ */

#ifndef ENGINE_H
#define ENGINE_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct engine engine_t;

/* Cycle de vie */
engine_t*   engine_create(const char* config_path);
void        engine_destroy(engine_t* e);

/* Operations */
int         engine_load_data(engine_t* e, const uint8_t* buf, size_t len);
double      engine_compute(engine_t* e);

/* Introspection */
const char* engine_last_error(const engine_t* e);

#ifdef __cplusplus
}
#endif

#endif /* ENGINE_H */
