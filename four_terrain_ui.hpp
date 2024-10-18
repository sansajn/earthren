#pragma once
#include <filesystem>
#include <SDL.h>

/*! \note Input handling is not part of the integration and we do not want it there. */
class four_terrain_ui {
public:
	// opitons
	float height_scale = 0.0f;

	four_terrain_ui();

	// TODO: non intuitive API, find something bether
	void init(std::filesystem::path const & config_filename);
	void setup(SDL_Window * window, SDL_GLContext & context);  // TODO: do we realy need init and setup?

	void create();  //!< called for every frame TODO: think for a better name setup_ui()?
	void render();

	void shutdown();

private:
	SDL_Window * _window;
};
