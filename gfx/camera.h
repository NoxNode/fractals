#pragma once
#include "../core/types.h"

struct Camera {
	v3 pos;
	v3 dir;
	v3 up;
	v3 right;

	mat4 projectionMatrix; // projection matrix (fov and clipping planes)
	mat4 viewMatrix; // view matrix (camera)
	mat4 vpMatrix; // view projection matrix

    // common between perspective and orthographic
	r32 nearClip;
	r32 farClip;

    // perspective
	r32 fov, aspect;

    // orthographic
    r32 leftClip, rightClip, bottomClip, topClip;
};

void RotateCamera(Camera& camera, r32 up, r32 right) {
	if(right != 0.0f) {
		camera.right = normalize(rotate(camera.right, -right, v3(0, 1, 0)));
		camera.up = normalize(rotate(camera.up, -right, v3(0, 1, 0)));
	}
	if(up != 0.0f) {
		camera.up = normalize(rotate(camera.up, up, camera.right));
	}
	camera.dir = cross(camera.up, camera.right);
	camera.up = cross(camera.right, camera.dir);
}

void UpdateMatrices(Camera& camera) {
	camera.projectionMatrix = glm::perspective(glm::radians(camera.fov), camera.aspect, camera.nearClip, camera.farClip);
	camera.viewMatrix = glm::lookAt(camera.pos, camera.pos + camera.dir, camera.up);
	camera.vpMatrix = camera.projectionMatrix * camera.viewMatrix;
}

void UpdateOrthoMatrices(Camera& camera) {
	camera.projectionMatrix = glm::ortho(camera.leftClip, camera.rightClip, camera.bottomClip, camera.topClip, camera.nearClip, camera.farClip);
	camera.viewMatrix = glm::lookAt(camera.pos, camera.pos + camera.dir, camera.up);
	camera.vpMatrix = camera.projectionMatrix * camera.viewMatrix;
}

void InitCamera(Camera& camera, v3 pos, v3 dir = v3(0, 0, -1), v3 up = v3(0, 1, 0), v3 right = v3(1, 0, 0),
r32 fov = 45.0f, r32 aspect = (800.0f / 600.0f), r32 nearClip = 0.01f, r32 farClip = 1000.0f) {
	camera.pos = pos;
	camera.dir = dir;
	// TODO: do something about if someone specifies only dir but not up or right then we auto calc that stuff
		// otherwise if we InitCamera with a different dir then call RotateCamera it will just use the default dir
	camera.up = up;
	camera.right = right;
	camera.fov = fov;
	camera.aspect = aspect;
	camera.nearClip = nearClip;
	camera.farClip = farClip;

    UpdateMatrices(camera);
}

void InitOrthoCamera(Camera& camera, v3 pos, v3 dir = v3(0, 0, -1), v3 up = v3(0, 1, 0), v3 right = v3(1, 0, 0),
r32 leftClip = -25.0f, r32 rightClip = 25.0f, r32 bottomClip = -25.0f, r32 topClip = 25.0f, r32 nearClip = 0.01f, r32 farClip = 50) {
	camera.pos = pos;
	camera.dir = dir;
	camera.up = up;
	camera.right = right;
    
    camera.leftClip = leftClip;
    camera.rightClip = rightClip;
    camera.bottomClip = bottomClip;
    camera.topClip = topClip;
	camera.nearClip = nearClip;
	camera.farClip = farClip;

    UpdateOrthoMatrices(camera);
}

void FreeMoveCamera(Camera& camera, vec3 moveRightForwardUp, vec2 rotRightUp) {
	camera.pos += camera.right * moveRightForwardUp.x;
	camera.pos += camera.dir * moveRightForwardUp.y;
	camera.pos  += camera.up * moveRightForwardUp.z;

	RotateCamera(camera, rotRightUp.y, rotRightUp.x);
}
