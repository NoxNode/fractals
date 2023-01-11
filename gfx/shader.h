#pragma once
#include <GL/glew.h>
#include <vector>
#include "../core/fileio.h"
using namespace std;

bool CheckOpenGLError(GLuint id, bool shader_program_info) {
	GLint Result = GL_FALSE;
	int InfoLogLength;
	if(shader_program_info) {
		glGetShaderiv(id, GL_COMPILE_STATUS, &Result);
		glGetShaderiv(id, GL_INFO_LOG_LENGTH, &InfoLogLength);
	}
	else {
		glGetProgramiv(id, GL_LINK_STATUS, &Result);
		glGetProgramiv(id, GL_INFO_LOG_LENGTH, &InfoLogLength);
	}
	if(InfoLogLength > 0) {
		std::vector<char> errorMessage(InfoLogLength + 1);
		if(shader_program_info)
			glGetShaderInfoLog(id, InfoLogLength, NULL, &errorMessage[0]);
		else
			glGetProgramInfoLog(id, InfoLogLength, NULL, &errorMessage[0]);
		printf("%s\n", &errorMessage[0]);
		return false;
	}
	return true;
}

GLuint CompileShaderProgram(const char* vertexShaderPath, const char* fragmentShaderPath) {
	// create shaders
	GLuint vertexShaderID = glCreateShader(GL_VERTEX_SHADER);
	GLuint fragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);

	// read in shaders
	string vertexShaderCode;
	if(!ReadFile(vertexShaderCode, vertexShaderPath)) {
		return 0;
	}
	char const* vertexSourcePointer = vertexShaderCode.c_str();
	std::string fragmentShaderCode;
	if(!ReadFile(fragmentShaderCode, fragmentShaderPath)) {
		return 0;
	}
	char const* fragmentSourcePointer = fragmentShaderCode.c_str();

	// compile shaders
	glShaderSource(vertexShaderID, 1, &vertexSourcePointer , NULL);
	glCompileShader(vertexShaderID);
	glShaderSource(fragmentShaderID, 1, &fragmentSourcePointer , NULL);
	glCompileShader(fragmentShaderID);

	// check if error compiling
	if(!CheckOpenGLError(vertexShaderID, true)) {
		return 0;
	}
	// check if error compiling
	if(!CheckOpenGLError(fragmentShaderID, true)) {
		return 0;
	}

	// Link the program
	GLuint program = glCreateProgram();
	glAttachShader(program, vertexShaderID);
	glAttachShader(program, fragmentShaderID);
	glLinkProgram(program);
	// check if error linking
	if(!CheckOpenGLError(program, false)) {
		return 0;
	}

	// cleanup
	glDetachShader(program, vertexShaderID);
	glDetachShader(program, fragmentShaderID);
	glDeleteShader(vertexShaderID);
	glDeleteShader(fragmentShaderID);

	return program;
}

void InitShader(GLuint& shaderProgram, const char* vertexShaderPath, const char* fragmentShaderPath) {
	shaderProgram = CompileShaderProgram(vertexShaderPath, fragmentShaderPath);
	if(!shaderProgram) {
		printf("error with CompileShaderProgram\n");
	}
	glUseProgram(shaderProgram);
}

void DeinitShader(GLuint shaderProgram) {
	glDeleteProgram(shaderProgram);
}
