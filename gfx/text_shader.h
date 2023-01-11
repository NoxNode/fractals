#pragma once
#include "shader.h"
#include "vertex.h"

struct TextShader {
    GLuint program;
    GLuint u_mvp;
    GLuint u_texture;
    GLuint u_color;
};

void InitTextShader(TextShader& shader) {
    InitShader(shader.program, "shaders/textVShader.glsl", "shaders/textFShader.glsl");
    
    shader.u_mvp = glGetUniformLocation(shader.program, "u_mvp");
    
    shader.u_texture = glGetUniformLocation(shader.program, "u_texture");

    shader.u_color = glGetUniformLocation(shader.program, "u_color");

}
