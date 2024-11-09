#include "imgui/imgui.h"
#include "imgui/examples/imgui_impl_sdl.h"
#include "imgui/examples/imgui_impl_opengl3.h"
#include "terrain_scale_ui.hpp"

using std::filesystem::path;

terrain_scale_ui::terrain_scale_ui()
	: _window{nullptr}
{}

void terrain_scale_ui::init(path const & config_filename) {
	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	[[maybe_unused]] ImGuiIO & io = ImGui::GetIO();
	io.IniFilename = config_filename.c_str();
	// we can configure ImGuiIO via io variable ...

	ImGui::StyleColorsDark();
}

void terrain_scale_ui::setup(SDL_Window * window, SDL_GLContext & context) {
	// Setup Platform/Renderer bindings
	assert(window);
	_window = window;
	ImGui_ImplSDL2_InitForOpenGL(_window, &context);
	ImGui_ImplOpenGL3_Init();
}

void terrain_scale_ui::create() {
	// create gui
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplSDL2_NewFrame(_window);
	ImGui::NewFrame();

	ImGui::Begin("Options");  // begin window

	ImGui::DragFloat("Height Scale", &height_scale, 0.1f, 1.0f, 20.0f);
	ImGui::DragFloat("Quad Scale", &quad_scale, 1.0f, 2.0f, 100.0f);

	if (ImGui::InputInt("Quad (NxN) resolution", &quad_resolution, 1, 5, ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CharsDecimal)) {
		quad_resolution = std::max(min_quad_resolution, quad_resolution);
	}

	ImGui::End();  // end window

	ImGui::SetWindowFocus(nullptr);
}

void terrain_scale_ui::render() {
	ImGui::Render();  // render ImGui
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void terrain_scale_ui::shutdown() {
	// TODO: can we do this in a constructor
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplSDL2_Shutdown();
	ImGui::DestroyContext();
	_window = nullptr;
}
