project(
  'sensor-reader',
  'cpp',
  version: '0.1',
  meson_version: '>=0.57.0',
  default_options: [
    'warning_level=3',
    'cpp_std=c++2a',
  ])

default_busname = 'xyz.openbmc_project.SensorReader'

conf_data = configuration_data()
conf_data.set_quoted('DEFAULT_BUSNAME', default_busname)

sdbusplus_dep = dependency('sdbusplus', required: false)
if sdbusplus_dep.found() and sdbusplus_dep.type_name() != 'internal'
  sdbusplusplus_prog = find_program('sdbus++', native: true)
  sdbuspp_gen_meson_prog = find_program('sdbus++-gen-meson', native: true)
else
  sdbusplus_proj = subproject('sdbusplus', required: true)
  sdbusplus_dep = sdbusplus_proj.get_variable('sdbusplus_dep')
  sdbusplusplus_prog = sdbusplus_proj.get_variable('sdbusplusplus_prog')
  sdbuspp_gen_meson_prog = sdbusplus_proj.get_variable('sdbuspp_gen_meson_prog')
endif

generated_sources = []
generated_others = []
yaml_sources = []
subdir('gen')
subdir('gen/xyz')
sreader_dbus_includes = include_directories('gen')

sreader_dbus_deps = [
  sdbusplus_dep,
]

sreader_dbus_lib = static_library(
  'sreader_dbus',
  generated_sources,
  implicit_include_directories: false,
  include_directories: sreader_dbus_includes,
  dependencies: sreader_dbus_deps)

generated_files_headers = []
foreach s : generated_sources
    foreach f : s.to_list()
        p = f.full_path()
        if p.endswith('.hpp')
            generated_files_headers += f
        endif
    endforeach
endforeach

sreader_dbus_dep = declare_dependency(
  sources: generated_files_headers,
  include_directories: sreader_dbus_includes,
  link_with: sreader_dbus_lib,
  dependencies: sreader_dbus_deps)

subdir('src')

configure_file(
  input: 'xyz.openbmc_project.SensorReader.service.in',
  output: 'xyz.openbmc_project.SensorReader.service',
  configuration: {
    'SYSTEMD_TARGET': 'multi-user.target',
    'DEFAULT_BUSNAME': default_busname,
  },
  install: true,
  install_dir: dependency('systemd').get_variable(
    pkgconfig: 'systemdsystemunitdir'))
