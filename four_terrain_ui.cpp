#include "imgui/imgui.h"
#include "imgui/examples/imgui_impl_sdl.h"
#include "imgui/examples/imgui_impl_opengl3.h"
#include "four_terrain_ui.hpp"

using std::filesystem::path;

four_terrain_ui::four_terrain_ui()
	: _window{nullptr}
{}

void four_terrain_ui::init(path const & config_filename) {
	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	[[maybe_unused]] ImGuiIO & io = ImGui::GetIO();
	io.IniFilename = config_filename.c_str();
	// we can configure ImGuiIO via io variable ...

	ImGui::StyleColorsDark();
}

void four_terrain_ui::setup(SDL_Window * window, SDL_GLContext & context) {
	// Setup Platform/Renderer bindings
	assert(window);
	_window = window;
	ImGui_ImplSDL2_InitForOpenGL(_window, &context);
	ImGui_ImplOpenGL3_Init();
}

void four_terrain_ui::create() {
	// create gui
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplSDL2_NewFrame(_window);
	ImGui::NewFrame();

	ImGui::Begin("Options");  // begin window

	ImGui::DragFloat("Height Scale", &height_scale, 0.1f, 1.0f, 20.0f);

	ImGui::End();  // end window

	ImGui::SetWindowFocus(nullptr);
}

void four_terrain_ui::render() {
	ImGui::Render();  // render ImGui
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void four_terrain_ui::shutdown() {
	// TODO: can we do this in a constructor
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplSDL2_Shutdown();
	ImGui::DestroyContext();
	_window = nullptr;
}
