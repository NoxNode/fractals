#pragma once
#include "gl_buffers.h"
#include <GL/glew.h>

struct Model {
    u32 verticesOffset;
    u32 numVertices;

    u32 indicesOffset;
    u32 numIndices;

	u32 jointsOffset;
	u32 numJoints;
	// bool jointsInDFSOrder; // (TODO) if true, we know we don't have to search to know the order in which we should calculate joint transforms

	u32 animationsOffset;
	u32 numAnimations;
};

struct IndexedModel {
	vector<vec3> positions;
	vector<vec2> uvCoords;
	vector<vec3> normals;
	vector<IndexType>  indices;
	vector<vec4> jointIndices;
	vector<vec4> jointWeights;
};

struct RiggedModel {
    IndexedModel iModel;
	vector<vec4> jointIndices;
	vector<vec4> jointWeights;
	vector<mat4> invJointTransforms;
	vector<IndexType> jointParents;
	vector<mat4> boneSpaceJointTransforms;
	map<string, IndexType> jointNamesToIndices;
	map<string, IndexType> jointNodeNamesToIndices;

	// vector from jointIndex to animationIndex to keyFrameIndex to jointTransform
	vector<vector<vector<mat4>>> animationKeyFrameTransforms;
	// vector from animationIndex to keyFrameIndex to timestamp
	vector<vector<r32>> animationKeyFrameTimestamps;
};

struct Face_Element {
	IndexType posIndex;
	IndexType uvCoordIndex;
	IndexType normalIndex;
};

struct Face_Element_Comp {
	bool operator() (const Face_Element& lhs, const Face_Element& rhs) const {
		return lhs.posIndex<rhs.posIndex ||
			lhs.uvCoordIndex<rhs.uvCoordIndex ||
			lhs.normalIndex<rhs.normalIndex;
	}
};

struct Face {
	vector<IndexType> posIndices;
	vector<IndexType> uvCoordIndices;
	vector<IndexType> normalIndices;
};

struct OBJModel {
	vector<vec3> positions;
	vector<vec2> uvCoords;
	vector<vec3> normals;
	vector<Face> faces;
};

void CalculateTangents(Model& model, const IndexedModel& indexedModel, Vertex* vertices) {
	if(indexedModel.uvCoords.size() == 0)
		return;
	for(u32 i = 0; i < indexedModel.indices.size(); i += 3) {
		vec3 p0 = indexedModel.positions[indexedModel.indices[i + 0]];
		vec3 p1 = indexedModel.positions[indexedModel.indices[i + 1]];
		vec3 p2 = indexedModel.positions[indexedModel.indices[i + 2]];

		vec3 Edge1 = p1 - p0;
		vec3 Edge2 = p2 - p0;

		vec2 t0 = indexedModel.uvCoords[indexedModel.indices[i + 0]];
		vec2 t1 = indexedModel.uvCoords[indexedModel.indices[i + 1]];
		vec2 t2 = indexedModel.uvCoords[indexedModel.indices[i + 2]];

		float DeltaU1 = t1.x - t0.x;
		float DeltaV1 = t1.y - t0.y;
		float DeltaU2 = t2.x - t0.x;
		float DeltaV2 = t2.y - t0.y;

		float f = 1.0f / (DeltaU1 * DeltaV2 - DeltaU2 * DeltaV1);

		vec3 tangent;
		tangent.x = f * (DeltaV2 * Edge1.x - DeltaV1 * Edge2.x);
		tangent.y = f * (DeltaV2 * Edge1.y - DeltaV1 * Edge2.y);
		tangent.z = f * (DeltaV2 * Edge1.z - DeltaV1 * Edge2.z);

		// vec3 bitangent;
		// bitangent.x = f * (-DeltaU2 * Edge1.x - DeltaU1 * Edge2.x);
		// bitangent.y = f * (-DeltaU2 * Edge1.y - DeltaU1 * Edge2.y);
		// bitangent.z = f * (-DeltaU2 * Edge1.z - DeltaU1 * Edge2.z);

		for(u32 j = 0; j < 3; j++) {
			u32 index = indexedModel.indices[i + j];
			vertices[model.verticesOffset + index].tangent.x += tangent.x;
			vertices[model.verticesOffset + index].tangent.y += tangent.y;
			vertices[model.verticesOffset + index].tangent.z += tangent.z;

			// vertices[model.verticesOffset + index].bitangent.x += bitangent.x;
			// vertices[model.verticesOffset + index].bitangent.y += bitangent.y;
			// vertices[model.verticesOffset + index].bitangent.z += bitangent.z;
		}
	}
}

void LoadModelToBuffers(Model& model, RiggedModel* riggedModel, VBO& vbo, IBO& ibo, JointBuffers& jointBuffers) {
	IndexedModel& indexedModel = riggedModel->iModel;

	model.verticesOffset = vbo.verticesOffset;
	model.indicesOffset = ibo.indicesOffset;

    model.numVertices = indexedModel.positions.size();
    model.numIndices = indexedModel.indices.size();
	if(model.verticesOffset + model.numVertices > MAX_VERTICES) {
		printf("max vertices exceeded when loading model to buffers\n");
		exit(1);
	}
	if(model.indicesOffset + model.numIndices > MAX_INDICES) {
		printf("max indices exceeded when loading model to buffers\n");
		exit(1);
	}

	for(u32 i = 0; i < indexedModel.positions.size(); i++) {
		vbo.vertices[model.verticesOffset + i].pos = indexedModel.positions[i];
	}
	for(u32 i = 0; i < indexedModel.uvCoords.size(); i++) {
		vbo.vertices[model.verticesOffset + i].uvCoords = indexedModel.uvCoords[i];
	}
	for(u32 i = 0; i < indexedModel.normals.size(); i++) {
		vbo.vertices[model.verticesOffset + i].normal = indexedModel.normals[i];
	}
	for(u32 i = 0; i < indexedModel.jointIndices.size(); i++) {
		vbo.vertices[model.verticesOffset + i].jointIndices = indexedModel.jointIndices[i];
	}
	for(u32 i = 0; i < indexedModel.jointWeights.size(); i++) {
		vbo.vertices[model.verticesOffset + i].jointWeights = indexedModel.jointWeights[i];
	}

	CalculateTangents(model, indexedModel, vbo.vertices);

	for(u32 i = 0; i < indexedModel.indices.size(); i++) {
		ibo.indices[model.indicesOffset + i] = model.verticesOffset + indexedModel.indices[i];
	}

	vbo.verticesOffset += model.numVertices;
	ibo.indicesOffset += model.numIndices;

	// load joint info
	model.jointsOffset = jointBuffers.jointsOffset;
	model.numJoints = riggedModel->jointParents.size();
	if(model.jointsOffset + model.numJoints > MAX_JOINTS) {
		printf("max joints exceeded when loading model to buffers\n");
		exit(1);
	}
	if(model.numJoints > MAX_JOINTS_PER_MODEL) {
		printf("max joints per model exceeded when loading model to buffers\n");
		exit(1);
	}

	for(int i = 0; i < model.numJoints; i++) {
		jointBuffers.jointParents[model.jointsOffset + i] = riggedModel->jointParents[i];
		jointBuffers.invJointTransforms[model.jointsOffset + i] = riggedModel->invJointTransforms[i];
		jointBuffers.boneSpaceJointTransforms[model.jointsOffset + i] = riggedModel->boneSpaceJointTransforms[i];
	}
	jointBuffers.jointsOffset += model.numJoints;

	// load animation keyframes
	model.animationsOffset = jointBuffers.animationsOffset;
	model.numAnimations = riggedModel->animationKeyFrameTimestamps.size();
	if(model.animationsOffset + model.numAnimations > MAX_ANIMATIONS) {
		printf("max animations exceeded when loading model to buffers\n");
		exit(1);
	}
	for(int i = 0; i < model.numAnimations; i++) {
		int numKeyFrames = riggedModel->animationKeyFrameTimestamps[i].size();
		if(numKeyFrames > MAX_KEYFRAMES_PER_ANIMATION) {
			printf("max key frames in an animation exceeded when loading model to buffers\n");
			exit(1);
		}
		jointBuffers.numKeyFramesInAnimation[i] = numKeyFrames;
		for(int j = 0; j < numKeyFrames; j++) {
			jointBuffers.animationKeyFrames[i][j].timestamp = riggedModel->animationKeyFrameTimestamps[i][j] - riggedModel->animationKeyFrameTimestamps[i][0];
			for(int k = 0; k < model.numJoints; k++) {
				// if the animations don't animate certain joints, make sure to initialize their transforms to the identity matrix
				if(riggedModel->animationKeyFrameTransforms[k][i].size() == 0) {
					jointBuffers.animationKeyFrames[i][j].jointTransforms[k].matrix = mat4(1.0f);
					continue;
				}
				jointBuffers.animationKeyFrames[i][j].jointTransforms[k].matrix = riggedModel->animationKeyFrameTransforms[k][i][j];
			}
		}
	}
	jointBuffers.animationsOffset += model.numAnimations;
}

void AddFaceElem(RiggedModel* rModel, OBJModel& obj_model, Face_Element& faceElem, map<Face_Element, IndexType, Face_Element_Comp>& faceMap, u32& f, u32 p) {
	// set the current face elem
	faceElem.posIndex = obj_model.faces[f].posIndices[p];
	if(obj_model.faces[f].uvCoordIndices.size() > 0) {
		faceElem.uvCoordIndex = obj_model.faces[f].uvCoordIndices[p];
	}
	if(obj_model.faces[f].normalIndices.size() > 0) {
		faceElem.normalIndex = obj_model.faces[f].normalIndices[p];
	}
	// see if insert fails because there's we've already encountered this face elem
	auto res = faceMap.insert(pair<Face_Element, IndexType>(faceElem, rModel->iModel.positions.size()));
	if(res.second == true) {
		// not yet encountered, so add it
		rModel->iModel.indices.push_back(rModel->iModel.positions.size());

		rModel->iModel.positions.push_back(obj_model.positions[faceElem.posIndex]);
		if(obj_model.faces[f].uvCoordIndices.size() > 0) {
			rModel->iModel.uvCoords.push_back(obj_model.uvCoords[faceElem.uvCoordIndex]);
		}
		if(obj_model.faces[f].normalIndices.size() > 0) {
			rModel->iModel.normals.push_back(obj_model.normals[faceElem.normalIndex]);
		}
		if(rModel->jointIndices.size() > 0) {
			rModel->iModel.jointIndices.push_back(rModel->jointIndices[faceElem.posIndex]);
			rModel->iModel.jointWeights.push_back(rModel->jointWeights[faceElem.posIndex]);

			// ivec4 indices = rModel->jointIndices[faceElem.posIndex];
			// bool allLT2 = true;
			// for(int i = 0; i < 4; i++)
			// 	if(indices[i] >= 2) allLT2 = false;
			// if(!allLT2) {
			// 	printf("%d, %d, %d, %d\n", indices.x, indices.y, indices.z, indices.w);
			// }
		}
	}
	else {
		// already encountered, so just add an index to it
		rModel->iModel.indices.push_back(res.first->second);
	}
}

void fillIndexedModel(RiggedModel* rModel, OBJModel& obj_model) {
	// benny's obj_loader has another step, so see if that's necessary
		// the extra step is for generating normals in between the actual objmodel to indexedmodel conversion,
		// but I think we can do both in 1 step
	map<Face_Element, IndexType, Face_Element_Comp> faceMap;
	Face_Element curFaceElem = {0, 0, 0};
	for(u32 f = 0; f < obj_model.faces.size(); f++) {
		if(obj_model.faces[f].normalIndices.size() == 0) {
			// if the model has no normals, calculate them
			vec3 p1 = obj_model.positions[obj_model.faces[f].posIndices[0]];
			vec3 p2 = obj_model.positions[obj_model.faces[f].posIndices[1]];
			vec3 p3 = obj_model.positions[obj_model.faces[f].posIndices[2]];

			vec3 v1 = p2 - p1;
			vec3 v2 = p3 - p2;
			vec3 normal = normalize(glm::cross(v1, v2));

			// add the normal to the obj_model's normals if not already there
			u32 normalIndex = -1;
			for(u32 i = 0; i < obj_model.normals.size(); i++) {
				if(obj_model.normals[i] == normal) {
					normalIndex = i;
					break;
				}
			}
			if(normalIndex == -1) {
				obj_model.normals.push_back(normal);
				normalIndex = obj_model.normals.size() - 1;
			}

			// add the normal to the face's normals
			for(u32 i = 0; i < obj_model.faces[f].posIndices.size(); i++) {
				obj_model.faces[f].normalIndices.push_back(normalIndex);
			}
		}

		for(u32 p = 2; p < obj_model.faces[f].posIndices.size(); p++) {
			// connect the vertices in a triangle fan
			// TODO: see if this ever needs to be a triangle strip
			AddFaceElem(rModel, obj_model, curFaceElem, faceMap, f, 0);
			AddFaceElem(rModel, obj_model, curFaceElem, faceMap, f, p - 1);
			AddFaceElem(rModel, obj_model, curFaceElem, faceMap, f, p);
		}
	}
}
