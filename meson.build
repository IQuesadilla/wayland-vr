project('sdldemo', 'c')

sdl3_dep = dependency('sdl3')
sdl3_ttf_dep = dependency('sdl3-ttf')

executable('console',
  ['main.c', 'console.c'],
  dependencies: [sdl3_dep, sdl3_ttf_dep]
)
