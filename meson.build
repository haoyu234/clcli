project(
    'clcli', 
    ['c', 'cpp'],
    version: 'v0.0.1',
    default_options: 'cpp_std=c++11'
)

llvm_include_dir = run_command('llvm-config', '--includedir', check: true)

executable(
    'clcli',
    sources: [
        'src/main.cpp',
        'src/visit.cpp',
        'src/util.cpp',
        'src/builder.cpp',
    ],
    include_directories: llvm_include_dir.stdout().strip(),
    dependencies: [
        dependency('Clang', modules: ['libclang']),
        dependency('fmt'),
    ]
)

