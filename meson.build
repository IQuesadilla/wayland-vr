project('wayvr', 'c')

wayvr_asset_dir = get_option('prefix') / get_option('datadir') / 'wayvr'
add_project_arguments(['-DWAYVR_ASSET_DIR="' + wayvr_asset_dir + '"'], language: 'c')
#add_global_arguments(['-DSDL_MAIN_USE_CALLBACKS'], language: 'c')

sdl3_dep = dependency('sdl3')
sdl3_ttf_dep = dependency('sdl3-ttf')
cglm_dep = dependency('cglm')

install_subdir('assets',
               install_dir: wayvr_asset_dir)

executable('wayvr_console',
  ['console_app/main.c', 'console_app/console.c'],
  dependencies: [sdl3_dep, sdl3_ttf_dep],
  install: true
)

executable('wayvr',
  ['AppInit.c', 'AppEvent.c', 'AppIterate.c', 'displays.c', 'window.c', 'cam2d.c'],
  dependencies: [sdl3_dep, cglm_dep],
  link_args: ['-lm'],
  install: true
)
