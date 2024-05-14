phosphor_logging_dep = dependency(
  'phosphor-logging',
  fallback: ['phosphor-logging', 'phosphor_logging_dep'])

src_includes = include_directories('.')

json_dep = declare_dependency()
  # nlohmann_json might not have a pkg-config. It is header only so just make
  # sure we can access the needed symbols from the header.
  has_json = meson.get_compiler('cpp').has_header_symbol(
    'nlohmann/json.hpp',
    'nlohmann::json::string_t',
    required: false)
  if not has_json
    json_dep = dependency(
      'nlohmann_json',
      fallback: ['nlohmann_json', 'nlohmann_json_dep'],
      required: true)
  endif

sensorreader_deps = [
  json_dep,
  phosphor_logging_dep,
  sreader_dbus_dep,
  sdbusplus_dep,
  dependency('sdeventplus', fallback: ['sdeventplus', 'sdeventplus_dep']),
  dependency('stdplus', fallback: ['stdplus', 'stdplus_dep']),
]

sensorreader_dep = declare_dependency(
  dependencies: sensorreader_deps,
  include_directories: src_includes
  )

boost_compile_args = [
    '-DBOOST_ASIO_DISABLE_THREADS',
    '-DBOOST_ALL_NO_LIB',
    '-DBOOST_SYSTEM_NO_DEPRECATED',
    '-DBOOST_ERROR_CODE_HEADER_ONLY',
    '-DBOOST_COROUTINES_NO_DEPRECATION_WARNING',
]

boost_dep = declare_dependency(
    dependencies: dependency('boost', required: false),
    compile_args: boost_compile_args)

executable(
  'sensor-reader',
  'sensor_reader_main.cpp',
  'sensor_reader.cpp',
  implicit_include_directories: false,
  dependencies: [
        boost_dep,
        dependency(
            'boost',
            modules: ['coroutine', 'context', 'serialization', 'filesystem'],
            disabler: true,
            required: false,
        ),
        sensorreader_dep,
        dependency('threads'),
        ],
  install: true,
  install_dir: get_option('bindir'))
