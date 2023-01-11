#include "core/memory.h"
#include "core/rdll.h"

#ifndef DLL_FILE
	#define DLL_FILE "game.dll"
#endif

#ifndef MEM_SIZE
	#define MEM_SIZE 200 MB
#endif

#ifndef MAX_DLLS
	#define MAX_DLLS 20
#endif

#ifndef WINDOW_WIDTH
	// #define WINDOW_WIDTH 1920
	#define WINDOW_WIDTH 1200
#endif

#ifndef WINDOW_HEIGHT
	// #define WINDOW_HEIGHT 1080
	#define WINDOW_HEIGHT 800
#endif

typedef void (*DLLFunc)(Memory&);

// #define DONT_OPEN_WINDOW
#ifndef DONT_OPEN_WINDOW
	#include <GL/glew.h>
	#include "core/window.h"
#endif

#define MAX_SOUND_BUFFERS 20
#include <SFML/Audio.hpp>

/*
config file to replace all the #defines
*/

int main() {
	Memory mem;
	InitMemory(mem, MEM_SIZE);

#ifndef DONT_OPEN_WINDOW
	void* win = Alloc(mem, sizeof(Window)); // alloc this separately to make it the first thing allocated
	Window* window = InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, win, Alloc(mem, sizeof(sf::RenderWindow)));
	glewExperimental = GL_TRUE;
	if(glewInit() != GLEW_OK) {
		printf("Failed to initialize GLEW\n");
		return 1;
	}
#else
	bool& running = *((bool*)Alloc(mem, sizeof(bool)));
	running = true;
#endif


	// load and init dll
	string dll_file_name = string(DLL_FILE);
	ReloadableDLL& rdll = *((ReloadableDLL*)Alloc(mem, sizeof(ReloadableDLL)));
	DLLFunc updateFunc;
	if(!InitReloadableDLL(rdll, dll_file_name, "open_" + dll_file_name)) {
		DeinitMemory(mem);
		return 1;
	}

	sf::SoundBuffer** sound_buffers = ((sf::SoundBuffer**) Alloc(mem, MAX_SOUND_BUFFERS * sizeof(sf::SoundBuffer*)));
	for(int i = 0; i < MAX_SOUND_BUFFERS; i++) {
		sound_buffers[i] = new
			(Alloc(mem, sizeof(sf::SoundBuffer)))
			(sf::SoundBuffer)();
	}

	// run the Init func of each newly loaded dll
	((DLLFunc) GetFuncFromDLL(rdll.dll_handle, "Init"))(mem);
	// get pointer to update func
	updateFunc = (DLLFunc) GetFuncFromDLL(rdll.dll_handle, "Update");

	// do game loop
#ifndef DONT_OPEN_WINDOW
	while(window->sfml_window->isOpen()) {
#else
	while(running) {
#endif
		int status = ReloadDLLIfUpdated(rdll);
		if(status == 0) {
			DeinitMemory(mem);
			return 1;
		}
		else if(status == 1) {
			printf("reloading dll\n");
			// call reinit
			((DLLFunc) GetFuncFromDLL(rdll.dll_handle, "Reinit"))(mem);
			// get pointer to update func
			updateFunc = (DLLFunc) GetFuncFromDLL(rdll.dll_handle, "Update");
		}
		if(rdll.dll_handle == NULL) continue;
		// call update
		updateFunc(mem);
	}
	
	// deinit dll
	((DLLFunc) GetFuncFromDLL(rdll.dll_handle, "Deinit"))(mem);

#ifndef DONT_OPEN_WINDOW
	DeinitWindow(window);
#endif
	DeinitMemory(mem);

	return 0;
}
