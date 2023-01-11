#pragma once
#include "shader.h"
#include "texture.h"
#include "vertex.h"
#include "gl_buffers.h"
#include "camera.h"

struct RaymarchRenderer {
	// GLuint vao;

    GLuint shader;
    FBO* renderMap;
	// Camera orthoCamera;
    // TODO: can have an option to draw this to an FBO and pass it to the default renderer
};

void InitRaymarchRenderer(RaymarchRenderer& renderer, VBO& vbo, IBO& ibo, FBO* renderMap) {
	InitShader(renderer.shader, "shaders/simpleVS.glsl", "shaders/fractalFS3.glsl");
	renderer.renderMap = renderMap;

	// glGenVertexArrays(1, &renderer.vao);
	// glBindVertexArray(renderer.vao);
	// glBindBuffer(GL_ARRAY_BUFFER, vbo.id);

	// GLint posLoc = glGetAttribLocation(renderer.shader, "a_position");
	// glVertexAttribPointer(posLoc, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*) offsetof(Vertex, pos));
	// glEnableVertexAttribArray(posLoc);
	
	// glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo.id);

	// InitOrthoCamera(renderer.orthoCamera, v3(0,0,1), vec3(0,0,-1), v3(0,1,0), v3(1,0,0), -0.5f, 0.5f, -0.5f, 0.5f, 0.01f, 10.0f);
}

void DeinitRaymarchRenderer(RaymarchRenderer& renderer) {
    DeinitShader(renderer.shader);
	// glDeleteVertexArrays(1, &renderer.vao);
}

void RaymarchRender(RaymarchRenderer& renderer, Camera& camera) {
	glUseProgram(renderer.shader);
	// glBindVertexArray(renderer.vao);
	
	glDisable(GL_DEPTH_TEST);
	
	glClear(GL_COLOR_BUFFER_BIT);

	// u_cameraPos
	GLuint u_cameraPos = glGetUniformLocation(renderer.shader, "u_cameraPos");
	glUniform3fv(u_cameraPos, 1, &camera.pos[0]);
	// u_cameraDir
	GLuint u_cameraDir = glGetUniformLocation(renderer.shader, "u_cameraDir");
	glUniform3fv(u_cameraDir, 1, &camera.dir[0]);

	GLuint u_renderMap = glGetUniformLocation(renderer.shader, "u_renderMap");
	BindTexture(u_renderMap, renderer.renderMap->texture, 0);

	// GLuint u_mvpMatrix = glGetUniformLocation(renderer.shader, "u_mvpMatrix");
	// glUniformMatrix4fv(u_mvpMatrix, 1, GL_FALSE, &renderer.orthoCamera.vpMatrix[0][0]);

	// GLuint iResolution = glGetUniformLocation(renderer.shader, "iResolution");
	// vec2 resolution = vec2(game->window->sfml_window->getSize().x, game->window->sfml_window->getSize().y);
	// glUniform3fv(iResolution, 1, &resolution[0]);
	
    // hard coded 6 as the num indices for a square and 0 for the offset (always have square as the first model in the vbo)
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (void*)(0 * sizeof(IndexType)));

	glEnable(GL_DEPTH_TEST);
}
