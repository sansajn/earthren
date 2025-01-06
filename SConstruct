# \note The problem with this setup is that we can't have diferrent library 
# version dependencies for different targets and also to build any of the 
# target all dependencies needs to be met.

AddOption('--build-debug', action='store_true', dest='build_debug', default=False)


def build():
	cpp20_env = Environment(
	   CXXFLAGS=['-std=c++20'], CCFLAGS=['-Wall', '-Wextra'],
		LIBS = [
			'boost_stacktrace_backtrace', 'dl', 'backtrace',  # to suport stack trace
			'tiffxx', 'boost_filesystem'],
		CPPDEFINES=[
			'BOOST_STACKTRACE_USE_BACKTRACE',  # requires 'boost_stacktrace_backtrace', 'dl' and 'backtrace'
			'IMGUI_IMPL_OPENGL_ES3'],  # ImGUI OpenGL ES3 backand
		CPPPATH=['.', 'libs/',
			'imgui/', 'imgui/examples/'])

	if GetOption('build_debug'):
		cpp20_env.Append(CCFLAGS=['-ggdb3', '-O0', '-D_DEBUG'])
	#else:  # release
	#	cpp20_env.Append(CCFLAGS=['-O2', '-DNDEBUG'])

	# for OpenGL samples

	# List of pkg-config based library dependencies as (library, version) pair or just package name as string (e.g. ('libzmq', '>= 4.3.0') pair or 'libzmq' string). Library package version can be found by running `pkg-config --modversion LIBRARY` command.
	deps = [
		('fmt', '>= 8.1.1'),
		('glm', '>= 0.9.9.5'), # libglm-dev
		('sdl2', '>= 2.0.20'), # libsdl2-dev
		('glesv2', '>= 3.2'),
		('Magick++', '>= 6.9.11'),
		('libtiff-4', '>= 4.3.0'),  # libtiff-dev
		('spdlog', '>= 1.9.2'),  # libspdlog-dev
		('fmt', '>= 6.1.2')  # libfmt-dev
		# libboost-dev (Boost.GIL, ...)
	]

	env = cpp20_env.Clone()
	env = configure(env, deps)

	env.Program(['xy_plane.cpp'])
	env.Program(['xy_plane_panzoom.cpp'])
	env.Program(['xy_plane_grid.cpp'])
	env.Program(['texture_storage.cpp'])
	env.Program(['texture_storage_tiff.cpp'])
	env.Program(['xy_plane_texture.cpp'])
	env.Program(['xy_plane_grid_textured.cpp'])
	env.Program(['map_camera.cpp'])
	env.Program(['height_sinxy.cpp'])
	env.Program(['plot_sinxy.cpp'])
	env.Program(['height_sinxy_map.cpp'])
	env.Program(['height_sinxy_normals.cpp', 'camera.cpp'])
	env.Program(['height_sinxy_normals_fce.cpp', 'camera.cpp'])
	env.Program(['sinxy_heights.cpp'])
	env.Program(['height_map.cpp', 'camera.cpp', 'free_camera.cpp'])
	env.Program(['normals.cpp', 'camera.cpp'])
	env.Program(['geoms_plane.cpp', 'camera.cpp'])
	env.Program(['gs_triangle_broken.cpp', 'camera.cpp', 'free_camera.cpp'])
	env.Program(['terrain_quad.cpp', 'camera.cpp', 'free_camera.cpp'])
	env.Program(['satellite_map.cpp', 'camera.cpp', 'free_camera.cpp', 'texture.cpp', 'shader.cpp', 'tiff.cpp'])

	# height_scale	sample
	imgui = env.StaticLibrary([
		Glob('imgui/*.cpp'),
		'imgui/examples/imgui_impl_sdl.cpp',  # TODO: maybe we do not need this backend for SDL
		'imgui/examples/imgui_impl_opengl3.cpp',  # backend for opengl es3
	])

	height_scale_common = ['camera.cpp', 'free_camera.cpp', 'texture.cpp', 'shader.cpp',
		'tiff.cpp', 'io.cpp']

	env.Program(['height_scale.cpp', height_scale_common, imgui])

	# four terrain sample
	env.Program(['four_terrain.cpp', height_scale_common, 'flat_shader.cpp', 'quad.cpp',
		'axes_model.cpp', 'four_terrain_ui.cpp', 'four_terrain_shader_program.cpp',
		'set_uniform.cpp', imgui])

	# height overlap sample
	env.Program(['height_overlap.cpp', height_scale_common, 'flat_shader.cpp', 'quad.cpp',
		'axes_model.cpp', 'four_terrain_ui.cpp', 'height_overlap_shader_program.cpp',
		'set_uniform.cpp', imgui])

	# terrain mesh sample
	env.Program(['terrain_mesh.cpp', height_scale_common, 'flat_shader.cpp', 'quad.cpp',
		'axes_model.cpp', 'four_terrain_ui.cpp', 'height_overlap_shader_program.cpp',
		'set_uniform.cpp', imgui])

	# tile_grid
	env.Program(['tile_grid.cpp'])

	# terrain mesh sample
	env.Program(['terrain_scale.cpp', height_scale_common, 'flat_shader.cpp', 'quad.cpp',
		'axes_model.cpp', 'terrain_scale_ui.cpp', 'height_overlap_shader_program.cpp',
		'set_uniform.cpp', imgui])

	# above terrain
	above_terrain_common = ['free_camera.cpp', 'texture.cpp', 'shader.cpp',
		'tiff.cpp', 'io.cpp']

	env.Program(['above_terrain.cpp', above_terrain_common, 'flat_shader.cpp', 'quad.cpp',
		'axes_model.cpp', 'terrain_scale_ui.cpp', 'height_overlap_shader_program.cpp',
		'above_terrain_outline_shader_program.cpp', 'set_uniform.cpp', imgui])

	# generate_dump sample
	env.Program(['generate_dump.cpp'])

	# grid of terrains
	grid_of_terrains_common = [above_terrain_common, 'axes_model.cpp', 'flat_shader.cpp', 'terrain_scale_ui.cpp',
		'height_overlap_shader_program.cpp', 'above_terrain_outline_shader_program.cpp', 'set_uniform.cpp']

	env.Program(['grid_of_terrains.cpp', grid_of_terrains_common, 'quad.cpp',
		'grid_of_terrains_lightdir_shader_program.cpp', 'terrain_grid.cpp', 'terrain_camera.cpp', imgui])

	# more details
	more_details_common = [grid_of_terrains_common, 'quad.cpp',
		'grid_of_terrains_lightdir_shader_program.cpp', 'terrain_camera.cpp']

	env.Program(['more_details.cpp', 'more_details_terrain_grid.cpp', more_details_common, imgui])

	# lod tiles
	env.Program(['lod_tiles.cpp', 'more_details_terrain_grid.cpp',
		'lod_tiles_user_input.cpp', 'lod_tiles_draw_terrain.cpp', more_details_common, imgui])

	# other samples ...

def configure(env, dependency_list):
	conf = env.Configure(
		custom_tests={'CheckPkgVersion': check_pkg_version})

	for dep in dependency_list:
		pkg = ("%s %s" % dep) if type(dep) == tuple else dep
		if not conf.CheckPkgVersion(pkg):
			print("'%s' library not found" % pkg)
			Exit(1)

	conf_env = conf.Finish()

	pkg_conf = 'pkg-config --cflags --libs ' + ' '.join(
		map(lambda dep: dep[0] if type(dep) == tuple else dep, dependency_list))

	conf_env.ParseConfig(pkg_conf)

	return conf_env

def check_pkg_version(context, pkg):
	"""custom pkg-config based package version check"""

	context.Message("Checking for '%s' library ... " % pkg)
	res = context.TryAction("pkg-config --exists '%s'" % pkg)[0]
	context.Result(res)
	return res

build()
