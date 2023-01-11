#pragma once
#include "joint_state.h"
#include "model.h"

// #define MAX_TARGETS_PER_JOINT 20 // dont know what this is for

struct RenderObj {
	// for everyting
	Transform transform; // modelMatrix == transform.matrix

	// for rendering
	mat4 normalMatrix; // transpose of the inverse of the model matrix
    Model* model;
    Material* material;
	
	// for skeletal animation
	JointState* jointStates; // either NULL or an array of size MAX_JOINTS_PER_MODEL based on if the model is rigged or not
	s32 curAnimation; // -1 for no animation
	s32 nextAnimation; // play this animation after finishing curAnimation (set = curAnimation for loop)
	r32 animationTime; // how far into the curAnimation are we
	u32 prevKeyFrameIndex;
	u32 nextKeyFrameIndex;
	// TODO: dynamic animations / ragdolls

	// for collisin / frustum culling
	vec3 minExtents, maxExtents;

	// for physics
	r32 invMass;
	vec3 velocity;
	vec3 impulseAccumulator;
};

/** must have called CalcJointTransforms before this function */
void GetExtents(RenderObj& renderObj, Vertex* vertices, JointBuffers& jointBuffers) {
	renderObj.maxExtents = vec3(FLT_MIN, FLT_MIN, FLT_MIN);
	renderObj.minExtents = vec3(FLT_MAX, FLT_MAX, FLT_MAX);
	for(int i = 0; i < renderObj.model->numVertices; i++) {
		vec3 vertexPos = vertices[renderObj.model->verticesOffset + i].pos;
		mat4 transformMat = renderObj.transform.matrix;
		// transform vertexPos by joint state
		if(renderObj.jointStates != NULL) {
			// get weights and indices
			vec4 weights = vertices[renderObj.model->verticesOffset + i].jointWeights;
			vec4 indices = vertices[renderObj.model->verticesOffset + i].jointIndices;
			mat4 jointTransform = mat4(0);
			for(int j = 0; j < 4; j++) {
				if(indices[j] <= 0.0f) continue;
				jointTransform += jointBuffers.jointTransforms[(int)indices[j]] * weights[j];
			}
			transformMat = transformMat * jointTransform;
		}
		// transformed vertexPos
		vertexPos = vec3(transformMat * vec4(vertexPos, 1.0f));
		for(int i = 0; i < 3; i++) {
			if(vertexPos[i] > renderObj.maxExtents[i]) {
				renderObj.maxExtents[i] = vertexPos[i];
			}
			if(vertexPos[i] < renderObj.minExtents[i]) {
				renderObj.minExtents[i] = vertexPos[i];
			}
		}
	}
}

void UpdateMatrices(RenderObj& renderObj) {
	CalcTransformMatrix(renderObj.transform);
	renderObj.normalMatrix = transpose(inverse(renderObj.transform.matrix));
}

void InitRenderObj(RenderObj& renderObj, Model* model, Material* material, v3 pos = v3(0,0,0), v3 rot = v3(0,0,0), v3 scale = v3(1,1,1), v3 pivot = v3(0,0,0)) {
	renderObj.model = model;
	renderObj.material = material;

	renderObj.transform.pos = pos;
	renderObj.transform.rot = rot;
	renderObj.transform.scale = scale;
	renderObj.transform.pivot = pivot;
	UpdateMatrices(renderObj);
	
	renderObj.invMass = 0.5f;
	renderObj.velocity = vec3(0.0f);
	renderObj.impulseAccumulator = vec3(0.0f);

	renderObj.curAnimation = -1;
	renderObj.animationTime = 0;
	renderObj.prevKeyFrameIndex = 0;
	renderObj.nextKeyFrameIndex = 1;
}

void CalcInitialJointStates(RenderObj& renderObj, JointBuffers& jointBuffers) {
	if(renderObj.jointStates == NULL)
		return;

	int jointsOffset = renderObj.model->jointsOffset;
	int numJoints = renderObj.model->numJoints;

	mat4 jointTransforms[numJoints];
	for(int i = 0; i < numJoints; i++) {
		IndexType parentIndex = jointBuffers.jointParents[jointsOffset + i];
		
		renderObj.jointStates[i].transform.matrix = jointBuffers.boneSpaceJointTransforms[jointsOffset + i];
		if(parentIndex == -1) {
			jointTransforms[i] = renderObj.jointStates[i].transform.matrix;
		}
		else {
			jointTransforms[i] = jointTransforms[parentIndex] * renderObj.jointStates[i].transform.matrix;
		}
		// probably don't need to do this - invJoinTransforms should already be filled when loading the dae
		jointBuffers.invJointTransforms[jointsOffset + i] = inverse(jointTransforms[i]);
	}
}

void CalcJointTransforms(RenderObj& renderObj, JointBuffers& jointBuffers) {
	if(renderObj.jointStates == NULL)
		return;

	int jointsOffset = renderObj.model->jointsOffset;
	int numJoints = renderObj.model->numJoints;

	// loop through this model's joints and set the renderObj's jointStates accordingly
	for(int i = 0; i < numJoints; i++) {
		IndexType parentIndex = jointBuffers.jointParents[jointsOffset + i];

		// CalcTransformMatrix(renderObj.jointStates[i].transform);
		// CalcTransformMatrixWithQuat(renderObj.jointStates[i].transform);
		// CalcTransformMatrixWithPosAndRotqOnly(renderObj.jointStates[i].transform);

		if(parentIndex == -1) {
			jointBuffers.jointTransforms[i] = renderObj.jointStates[i].transform.matrix;
		}
		else {
			jointBuffers.jointTransforms[i] = jointBuffers.jointTransforms[parentIndex] * renderObj.jointStates[i].transform.matrix;
		}
	}
	for(int i = 0; i < numJoints; i++) {
		jointBuffers.jointTransforms[i] = jointBuffers.jointTransforms[i] * jointBuffers.invJointTransforms[jointsOffset + i];
	}
}

void FindKeyFramesForInterpolation(RenderObj& obj, JointBuffers& jointBuffers) {
	if(obj.curAnimation == -1) return;
	u32 numKeyFrames = jointBuffers.numKeyFramesInAnimation[obj.curAnimation];
	if(numKeyFrames == 0) return;
	// if farther than the last key frame's timestamp
	while(obj.animationTime > jointBuffers.animationKeyFrames[obj.curAnimation][numKeyFrames - 1].timestamp) {
		if(obj.nextAnimation != -1) {
			// if there's a next animation lined up, do that one starting at time 0
			obj.curAnimation = obj.nextAnimation;
			obj.animationTime = 0;
		}
		else {
			// subtract instead of set to 0 to keep offset smooth
			obj.animationTime -= jointBuffers.animationKeyFrames[obj.curAnimation][numKeyFrames - 1].timestamp;
		}
		obj.prevKeyFrameIndex = 0;
		obj.nextKeyFrameIndex = 1;
	}
	while(obj.animationTime > jointBuffers.animationKeyFrames[obj.curAnimation][obj.nextKeyFrameIndex].timestamp) {
		obj.prevKeyFrameIndex++;
		obj.nextKeyFrameIndex++;
		if(obj.nextKeyFrameIndex == numKeyFrames) {
			printf("p sure this shouldnt ever print\n");
			obj.prevKeyFrameIndex = 0;
			obj.nextKeyFrameIndex = 1;
		}
	}
}

r32 GetInterpolationValue(KeyFrame& prev, KeyFrame& next, r32 animationTime) {
	return (animationTime - prev.timestamp) / (next.timestamp - prev.timestamp);
}

void InterpolateKeyFrames(KeyFrame& prev, KeyFrame& next, r32 t, JointState* jointStates) {
	for(int i = 0; i < MAX_JOINTS_PER_MODEL; i++) {
		jointStates[i].transform.matrix = prev.jointTransforms[i].matrix * (1.0f - t) + next.jointTransforms[i].matrix * t;
	}
}
