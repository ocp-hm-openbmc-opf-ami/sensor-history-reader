# Unit tests for sensor-reader
#
# Build with:
#   meson setup builddir -Db_coverage=true   (enables gcov coverage)
#   ninja -C builddir test
#
# Coverage report (requires gcovr or lcov):
#   ninja -C builddir coverage          (summary)
#   ninja -C builddir coverage-html     (HTML report in builddir/meson-logs/)

gtest_dep = dependency('gtest_main', version: '>=1.11.0',
                        disabler: true, required: false)
gmock_dep = dependency('gmock', version: '>=1.11.0',
                        disabler: true, required: false)

if not gtest_dep.found() or not gmock_dep.found()
    gtest_proj = subproject('gtest', required: build_tests.enabled())
    if gtest_proj.found()
        gtest_dep = gtest_proj.get_variable('gtest_main_dep').as_system('system')
        gmock_dep = gtest_proj.get_variable('gmock_dep').as_system('system')
    else
        warning('gtest/gmock not found – skipping sensor-reader unit tests')
        subdir_done()
    endif
endif

# Redeclare dependencies that were scoped inside src/meson.build
_phosphor_logging_dep = dependency(
    'phosphor-logging',
    fallback: ['phosphor-logging', 'phosphor_logging_dep'])

_has_json = meson.get_compiler('cpp').has_header_symbol(
    'nlohmann/json.hpp', 'nlohmann::json::string_t', required: false)
_json_dep = _has_json ? declare_dependency() : dependency(
    'nlohmann_json',
    fallback: ['nlohmann_json', 'nlohmann_json_dep'],
    required: true)

_boost_compile_args = [
    '-DBOOST_ASIO_DISABLE_THREADS',
    '-DBOOST_ALL_NO_LIB',
    '-DBOOST_SYSTEM_NO_DEPRECATED',
    '-DBOOST_ERROR_CODE_HEADER_ONLY',
    '-DBOOST_COROUTINES_NO_DEPRECATION_WARNING',
]

_boost_dep = declare_dependency(
    dependencies: dependency('boost', required: false),
    compile_args: _boost_compile_args)

_boost_modules_dep = dependency(
    'boost',
    modules: ['coroutine', 'context', 'serialization', 'filesystem'],
    disabler: true,
    required: false)

_sdeventplus_dep = dependency(
    'sdeventplus', fallback: ['sdeventplus', 'sdeventplus_dep'])
_stdplus_dep = dependency(
    'stdplus', fallback: ['stdplus', 'stdplus_dep'])

# Static library compiled from production sensor_reader.cpp so each test
# executable links the same object code (also enables accurate coverage).
sensor_reader_ut_lib = static_library(
    'sensor_reader_ut',
    '../src/sensor_reader.cpp',
    implicit_include_directories: false,
    include_directories: [
        include_directories('../src'),
        sreader_dbus_includes,
    ],
    dependencies: [
        _json_dep,
        _phosphor_logging_dep,
        sreader_dbus_dep,
        sdbusplus_dep,
        _sdeventplus_dep,
        _stdplus_dep,
        _boost_dep,
        _boost_modules_dep,
        dependency('threads'),
    ],
)

_test_inc = [
    include_directories('.'),
    include_directories('../src'),
    sreader_dbus_includes,
]

_test_link_deps = [
    _json_dep,
    _phosphor_logging_dep,
    sreader_dbus_dep,
    sdbusplus_dep,
    _sdeventplus_dep,
    _stdplus_dep,
    _boost_dep,
    _boost_modules_dep,
    dependency('threads'),
    gtest_dep,
    gmock_dep,
]

# Single unit-test binary aggregating all test suites (matches the
# convention used by the other applications' *_tests binaries).
test('sensor_reader_ut',
    executable('sensor_reader_ut',
        [
            'test_history_read.cpp',
            'test_history_config.cpp',
            'test_history_persistence.cpp',
        ],
        implicit_include_directories: false,
        include_directories: _test_inc,
        link_with: sensor_reader_ut_lib,
        dependencies: _test_link_deps,
    ),
    timeout: 600,
)
