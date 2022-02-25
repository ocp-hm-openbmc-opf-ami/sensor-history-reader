# Generated file; do not modify.
subdir('Read')
generated_others += custom_target(
    'xyz/openbmc_project/SensorReader/History/Read__markdown'.underscorify(),
    input: [ '../../../../../yaml/xyz/openbmc_project/SensorReader/History/Read.interface.yaml',  ],
    output: [ 'Read.md' ],
    command: [
        sdbuspp_gen_meson_prog, '--command', 'markdown',
        '--output', meson.current_build_dir(),
        '--tool', sdbusplusplus_prog,
        '--directory', meson.current_source_dir() / '../../../../../yaml',
        'xyz/openbmc_project/SensorReader/History/Read',
    ],
)
