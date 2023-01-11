#version 410

#define MAX_JOINTS_PER_MODEL 100
#define MAX_WEIGHTS 4

// attribute vec4 a_position;
// uniform mat4 u_mvpMatrix;

// void main() {
// 	gl_Position = u_mvpMatrix * a_position;
// }
layout(location=0) in vec4 a_position;
layout(location=5) in vec4 a_jointIndices;
layout(location=6) in vec4 a_jointWeights;

uniform mat4 u_mvpMatrix;
uniform mat4 u_jointTransforms[MAX_JOINTS_PER_MODEL];
uniform bool skeletal_animations_enabled;

void main() {
	mat4 jointTransform = mat4(1.0);
	if(skeletal_animations_enabled) {
		jointTransform = mat4(0);
		for(int i = 0; i < MAX_WEIGHTS; i++) {
			if(a_jointWeights[i] <= 0.0f) continue;
			jointTransform += u_jointTransforms[int(a_jointIndices[i])] * a_jointWeights[i];
		}
	}
	gl_Position = u_mvpMatrix * jointTransform * a_position;
}
