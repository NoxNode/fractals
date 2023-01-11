#pragma once
#include "dll.h"
#include "fileio.h"

#define MAX_DLL_SIZE 1 MB

typedef struct {
	FILETIME lastModifyTime;
	void* dll_handle;
	string dll_filename;
	string open_dll_filename;
	void* mem;
} ReloadableDLL;

// returns false on error
bool InitReloadableDLL(ReloadableDLL& rdll, string dll_filename, string open_dll_filename) {
#ifndef DISABLE_LIVE_UPDATING
	if(!MakeCopyOfFile(dll_filename.c_str(), open_dll_filename.c_str())) {
		printf("InitReloadableDLL(): failed to copy dll\n");
		return false;
	}
	rdll.lastModifyTime = LastModifiedOfFile(dll_filename.c_str());
	rdll.dll_handle = LoadDLL(open_dll_filename.c_str());
#else
	rdll.dll_handle = LoadDLL(dll_filename.c_str());
#endif
	if(rdll.dll_handle == NULL) {
		printf("InitReloadableDLL(): failed to load dll\n");
		return false;
	}
	rdll.dll_filename = dll_filename;
	rdll.open_dll_filename = open_dll_filename;
	return true;
}

void DeinitReloadableDLL(ReloadableDLL& rdll) {
	if(rdll.dll_handle == NULL) return;
	UnloadDLL(rdll.dll_handle);
}

// returns 0 on error, 1 if reloaded, 2 if not reloaded
int ReloadDLLIfUpdated(ReloadableDLL& rdll) {
#ifndef DISABLE_LIVE_UPDATING
	FILETIME newModifyTime = LastModifiedOfFile(rdll.dll_filename.c_str());
	if(CompareFileTime(&rdll.lastModifyTime, &newModifyTime) == 0)
		return 2; // no error, just didn't need to reload
	UnloadDLL(rdll.dll_handle);
	rdll.dll_handle = NULL;
	if(!MakeCopyOfFile(rdll.dll_filename.c_str(), rdll.open_dll_filename.c_str())) {
		printf("ReloadDLLIfUpdated(): failed to copy dll\n");
		return 2;
	}
	rdll.dll_handle = LoadDLL(rdll.open_dll_filename.c_str());
	if(rdll.dll_handle == NULL) {
		printf("ReloadDLLIfUpdated(): failed to load dll\n");
		return 2;
	}
	rdll.lastModifyTime = newModifyTime;
	return 1;
#endif
	return 2;
}
