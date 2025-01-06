/*! \file
Sample user input handling functions (for keyboard, mouse). */
#pragma once
#include <SDL.h>
#include "free_camera.hpp"
#include "terrain_camera.hpp"

struct input_mode {  // list of active input contol modes
	bool pan = false,
		rotate = false;
	bool detail_camera = false;

	// in case of detail camera used (free_camera)
	bool move_forward = false,  // w
		move_backkward = false,  // s
		move_left = false,  // a
		move_right = false;  // d
};

struct input_events {
	bool zoom_in;
	bool camera_switch;
	bool info_request;

	input_events() : zoom_in{false}, camera_switch{false}, info_request{false} {}

	void reset() {
		zoom_in = camera_switch = info_request = false;
	}
};

struct render_features {  // list of selected rendering features
	bool show_terrain,
		show_lightdir,
		show_outline,
		show_satellite,
		calculate_shades;
};


void input_render_features(SDL_Event const & event, render_features & features);  //!< Handling render features.
void input_control_mode(SDL_Event const & event, input_mode & mode, input_events & events);  //!< Handle pan/rotate modes.
void input_camera(SDL_Event const & event, terrain_camera & cam, input_mode const & mode,
	input_events & events);  //!< handle camera movement, rotations
void input_camera(SDL_Event const & event, free_camera & cam, input_mode const & mode,
	input_events & events);  //!< handle camera movement, rotations
