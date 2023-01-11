#pragma once
#include <GL/glew.h>
#include "vertex.h"
#include "key_frame.h"

#define MAX_VERTICES 500000
#define MAX_INDICES  2000000

typedef u32 IndexType;

struct VBO {
	GLuint id;
	u32 verticesOffset;
	Vertex vertices[MAX_VERTICES];
};

struct IBO {
	GLuint id;
	u32 indicesOffset;
	IndexType indices[MAX_INDICES];
};

#define MAX_ANIMATIONS 32
#define MAX_KEYFRAMES_PER_ANIMATION 512

#define MAX_JOINTS 128
// see key_frame.h for MAX_JOINTS_PER_MODEL
struct JointBuffers {
	// this is the joint info we use to calculate the joint info we need to send to the shader
	u32 jointsOffset;
	mat4 invJointTransforms[MAX_JOINTS];
	IndexType jointParents[MAX_JOINTS];
	mat4 boneSpaceJointTransforms[MAX_JOINTS];

	// this is the joint info we sent to the shader
	mat4 jointTransforms[MAX_JOINTS_PER_MODEL];

	// this is animation keyframe info we use to show animations
	u32 animationsOffset;
	KeyFrame animationKeyFrames[MAX_ANIMATIONS][MAX_KEYFRAMES_PER_ANIMATION];
	u32 numKeyFramesInAnimation[MAX_ANIMATIONS];
};

void InitGLBuffers(VBO& vbo, IBO& ibo) {
	vbo.verticesOffset = 0;
	ibo.indicesOffset = 0;

	glGenBuffers(1, &vbo.id);
	glBindBuffer(GL_ARRAY_BUFFER, vbo.id);

	glGenBuffers(1, &ibo.id);
}

void FillGLBuffers(VBO& vbo, IBO& ibo) {
	glBindBuffer(GL_ARRAY_BUFFER, vbo.id);
	glBufferData(GL_ARRAY_BUFFER, vbo.verticesOffset * sizeof(Vertex), vbo.vertices, GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo.id);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, ibo.indicesOffset * sizeof(IndexType), ibo.indices, GL_STATIC_DRAW);
}

void DeinitGLBuffers(VBO& vbo, IBO& ibo) {
	glDeleteBuffers(1, &vbo.id);
	glDeleteBuffers(1, &ibo.id);
}
