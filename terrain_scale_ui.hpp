#pragma once
#include <filesystem>
#include <SDL.h>

/*! \note Input handling is not part of the integration and we do not want it there. */
class terrain_scale_ui {
public:
	// opitons
	float height_scale = 0.0f,
		quad_scale = 2.0f;

	int quad_resolution = 100;

	// constrains
	constexpr static int min_quad_resolution = 10;

	terrain_scale_ui();

	// TODO: non intuitive API, find something bether
	void init(std::filesystem::path const & config_filename);
	void setup(SDL_Window * window, SDL_GLContext & context);  // TODO: do we realy need init and setup?

	void create();  //!< \note needs to be called for every frame TODO: think for a better name setup_ui()?
	void render();

	void shutdown();

private:
	SDL_Window * _window;
};
