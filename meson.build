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
        'example/main.c',
    ],
    include_directories: 'include',
    dependencies: dependency('msgpack'),
    link_with: columns,
)

executable(
    'clcli',
    sources: [
        'clcli/main.cpp',
        'clcli/visit.cpp',
        'clcli/util.cpp',
    ],
    include_directories: 'include',
    dependencies: [
        dependency('Clang', modules: ['libclang']),
        dependency('fmt'),
    ]
)
