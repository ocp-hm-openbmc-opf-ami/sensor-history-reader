project(
  'sensor-reader',
  'cpp',
  version: '0.1',
  meson_version: '>=1.1.1',
  default_options: [
    'warning_level=3',
    'cpp_std=c++23',
  ])

default_busname = 'xyz.openbmc_project.SensorReader'

conf_data = configuration_data()
conf_data.set_quoted('DEFAULT_BUSNAME', default_busname)

if get_option('tests').enabled()
  sdbusplus_dep = subproject('sdbusplus').get_variable('sdbusplus_dep')
  sdbusplusplus_prog = subproject('sdbusplus').get_variable('sdbusplusplus_prog')
  sdbuspp_gen_meson_prog = subproject('sdbusplus').get_variable('sdbuspp_gen_meson_prog')
  meson.override_dependency('sdbusplus',sdbusplus_dep)
  phosphor_dbus_interfaces_dep = dependency('phosphor-dbus-interfaces-ami')
  phosphor_logging_dep = subproject('phosphor-logging').get_variable('phosphor_logging_dep')

    cmake = import('cmake')
    opt = cmake.subproject_options()
    boost_libs = [
        'asio',
        'context',
        'coroutine',
        'filesystem',
        'headers',
        'serialization',
    ]
    opt.add_cmake_defines(
        {
            'CMAKE_CXX_FLAGS': '-Wno-uninitialized',
            'BOOST_INCLUDE_LIBRARIES': ';'.join(boost_libs),
            'BUILD_SHARED_LIBS': 'OFF',
        },
    )

    boost_proj = cmake.subproject('boost', required: true, options: opt)
    boost_dep = []
    foreach boost_lib : boost_libs
        boost_dep += [boost_proj.dependency('boost_' + boost_lib).as_system()]
    endforeach
else
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

build_tests = get_option('tests')
if build_tests.enabled()
  subdir('tests')
endif

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
