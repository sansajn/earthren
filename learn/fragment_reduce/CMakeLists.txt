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

# Optionally, if you use pkg-config for SDL2:
# find_package(PkgConfig REQUIRED)
# pkg_check_modules(SDL2 REQUIRED sdl2)
# target_include_directories(reducer PRIVATE ${SDL2_INCLUDE_DIRS})
# target_link_libraries(reducer ${SDL2_LIBRARIES})

# If fmt is not found automatically, you may need to add it manually or use FetchContent.
