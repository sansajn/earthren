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
		])

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
		('spdlog', '>= 1.9.2')  # libspdlog-dev
		# Boost.GIL
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
