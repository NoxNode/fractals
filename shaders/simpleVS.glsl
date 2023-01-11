#version 410

layout(location=0) in vec4 a_position;
layout(location=1) in vec4 a_uvCoords;

out vec4 v_uvCoords;

uniform mat4 u_mvpMatrix;

void main() {
	// gl_Position = u_mvpMatrix * a_position;
	gl_Position = a_position;
	// v_uvCoords = a_uvCoords;
}
