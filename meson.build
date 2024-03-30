project(
  'game-of-life',
  'cpp',
  version: '1.0',
  license: 'AGPLv3',
  default_options: [
    'cpp_std=c++17',
  ],
)
gnt_dep = dependency('gnt-0.1')
ggb_dep = dependency('ggb-0.1')
gtk_dep = dependency('gtk4')
deps = [gtk_dep, gnt_dep, ggb_dep]

executable(
  'game-of-life',
  'main.cpp',
  dependencies: deps,
)
