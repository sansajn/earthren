# run 
#   $ cmake -S earthren -B \ramdisk\earthren
#   $ make -j4
# commands to build
cmake_minimum_required(VERSION 3.20)

project(MyProject)

set(CMAKE_CXX_STANDARD 20)

# TODO: how to trigger debug-build? What build is the default one debug or release?
# Compiler flags
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/libs)  # TOTO: thre are more of include directories


# TODO: I want to specify a library versons as well there
find_package(OpenGL REQUIRED)
find_package(SDL2 REQUIRED)
find_package(Boost REQUIRED COMPONENTS filesystem)
find_package(fmt REQUIRED)
find_package(ImageMagick REQUIRED COMPONENTS Magick++)
find_package(TIFF REQUIRED)


# Samples

set(BASIC_COMMON_LIBRARIES
	${OPENGL_LIBRARIES} 
	${SDL2_LIBRARIES}
	Boost::filesystem
	fmt::fmt)

set(IMAGE_COMMON_LIBRARIES
	ImageMagick::Magick++
	TIFF::TIFF 
	TIFF::CXX)

# WITH_IMAGE
set(WITH_IMAGE
	${BASIC_COMMON_LIBRARIES}
	${IMAGE_COMMON_LIBRARIES})

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

# map_camera
add_executable(map_camera map_camera.cpp)
target_link_libraries(map_camera ${BASIC_COMMON_LIBRARIES} ${IMAGE_COMMON_LIBRARIES})

# height_sinxy
add_executable(height_sinxy height_sinxy.cpp)
target_link_libraries(height_sinxy ${BASIC_COMMON_LIBRARIES} ${IMAGE_COMMON_LIBRARIES})

# plot_sinxy
add_executable(plot_sinxy plot_sinxy.cpp)
target_link_libraries(plot_sinxy ${BASIC_COMMON_LIBRARIES} ${IMAGE_COMMON_LIBRARIES})

# height_sinxy_map
add_executable(height_sinxy_map height_sinxy_map.cpp)
target_link_libraries(height_sinxy_map ${BASIC_COMMON_LIBRARIES} ${IMAGE_COMMON_LIBRARIES})

# height_sinxy_normals
add_executable(height_sinxy_normals height_sinxy_normals.cpp camera.cpp)
target_link_libraries(height_sinxy_normals ${BASIC_COMMON_LIBRARIES} ${IMAGE_COMMON_LIBRARIES})

# height_sinxy_normals_fce
add_executable(height_sinxy_normals_fce height_sinxy_normals_fce.cpp camera.cpp)
target_link_libraries(height_sinxy_normals_fce ${WITH_IMAGE})

# sinxy_heights
add_executable(sinxy_heights sinxy_heights.cpp)
target_link_libraries(sinxy_heights ${WITH_IMAGE})

# height_map
add_executable(height_map height_map.cpp camera.cpp free_camera.cpp)
target_link_libraries(height_map ${WITH_IMAGE})

# normals
add_executable(normals normals.cpp camera.cpp)
target_link_libraries(normals ${WITH_IMAGE})

# geoms_plane
add_executable(geoms_plane geoms_plane.cpp camera.cpp)
target_link_libraries(geoms_plane ${WITH_IMAGE})

# gs_triangle_broken
add_executable(gs_triangle_broken gs_triangle_broken.cpp camera.cpp free_camera.cpp)
target_link_libraries(gs_triangle_broken ${BASIC_COMMON_LIBRARIES})

# terrain_quad
add_executable(terrain_quad terrain_quad.cpp camera.cpp free_camera.cpp)
target_link_libraries(terrain_quad ${WITH_IMAGE})

# satellite_map
add_executable(satellite_map satellite_map.cpp camera.cpp free_camera.cpp texture.cpp shader.cpp tiff.cpp)
target_link_libraries(satellite_map ${WITH_IMAGE})

# basic raw-opengl samples

# triangle sample
add_executable(triangle triangle.cpp shader.cpp)
target_link_libraries(triangle ${BASIC_COMMON_LIBRARIES})

# FBO render samples
add_executable(fbo_render fbo_render.cpp shader.cpp)
target_link_libraries(fbo_render ${WITH_IMAGE})

add_executable(reduce_shader reduce_shader.cpp shader.cpp)
target_link_libraries(reduce_shader ${WITH_IMAGE})


# ImGUI static library
file(GLOB IMGUI_SRC
	${CMAKE_SOURCE_DIR}/imgui/*.cpp
	${CMAKE_SOURCE_DIR}/imgui/examples/imgui_impl_sdl.cpp
	${CMAKE_SOURCE_DIR}/imgui/examples/imgui_impl_opengl3.cpp)

add_library(imgui STATIC ${IMGUI_SRC})
target_include_directories(imgui PUBLIC
	${CMAKE_SOURCE_DIR}/imgui
	${CMAKE_SOURCE_DIR}/imgui/examples)

target_include_directories(imgui PRIVATE ${SDL2_INCLUDE_DIRS})
target_compile_definitions(imgui PUBLIC IMGUI_IMPL_OPENGL_ES3)

# Suppress deprecated-enum-enum-conversion warnings for ImGUI
if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
	target_compile_options(imgui PRIVATE -Wno-deprecated-enum-enum-conversion)
endif()

# height_scale sample
set(HEIGHT_SCALE_SOURCES
	camera.cpp
	free_camera.cpp
	texture.cpp
	shader.cpp
	tiff.cpp
	fs.cpp)

add_executable(height_scale height_scale.cpp ${HEIGHT_SCALE_SOURCES})
target_link_libraries(height_scale ${WITH_IMAGE} imgui)



# four_terrain sample
add_executable(four_terrain
    four_terrain.cpp
    camera.cpp free_camera.cpp texture.cpp shader.cpp tiff.cpp fs.cpp
    flat_shader.cpp quad.cpp axis_model.cpp four_terrain_ui.cpp four_terrain_shader_program.cpp set_uniform.cpp
)
target_link_libraries(four_terrain ${WITH_IMAGE} imgui)

# height_overlap sample
add_executable(height_overlap
    height_overlap.cpp
    camera.cpp free_camera.cpp texture.cpp shader.cpp tiff.cpp fs.cpp
    flat_shader.cpp quad.cpp axis_model.cpp four_terrain_ui.cpp height_overlap_shader_program.cpp set_uniform.cpp
)
target_link_libraries(height_overlap ${WITH_IMAGE} imgui)

# terrain_mesh sample
add_executable(terrain_mesh
    terrain_mesh.cpp
    camera.cpp free_camera.cpp texture.cpp shader.cpp tiff.cpp fs.cpp
    flat_shader.cpp quad.cpp axis_model.cpp four_terrain_ui.cpp height_overlap_shader_program.cpp set_uniform.cpp
)
target_link_libraries(terrain_mesh ${WITH_IMAGE} imgui)

# tile_grid sample
add_executable(tile_grid tile_grid.cpp)
target_link_libraries(tile_grid ${WITH_IMAGE})

# terrain_scale sample
add_executable(terrain_scale
    terrain_scale.cpp
    camera.cpp free_camera.cpp texture.cpp shader.cpp tiff.cpp fs.cpp
    flat_shader.cpp quad.cpp axis_model.cpp terrain_scale_ui.cpp height_overlap_shader_program.cpp set_uniform.cpp
)
target_link_libraries(terrain_scale ${WITH_IMAGE} imgui)

# above_terrain sample
add_executable(above_terrain
    above_terrain.cpp
    free_camera.cpp texture.cpp shader.cpp tiff.cpp fs.cpp
    flat_shader.cpp quad.cpp axis_model.cpp terrain_scale_ui.cpp height_overlap_shader_program.cpp
    above_terrain_outline_shader_program.cpp set_uniform.cpp
)
target_link_libraries(above_terrain ${WITH_IMAGE} imgui)

# generate_dump sample
add_executable(generate_dump generate_dump.cpp)
target_link_libraries(generate_dump ${BASIC_COMMON_LIBRARIES})

# grid_of_terrains sample
add_executable(grid_of_terrains
    grid_of_terrains.cpp
    free_camera.cpp texture.cpp shader.cpp tiff.cpp fs.cpp
    axis_model.cpp flat_shader.cpp terrain_scale_ui.cpp height_overlap_shader_program.cpp
    above_terrain_outline_shader_program.cpp set_uniform.cpp
    quad.cpp grid_of_terrains_lightdir_shader_program.cpp terrain_grid.cpp terrain_camera.cpp
)
target_link_libraries(grid_of_terrains ${WITH_IMAGE} imgui)

# more_details sample
add_executable(more_details
    more_details.cpp
    more_details_terrain_grid.cpp
    free_camera.cpp texture.cpp shader.cpp tiff.cpp fs.cpp
    axis_model.cpp flat_shader.cpp terrain_scale_ui.cpp height_overlap_shader_program.cpp
    above_terrain_outline_shader_program.cpp set_uniform.cpp
    quad.cpp grid_of_terrains_lightdir_shader_program.cpp terrain_camera.cpp
)
target_link_libraries(more_details ${WITH_IMAGE} imgui)

# lod_tiles sample
add_executable(lod_tiles
    lod_tiles.cpp
    lod_tiles_terrain_grid.cpp
    lod_tiles_user_input.cpp
    lod_tiles_draw_terrain.cpp
    free_camera.cpp texture.cpp shader.cpp tiff.cpp fs.cpp
    axis_model.cpp flat_shader.cpp terrain_scale_ui.cpp height_overlap_shader_program.cpp
    above_terrain_outline_shader_program.cpp set_uniform.cpp
    quad.cpp grid_of_terrains_lightdir_shader_program.cpp terrain_camera.cpp
)
target_link_libraries(lod_tiles ${WITH_IMAGE} imgui)

# Add ogl static library
file(GLOB OGL_SRC ${CMAKE_SOURCE_DIR}/libs/ogl/*.cpp)
add_library(ogl STATIC ${OGL_SRC})
target_include_directories(ogl PUBLIC ${CMAKE_SOURCE_DIR}/libs/ogl)

# lod_camera sample
add_executable(lod_camera
    lod_camera.cpp
    lod_camera_terrain_grid.cpp
    lod_tiles_user_input.cpp
    lod_camera_draw_terrain.cpp
    shape.cpp shape_mesh.cpp shade_shader.cpp mesh_draw.cpp
    free_camera.cpp texture.cpp shader.cpp tiff.cpp fs.cpp
    axis_model.cpp flat_shader.cpp terrain_scale_ui.cpp height_overlap_shader_program.cpp
    above_terrain_outline_shader_program.cpp set_uniform.cpp
    quad.cpp grid_of_terrains_lightdir_shader_program.cpp terrain_camera.cpp
)
target_link_libraries(lod_camera ${WITH_IMAGE} imgui ogl)
