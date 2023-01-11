#pragma once

#include <freetype2/ft2build.h>
#include FT_FREETYPE_H

#include <iostream>
#include <exception>
#include "character.hpp"
#include "../core/map.h"
#include "../core/string.h"
#include <SFML/OpenGL.hpp>
#include "text_shader.h"
#include "render_obj.h"
#include "material.h"
#include "light.h"
#include "../core/string.h"
#include "../core/map.h"
#include "../gfx/character.hpp"
#include "gl_buffers.h"

void loadCharacters(Map* characters, Memory& mem)
{
    FT_Library ft;
    if (FT_Init_FreeType(&ft))
    {
        std::cout << "ERROR::FREETYPE: Could not init FreeType Library" << std::endl;
        throw std::runtime_error("Could not load FreeType");
    }
    std::cout << "FREETYPE INITIALIZED" << std::endl;

    FT_Face face;
    if (FT_New_Face(ft, "fonts/Arial.ttf", 0, &face))
    {
        std::cout << "ERROR::FREETYPE: Failed to load font" << std::endl;
        throw std::runtime_error("Could not load font");
    }
    std::cout << "FACE INITIALIZED" << std::endl;

    FT_Set_Pixel_Sizes(face, 0, 48); 

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    for (unsigned char c = 0; c < 128; c++)
    {
        // load character glyph 
        if (FT_Load_Char(face, c, FT_LOAD_RENDER))
        {
            std::cout << "ERROR::FREETYTPE: Failed to load Glyph" << std::endl;
            continue;
        }
        // generate texture
        unsigned int texture;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(
            GL_TEXTURE_2D,
            0,
            GL_RED,
            face->glyph->bitmap.width,
            face->glyph->bitmap.rows,
            0,
            GL_RED,
            GL_UNSIGNED_BYTE,
            face->glyph->bitmap.buffer
        );
        // set texture options
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        // now store character for later use
        Character* character = (Character*) Alloc(mem, sizeof(Character));
        character->TextureID = texture;
        character->Bearing = glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top);
        character->Size = glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows);
        character->Advance = (uint32_t)face->glyph->advance.x;

        char temp_str[2];
        temp_str[0] = c;
        temp_str[1] = '\0';
        (*characters)[temp_str] = character;
    }
    FT_Done_Face(face);
    FT_Done_FreeType(ft);
}

struct TextRenderer {
	GLuint vao;
    TextShader textShader;
};


void InitTextRenderer(TextRenderer& renderer) {
	InitTextShader(renderer.textShader);
}

void DeinitTextRenderer(TextRenderer& renderer)
{
	DeinitShader(renderer.textShader.program);
}

void TextRender(TextRenderer& renderer,
        const std::string& text,
        Camera& camera,
        const v3& color,
        v3 position,
        const float scale,
        u32 nRenderObjs,
        u32 windowWidth,
        u32 windowHeight,
        Map* characters)
{

    glm::mat4 model_matrix = glm::mat4(1);

	// init for default render
	glUseProgram(renderer.textShader.program);
	glUniform3fv(renderer.textShader.u_color, 1, &color[0]);

	// glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    // glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // iterate through all characters
    for (int i = 0; i < text.length(); i++)
    {
        char key = text[i];

        char temp_str[2];
        temp_str[0] = key;
        temp_str[1] = '\0';
        Character& ch = *((Character*)((*characters)[temp_str]));

        glActiveTexture(GL_TEXTURE4); // TODO MANAGE THIS
        glBindTexture(GL_TEXTURE_2D, ch.TextureID);
        glUniform1i(glGetUniformLocation(renderer.textShader.program, "text"), 4);

        // printf("CHARACTER: key(%c) and id(%d)\n", key, ch.TextureID);

        float xpos = position.x + ch.Bearing.x * scale;
        float ypos = position.y - (ch.Size.y - ch.Bearing.y) * scale;

        float w = ch.Size.x * scale;
        float h = ch.Size.y * scale;

        float our_xpos = xpos + w / 2;
        float our_ypos = ypos + h / 2;

        float our_w = w/2;
        float our_h = h/2;

        glm::mat4 translated = glm::translate(glm::vec3(our_xpos, our_ypos, 0));

        auto u_mvp = camera.vpMatrix * translated * glm::scale(glm::vec3(our_w, our_h, 1));
        // auto u_mvp = camera.projectionMatrix;
        // auto u_mvp = camera.vpMatrix;
        glUniformMatrix4fv(renderer.textShader.u_mvp, 1, GL_FALSE, &u_mvp[0][0]);

        // render glyph texture over quad
        // update content of VBO memory

        // render quad
        // glDrawArrays(GL_TRIANGLES, 0, 6);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (void*)0);
        // now advance cursors for next glyph (note that advance is number of 1/64 pixels)
        position.x += (ch.Advance >> 6) * scale; // bitshift by 6 to get value in pixels (2^6 = 64)
    }

    glDisable(GL_BLEND);

    // glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glEnable(GL_DEPTH_TEST);

}
