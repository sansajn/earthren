#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <spdlog/spdlog.h>
#include "lod_tiles_user_input.hpp"

using glm::vec2, glm::vec3;

void input_render_features(SDL_Event const & event, render_features & features) {
	if (event.type == SDL_KEYDOWN) {
		switch (event.key.keysym.sym) {
		case SDLK_l:
			features.show_lightdir = !features.show_lightdir;
			spdlog::info("show_lightdir={}", features.show_lightdir);
			break;
		case SDLK_o:
			features.show_outline = !features.show_outline;
			spdlog::info("show_outline={}", features.show_outline);
			break;
		case SDLK_t:
			features.show_terrain = !features.show_terrain;
			spdlog::info("show_terrain={}", features.show_terrain);
			break;
		case SDLK_s:
			features.show_satellite = !features.show_satellite;
			spdlog::info("show_satellite={}", features.show_satellite);
			break;
		case SDLK_a:
			features.calculate_shades = !features.calculate_shades;
			spdlog::info("calculate_shadess={}", features.calculate_shades);
			break;
		}
	}
}

void input_control_mode(SDL_Event const & event, input_mode & mode, input_events & events) {
	//  handle pan mode
	if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) {
		mode.pan = true;
		spdlog::info("left mouse pressed");
	}

	if (event.type == SDL_MOUSEBUTTONUP && event.button.button == SDL_BUTTON_LEFT) {
		mode.pan = false;
		spdlog::info("left mouse released");
	}

	// rotation mode or detail camera
	if (event.type == SDL_KEYDOWN) {
		switch (event.key.keysym.sym) {
		case SDLK_LCTRL:  // enable camera rotation state
			mode.rotate = true;
			break;

		case SDLK_f:  // switch detail camera mode on/off
			mode.detail_camera = !mode.detail_camera;
			events.camera_switch = true;
			spdlog::info("detail-camera={}", mode.detail_camera);
			break;

		// detail camera input modes
		case SDLK_w:
			mode.move_forward = true;
			break;

		case SDLK_s:
			mode.move_backkward = true;
			break;

		case SDLK_a:
			mode.move_left = true;
			break;

		case SDLK_d:
			mode.move_right = true;
			break;


		// info request event
		case SDLK_i:
			events.info_request = true;
			break;
		}
	}
	else if (event.type == SDL_KEYUP) {
		switch (event.key.keysym.sym) {
		case SDLK_LCTRL:  // disable camera rotation state
			mode.rotate = false;
			break;

		// detail camera input modes
		case SDLK_w:
			mode.move_forward = false;
			break;

		case SDLK_s:
			mode.move_backkward = false;
			break;

		case SDLK_a:
			mode.move_left = false;
			break;

		case SDLK_d:
			mode.move_right = false;
			break;
		}
	}
}

void input_camera(SDL_Event const & event, terrain_camera & cam, input_mode const & mode,
	input_events & events) {
	// distance
	if (event.type == SDL_MOUSEWHEEL) {
		assert(event.wheel.direction == SDL_MOUSEWHEEL_NORMAL);

		if (event.wheel.y > 0) {  // scroll up
			cam.distance /= 1.1;
			events.zoom_in = true;
		}
		else if (event.wheel.y < 0)  // scroll down
			cam.distance *= 1.1;

		spdlog::info("distance={}", cam.distance);
	}

	// reset
	if (event.type == SDL_KEYDOWN) {
		switch (event.key.keysym.sym) {
		case SDLK_c:  // reset camera
			cam = terrain_camera{20.0f};
			cam.look_at = {0, 0};
			break;
		case SDLK_p:  // set camera to predefined position
			cam = terrain_camera{2.97287f};
			cam.look_at = {0, 0};
			cam.theta = 0.599999f;
			cam.phi = 0.62;
			break;
		}
	}

	// pan and rotation
	if (mode.pan && event.type == SDL_MOUSEMOTION) {
		vec2 ds = vec2{event.motion.xrel, -event.motion.yrel};
		if (mode.rotate) {
			cam.phi -= ds.x / 500.0f;
			cam.theta += ds.y / 500.0f;
		}
		else  // pan
			cam.look_at -= ds * 0.01f;
	}
}

void input_camera(SDL_Event const & event, free_camera & cam, input_mode const & mode,
	[[maybe_unused]] input_events & events) {

	constexpr float dt = 0.1f;
	constexpr float angular_speed = 1.0f/100.0f;  // rad/s

	// handle mouse movement when ctrl is pressed to rotate camera
	if (mode.pan && event.type == SDL_MOUSEMOTION) {
		vec2 ds = vec2{event.motion.xrel, -event.motion.yrel};
		if (mode.rotate) {
			vec3 const up = vec3{0,0,1};

			if (ds.x != 0.0f) {
				float const angle = ds.x * angular_speed * dt;
				cam.rotation = normalize(angleAxis(-angle, up) * cam.rotation);
			}
			if (ds.y != 0.0f) {
				float angle = ds.y * angular_speed * dt;
				cam.rotation = normalize(angleAxis(-angle, cam.right()) * cam.rotation);
			}
		}

		// note: pan is not supported for free_camera
	}

	// handle reset
	// note: we do not want to handle mouse wheel
}
