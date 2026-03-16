# src/meson.build
greeter_lib = library('greeter',
  'greeter.cpp',
  include_directories : include_directories('.'),
  install : false,
)

greeter_dep = declare_dependency(
  link_with : greeter_lib,
  include_directories : include_directories('.'),
)
