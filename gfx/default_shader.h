#pragma once
#include "shader.h"
#include "vertex.h"

struct DefaultShader {
    GLuint program;

    // matrices
    GLuint u_modelMatrix;
    GLuint u_normalMatrix;
    GLuint u_mvpMatrix;
    GLuint u_mvpMatrixFromLight;
    
    GLuint u_cameraPos;

    GLuint u_jointTransforms;

    // material
    GLuint u_texture;
    GLuint u_normalMap;
    GLuint u_dispMap;
    GLuint u_dispMapScale;
    GLuint u_dispMapBias;
    GLuint u_shine;
    GLuint u_color;

    // directional light
    GLuint u_lightDir;
    GLuint u_lightColor;
    GLuint u_shadowMap;

    // point / spot lights
    GLuint u_nLights;

    // flags
    GLuint lighting_enabled;
    GLuint diffuse_lighting_enabled;
    GLuint specular_lighting_enabled;
    GLuint ambient_lighting_enabled;
    GLuint texture_mapping_enabled;
    GLuint normal_mapping_enabled;
    GLuint displacement_mapping_enabled;
    GLuint shadow_mapping_enabled;
    GLuint pcf_enabled;
    GLuint shadow_cube_mapping_enabled;
    GLuint skeletal_animations_enabled;
};

void InitDefaultShader(DefaultShader& shader) {
    InitShader(shader.program, "shaders/defaultVS.glsl", "shaders/defaultFS.glsl");
    
    shader.u_modelMatrix = glGetUniformLocation(shader.program, "u_modelMatrix");
    shader.u_normalMatrix = glGetUniformLocation(shader.program, "u_normalMatrix");
    shader.u_mvpMatrix = glGetUniformLocation(shader.program, "u_mvpMatrix");
    shader.u_mvpMatrixFromLight = glGetUniformLocation(shader.program, "u_mvpMatrixFromLight");
    
    shader.u_cameraPos = glGetUniformLocation(shader.program, "u_cameraPos");
    
    shader.u_jointTransforms = glGetUniformLocation(shader.program, "u_jointTransforms");
    
    shader.u_texture = glGetUniformLocation(shader.program, "u_texture");
    shader.u_normalMap = glGetUniformLocation(shader.program, "u_normalMap");
    shader.u_dispMap = glGetUniformLocation(shader.program, "u_dispMap");
    shader.u_dispMapScale = glGetUniformLocation(shader.program, "u_dispMapScale");
    glUniform1f(shader.u_dispMapScale, 0.04f);
    shader.u_dispMapBias = glGetUniformLocation(shader.program, "u_dispMapBias");
    glUniform1f(shader.u_dispMapBias, -0.02f);
    shader.u_shine = glGetUniformLocation(shader.program, "u_shine");
    glUniform1f(shader.u_shine, 100.0f);
    shader.u_color = glGetUniformLocation(shader.program, "u_color");
    v3 defaultColor = v3(1,1,1);
    glUniform3fv(shader.u_color, 1, &defaultColor[0]);

    shader.u_lightDir = glGetUniformLocation(shader.program, "u_lightDir");
    shader.u_lightColor = glGetUniformLocation(shader.program, "u_lightColor");
    shader.u_shadowMap = glGetUniformLocation(shader.program, "u_shadowMap");
    
    shader.u_nLights = glGetUniformLocation(shader.program, "u_nLights");
    glUniform1i(shader.u_nLights, 0);

    shader.lighting_enabled = glGetUniformLocation(shader.program, "lighting_enabled");
    glUniform1i(shader.lighting_enabled, 1);
    shader.diffuse_lighting_enabled = glGetUniformLocation(shader.program, "diffuse_lighting_enabled");
    glUniform1i(shader.diffuse_lighting_enabled, 1);
    shader.specular_lighting_enabled = glGetUniformLocation(shader.program, "specular_lighting_enabled");
    glUniform1i(shader.specular_lighting_enabled, 1);
    shader.ambient_lighting_enabled = glGetUniformLocation(shader.program, "ambient_lighting_enabled");
    glUniform1i(shader.ambient_lighting_enabled, 1);
    shader.texture_mapping_enabled = glGetUniformLocation(shader.program, "texture_mapping_enabled");
    glUniform1i(shader.texture_mapping_enabled, 1);
    shader.normal_mapping_enabled = glGetUniformLocation(shader.program, "normal_mapping_enabled");
    glUniform1i(shader.normal_mapping_enabled, 1);
    shader.displacement_mapping_enabled = glGetUniformLocation(shader.program, "displacement_mapping_enabled");
    glUniform1i(shader.displacement_mapping_enabled, 1);
    shader.shadow_mapping_enabled = glGetUniformLocation(shader.program, "shadow_mapping_enabled");
    glUniform1i(shader.shadow_mapping_enabled, 1);
    shader.pcf_enabled = glGetUniformLocation(shader.program, "pcf_enabled");
    glUniform1i(shader.pcf_enabled, 1);
    shader.shadow_cube_mapping_enabled = glGetUniformLocation(shader.program, "shadow_cube_mapping_enabled");
    glUniform1i(shader.shadow_cube_mapping_enabled, 0);
    shader.skeletal_animations_enabled = glGetUniformLocation(shader.program, "skeletal_animations_enabled");
    glUniform1i(shader.skeletal_animations_enabled, 0);
}
