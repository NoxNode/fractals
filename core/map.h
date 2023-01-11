#pragma once

#include "string.h"

#define DEFAULT_MAP_SIZE 256

class Map {
public:
	String* keys;
	void** values;
	u32 curSize;
	u32 maxSize;

	// TODO: make this more efficient
	void*& operator[](const char* key) {
		for(u32 i = 0; i < curSize; i++) {
			if(keys[i] == key) return values[i];
		}
		// TODO: handle this error better
		if(curSize == maxSize) {
			printf("map is full\n");
			return values[curSize - 1];
		}
		curSize++;
		keys[curSize - 1] = key;
		return values[curSize - 1];
	}
};

Map* InitMap(Memory& mem, u32 map_size = DEFAULT_MAP_SIZE, u32 string_size = DEFAULT_STRING_SIZE) {
	Map* map = (Map*) Alloc(mem, sizeof(Map));

	map->curSize = 0;
	map->maxSize = map_size;
	map->keys = (String*) Alloc(mem, map->maxSize * sizeof(String));
	for(u32 i = 0; i < map->maxSize; i++) {
		InitString(mem, &map->keys[i], string_size);
	}
	map->values = (void**) Alloc(mem, map->maxSize * sizeof(void*));

	return map;
}

// simple unit test

// int main() {
// 	Memory mem;
// 	InitMemory(mem, 100 KB);

// 	Map& map = *(InitMap(mem));
// 	map["test"] = (void*)"test123";
// 	int i = 10;
// 	map["testI"] = (void*) &i;
// 	float r = 10.2f;
// 	map["testF"] = (void*) &r;

// 	printf("%s\n", (char*) map["test"]);
// 	printf("%d\n", *((int*) map["testI"]));
// 	printf("%f\n", *((float*) map["testF"]));

// 	DeinitMemory(mem);
// 	return 0;
// }
