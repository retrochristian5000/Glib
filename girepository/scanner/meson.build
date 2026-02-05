giscanner_conf_data = configuration_data()
giscanner_conf_data.set('VERSION', meson.project_version())
giscanner_version_py = configure_file(
  input: '_version.py.in',
  output: '_version.py',
  configuration: giscanner_conf_data,
)

scanner_res = import('gnome').compile_resources('scanner_resources', 'scanner.gresource.xml',
  c_name: 'scanner',
  dependencies: giscanner_version_py
)

flex = find_program('flex', 'win_flex')
bison = find_program('bison', 'win_bison')

scannerparser = custom_target('scannerparser',
    input: 'scannerparser.y',
   output: ['scannerparser.c', 'scannerparser.h'],
  command: [bison, '@INPUT@', '--defines=@OUTPUT1@', '--output=@OUTPUT0@']

)

scannerlexer = custom_target('scannerlexer',
    input: 'scannerlexer.l',
   output: 'scannerlexer.c',
  command: [flex, '-o', '@OUTPUT@', '@INPUT@']
)

giscanner_args = [
  '-DGI_COMPILATION',
  '-DGIR_DIR="@0@"'.format(glib_girdir),
]

if not cc.has_header('unistd.h')
  giscanner_args += '-DYY_NO_UNISTD_H'
endif

if cc.get_id() != 'msvc'
  giscanner_args += cc.get_supported_arguments([
    '-Wno-missing-field-initializers',
    '-Wno-unused-parameter',
    '-Wno-misleading-indentation',
  ])
endif

python_dep = python.dependency(embed: true)

serpent_lib = static_library('serpent', 'serpent.c',
  dependencies: [
    libglib_dep,
    libgio_dep,
    python_dep,
  ]
)

gi_generate_repo_bin = executable('gi-generate-repository',
  sources: [
    'gi-generate-repository.c',
    scanner_res,
    'giscannermodule.c',
    gi_visibility_h,
    'sourcescanner.c',
    scannerparser,
    scannerlexer,
  ],
  c_args: giscanner_args,
  include_directories: configinc,
  link_with: serpent_lib,
  dependencies: [
    libglib_dep,
    libgobject_dep,
    libgio_dep,
    libgmodule_dep,
    python_dep,
  ],
  install: true
)

meson.override_find_program('gi-generate-repository', gi_generate_repo_bin)
