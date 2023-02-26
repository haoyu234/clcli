project(
    'columns', 
    ['c', 'cpp'],
    version: 'v0.0.1',
    default_options: [
        'c_std=gnu89',
        'cpp_std=c++11'
    ])

columns = static_library(
    'columns',
    sources: 'src/columns.c',
    include_directories: 'include'
)

executable(
    'example',
    sources: [
        'example/decoder.c',
        'example/encoder.c',
        'example/internal.c',
        'example/main.c',
    ],
    include_directories: 'include',
    dependencies: dependency('msgpack'),
    link_with: columns,
)

llvm_include_dir = run_command('llvm-config', '--includedir', check: true)

executable(
    'clcli',
    sources: [
        'clcli/main.cpp',
        'clcli/visit.cpp',
        'clcli/util.cpp',
        'clcli/builder.cpp',
    ],
    include_directories: [
        'include',
        llvm_include_dir.stdout().strip()
    ],
    dependencies: [
        dependency('Clang', modules: ['libclang']),
        dependency('fmt'),
    ]
)
