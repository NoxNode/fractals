#version 410

layout(location=0) in vec4 a_position;
layout(location=1) in vec4 a_uvCoords;
// in vec4 a_normal;
// in vec4 a_tangent;
// in vec4 a_bitangent;
// in vec4 a_jointIndices;
// in vec4 a_jointWeights;

out vec2 texCoords;
uniform mat4 u_mvp;

void main()
{
    gl_Position = u_mvp * vec4(a_position.xy, 0.0, 1.0);
    texCoords = a_uvCoords.xy;
}