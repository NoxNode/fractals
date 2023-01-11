#pragma once
#include <stdio.h>
#include <stdlib.h>
#include "types.h"

typedef struct {
	u8* start;
	u8* curLocation;
	u32 maxSize;
} Memory;

// maxSize is in bytes of how much memory to allocate
void InitMemory(Memory& mem, u32 maxSize) {
	mem.maxSize = maxSize;
	mem.start = (u8*) calloc(maxSize, sizeof(u8));
	if(mem.start == NULL) {
		printf("Failed to allocate memory for program\n");
		exit(1);
	}
	mem.curLocation = mem.start;
}

void DeinitMemory(Memory& mem) {
	free(mem.start);
}

// size is in bytes of how much memory to allocate
void* Alloc(Memory& mem, u32 size) {
	if(mem.curLocation + size > mem.start + mem.maxSize) {
		printf("Ran out of memory");
		DeinitMemory(mem);
		exit(1);
	}

	u8* lastLocation = mem.curLocation;
	mem.curLocation += size;

	return (void*) lastLocation;
}
