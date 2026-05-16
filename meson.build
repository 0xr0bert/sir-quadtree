project(
    'sir-quadtree',
    'cpp',
    version: '1.0.0',
    default_options: ['warning_level=3', 'cpp_std=c++20']
)

inc = include_directories('src')

sir_quadtree = executable(
    'sir-quadtree',
    sources: [
        'src/main.cpp',
        'src/simulation.cpp',
        'src/quadtree.cpp'
    ],
    include_directories: inc,
    install : false
)