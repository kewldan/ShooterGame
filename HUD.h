#ifndef SHOOTERGAME_HUD_H
#define SHOOTERGAME_HUD_H

#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
#include "Window.h"

class HUD {
public:
	HUD(Window* window);
	~HUD();

	void begin();

	void end();
};


#endif //SHOOTERGAME_HUD_H
