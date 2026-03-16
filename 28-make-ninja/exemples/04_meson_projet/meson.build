# ============================================================================
# Section 28.4 : Meson — Build system montant
# Description : Projet Meson complet illustrant project(), library(),
#               declare_dependency(), executable(), et subdir()
# Fichier source : 04-meson.md, 04.1-syntaxe-meson.md
# ============================================================================

project('greeter_demo', 'cpp',
  version : '1.0.0',
  default_options : [
    'cpp_std=c++17',
    'warning_level=3',
    'buildtype=release',
  ]
)

subdir('src')

executable('demo', 'main.cpp',
  dependencies : greeter_dep,
)
