#pragma once
#include "types.h"
#include <stdio.h>
#include <fstream>
#include <string>
using namespace std;

// out_contents is the string to read the contents of the file into
bool ReadFile(string& out_contents, const char* path) {
	std::ifstream fileStream(path, std::ios::in);
	if(!fileStream.is_open()) {
		printf("Failed to open %s.\n", path);
		return false;
	}
	std::string line = "";
	while(getline(fileStream, line))
		out_contents += line + "\n";
	fileStream.close();
	return true;
}

// in_contents is the string to output to the file
bool WriteFile(string& in_contents, const char* path) {
	ofstream myfile;
	myfile.open (path);
	if(!myfile.is_open()) {
		printf("Failed to open %s.\n", path);
		return false;
	}
	myfile << in_contents;
	myfile.close();
	return true;
}

bool MakeCopyOfFile(const char* fromPath, const char* toPath) {
    std::ifstream  src(fromPath, std::ios::binary);
	if(!src.is_open()) return false;
    std::ofstream  dst(toPath,   std::ios::binary);
	if(!dst.is_open()) return false;
	size_t start = dst.tellp();
    dst << src.rdbuf();
	if(dst.tellp() == start) return false;
	return true;

	// std::ifstream  src(fromPath, std::ios::binary);
	// std::ofstream  dst(toPath,   std::ios::binary);
	// dst << src.rdbuf();
	// return true;
}

const char* getFileExtension(const char* filePath) {
	int lastPeriodIndex = 0;
	int curChar = 0;

	while(filePath[curChar] != '\0') {
		if(filePath[curChar] == '.')
			lastPeriodIndex = curChar;
		curChar++;
	}

	return filePath + lastPeriodIndex;
}

#include <sys/types.h>
#include <sys/stat.h>
#ifdef _WIN32
	#define stat _stat
#else
	#include <unistd.h>
#endif

#define FILETIME time_t
time_t LastModifiedOfFile(const char* filename) {
	struct stat result;
	stat(filename, &result);
	return result.st_mtime;
}

u32 CompareFileTime(FILETIME* a, FILETIME* b) {
	if(*a == *b)
		return 0;
	else
		return 1;
}