cmake_minimum_required(VERSION 3.10)
project(HeightMapReducer)

set(CMAKE_CXX_STANDARD 17)

# Find packages
find_package(OpenGL REQUIRED)
find_package(glfw3 REQUIRED)
find_package(GLEW REQUIRED)

# Create executable
add_executable(HeightMapReducer main.cpp)

# Link libraries
target_link_libraries(HeightMapReducer
    OpenGL::GL
    glfw
    GLEW::GLEW
)

# Include directories
target_include_directories(HeightMapReducer PRIVATE ${OPENGL_INCLUDE_DIRS})
