#pragma once
#include "input.h"
#include <SFML/Graphics.hpp>
#include <SFML/OpenGL.hpp>

typedef struct {
	sf::RenderWindow* sfml_window;
	Input input;
} Window;

Window* InitWindow(u32 width, u32 height, void* window_mem, void* sfml_window_mem) {
	Window* window = (Window*) window_mem;

	// placement new the sfml window in
	// need to do this because the constructor needs to be called
	sf::ContextSettings settings;
	settings.depthBits = 24;
	settings.majorVersion = 4;
	settings.minorVersion = 1;
	settings.attributeFlags = sf::ContextSettings::Core;
	window->sfml_window = new
		(sfml_window_mem)
		(sf::RenderWindow) (sf::VideoMode(width, height), "OpenGL", sf::Style::Default, settings);

	settings = window->sfml_window->getSettings();
	printf("depth bits: %d\n", settings.depthBits);
	printf("major version: %d, minor version: %d\n", settings.majorVersion, settings.minorVersion);
	if(settings.majorVersion < 4 && settings.minorVersion < 1) {
		printf("This game requires OpenGL version 4.1 or higher\n");
		exit(1);
	}

	// put more options here as needed
	window->sfml_window->setVerticalSyncEnabled(true);
	window->sfml_window->setActive(true);

	return window;
}

void DeinitWindow(Window* window) {
	window->sfml_window->close();
	window->sfml_window->~RenderWindow();
}

void UpdateWindowInput(Window* window) {
	if(!window->sfml_window->isOpen()) return;
	ResetInput(window->input);
	sf::Event event;
	while (window->sfml_window->pollEvent(event)) {
		if (event.type == sf::Event::Resized) {
			glViewport(0, 0, event.size.width, event.size.height);
		}
		HandleSFMLInput(window->input, event);
		if(window->input.windowClosed) {
			window->sfml_window->close();
			break;
		}
	}
}

void UpdateWindowDisplay(Window* window) {
	if(!window->sfml_window->isOpen()) return;
	// end the current frame (internally swaps the front and back buffers)
	window->sfml_window->display();
}
