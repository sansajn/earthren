cmake_minimum_required(VERSION 3.10)
project(HeightMapReducerES)

set(CMAKE_CXX_STANDARD 17)

# Find packages
find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2 REQUIRED sdl2)
pkg_check_modules(GLES REQUIRED glesv2)

# Create executable
add_executable(HeightMapReducerES main.cpp)

# Link libraries
target_link_libraries(HeightMapReducerES
    ${SDL2_LIBRARIES}
    ${GLES_LIBRARIES}
)

# Include directories
target_include_directories(HeightMapReducerES PRIVATE
    ${SDL2_INCLUDE_DIRS}
    ${GLES_INCLUDE_DIRS}
)

# Compiler flags
target_compile_options(HeightMapReducerES PRIVATE ${SDL2_CFLAGS_OTHER})
