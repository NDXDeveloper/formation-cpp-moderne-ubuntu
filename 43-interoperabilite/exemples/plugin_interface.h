/* ============================================================================
   Section 43.1.7 : Bibliotheques dynamiques et dlopen/dlsym
   Description : Interface commune pour les plugins (header bilingue C/C++)
   Fichier source : 01-cpp-et-c.md
   ============================================================================ */

#ifndef PLUGIN_INTERFACE_H
#define PLUGIN_INTERFACE_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct plugin plugin_t;

plugin_t*   plugin_create(void);
void        plugin_destroy(plugin_t* p);
int         plugin_execute(plugin_t* p, const char* input, char* output, size_t out_size);
const char* plugin_name(void);
int         plugin_version(void);

#ifdef __cplusplus
}
#endif

#endif
