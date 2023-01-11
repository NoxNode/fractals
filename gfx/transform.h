#pragma once
#include "../core/types.h"
#include <glm/gtx/matrix_decompose.hpp>

struct Transform {
    vec3 pos;
	vec3 rot;
	vec3 scale;
	vec3 pivot;
    mat4 matrix;
	
	// TODO: use quaternions
	quat rotq;
};

void FillFromMatrix(Transform& t, mat4& mat) {
	glm::vec4 perspective; // throwaway perspective
	glm::decompose(mat, t.scale, t.rotq, t.pos, t.pivot, perspective);
	// printf("persp: %f, %f, %f\n", perspective.x, perspective.y, perspective.z);
}

void CalcTransformMatrix(Transform& t) {
	t.matrix = translate(t.pos);
	// t.matrix *= toMat4(t.rot);
	t.matrix *= rotate(t.rot.x, vec3(1, 0, 0));
	t.matrix *= rotate(t.rot.y, vec3(0, 1, 0));
	t.matrix *= rotate(t.rot.z, vec3(0, 0, 1));
	t.matrix *= translate(-t.pivot);
	t.matrix *= scale(t.scale);
}

void CalcTransformMatrixWithQuat(Transform& t) {
	t.matrix = translate(t.pos);
	t.matrix *= toMat4(t.rotq);
	// t.matrix *= rotate(t.rot.x, vec3(1, 0, 0));
	// t.matrix *= rotate(t.rot.y, vec3(0, 1, 0));
	// t.matrix *= rotate(t.rot.z, vec3(0, 0, 1));
	t.matrix *= translate(-t.pivot);
	t.matrix *= scale(t.scale);
}

void CalcTransformMatrixWithPosAndRotqOnly(Transform& t) {
	t.matrix = translate(t.pos) * toMat4(t.rotq);
}

void CalcTransformMatrixWithLookAtRot(Transform& transform, vec3 lookAtDir) {
	mat4 lookAtCalc = lookAt(
		vec3(0.0f, 0.0f, 0.0f),
		lookAtDir,
		vec3(0.0f, 1.0f, 0.0f));
	transform.matrix = translate(transform.pos);
	transform.matrix *= inverse(lookAtCalc);
	transform.matrix *= translate(-transform.pivot);
	transform.matrix *= scale(transform.scale);
}
