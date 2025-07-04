cmake_minimum_required(VERSION 3.10)
project(fragment_reduce LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(SDL2 REQUIRED)
find_package(OpenGL REQUIRED)
find_package(ImageMagick COMPONENTS Magick++ REQUIRED)

add_executable(reducer
	reducer.cpp
	shader.cpp
	fs.cpp
)

target_include_directories(reducer PRIVATE
	${SDL2_INCLUDE_DIRS}
	${OPENGL_INCLUDE_DIR}
	.
)

target_link_libraries(reducer
	${SDL2_LIBRARIES}
	${OPENGL_gl_LIBRARY}
	GLESv2
	fmt
	stdc++fs
	ImageMagick::Magick++
)
