# run 
#   $ cmake -S earthren -B \ramdisk\earthren
#   $ make -j4
# commands to build
cmake_minimum_required(VERSION 3.20)

project(MyProject)

set(CMAKE_CXX_STANDARD 17)

# TODO: how to trigger debug-build? What build is the default one debug or release?
# Compiler flags
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/libs)  # TOTO: thre are more of include directories


# TODO: I want to specify a library versons there
find_package(OpenGL REQUIRED)
find_package(SDL2 REQUIRED)
find_package(Boost REQUIRED)
find_package(fmt REQUIRED)
find_package(ImageMagick COMPONENTS Magick++ REQUIRED)
find_package(TIFF REQUIRED)


# Samples

set(BASIC_COMMON_LIBRARIES
	${OPENGL_LIBRARIES} 
	${SDL2_LIBRARIES}
	${Boost_LIBRARIES}
	fmt::fmt)

set(IMAGE_COMMON_LIBRARIES
	ImageMagick::Magick++
	TIFF::TIFF 
	TIFF::CXX)

# xy_plane
add_executable(xy_plane xy_plane.cpp)
target_link_libraries(xy_plane ${BASIC_COMMON_LIBRARIES})

# xy_plane_panzoom
add_executable(xy_plane_panzoom xy_plane_panzoom.cpp)
target_link_libraries(xy_plane_panzoom ${BASIC_COMMON_LIBRARIES} ${IMAGE_COMMON_LIBRARIES})

# xy_plane_grid
add_executable(xy_plane_grid xy_plane_grid.cpp)
target_link_libraries(xy_plane_grid ${BASIC_COMMON_LIBRARIES} ${IMAGE_COMMON_LIBRARIES})

# texture_storage
add_executable(texture_storage texture_storage.cpp)
target_link_libraries(texture_storage ${BASIC_COMMON_LIBRARIES} ${IMAGE_COMMON_LIBRARIES})

# texture_storage_tiff
add_executable(texture_storage_tiff texture_storage_tiff.cpp)
target_link_libraries(texture_storage_tiff ${BASIC_COMMON_LIBRARIES} ${IMAGE_COMMON_LIBRARIES})

# xy_plane_texture
add_executable(xy_plane_texture xy_plane_texture.cpp)
target_link_libraries(xy_plane_texture ${BASIC_COMMON_LIBRARIES} ${IMAGE_COMMON_LIBRARIES})

# xy_plane_grid_textured
add_executable(xy_plane_grid_textured xy_plane_grid_textured.cpp)
target_link_libraries(xy_plane_grid_textured ${BASIC_COMMON_LIBRARIES} ${IMAGE_COMMON_LIBRARIES})