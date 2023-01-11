#pragma once

#ifdef _WIN32
	#include <Windows.h>
	void* LoadDLL(const char* path) {
		return LoadLibrary(path);
	}

	void* GetFuncFromDLL(void* dll_handle, const char* func_name) {
		return (void*)GetProcAddress((HMODULE) dll_handle, func_name);
	}

	void UnloadDLL(void* dll_handle) {
		FreeLibrary((HMODULE) dll_handle);
	}
#else
	#include <dlfcn.h>
	void* LoadDLL(const char* path) {
		return dlopen(path, RTLD_LAZY);
	}

	void* GetFuncFromDLL(void* dll_handle, const char* func_name) {
		return dlsym(dll_handle, func_name);
	}

	void UnloadDLL(void* dll_handle) {
		dlclose(dll_handle);
	}
#endif
