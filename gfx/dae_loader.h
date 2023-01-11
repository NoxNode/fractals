#pragma once
#include "model.h"
#include "../core/xml_parser.h"
#include <sstream>

void fillJointParents(vector<IndexType>& jointParents, map<string, IndexType>& jointNamesToIndices, XMLNode* parent, vector<mat4>& boneSpaceJointTransforms, map<string, IndexType>& jointNodeNamesToIndices) {
	string parentName = parent->attributes["sid"];
	int parentIndex = jointNamesToIndices[parentName];
	for(int i = 0; i < parent->childNodes.size(); i++) {
		if(parent->childNodes[i]->tag != "node" || parent->childNodes[i]->attributes["type"] != "JOINT") continue;
		string childName = parent->childNodes[i]->attributes["sid"];
		int childIndex = jointNamesToIndices[childName];
		jointNodeNamesToIndices[parent->childNodes[i]->attributes["id"]] = childIndex;
		if(childIndex != jointParents.size()) {
			printf("joints not specified in DFS manner\n");
			exit(1);
		}
		jointParents.push_back(parentIndex);
		
		string matrixNodeData = parent->childNodes[i]->childNodes[0]->data;
		{
			istringstream iss(matrixNodeData);
			string token;
			while(getline(iss, token, ' ')) {
				mat4 curMat;
				curMat[0][0] = stof(token);
				for(int j = 1; j < 16; j++) {
					getline(iss, token, ' ');
					curMat[j % 4][j / 4] = stof(token);
				}
				boneSpaceJointTransforms.push_back(curMat);
			}
		}
		fillJointParents(jointParents, jointNamesToIndices, parent->childNodes[i], boneSpaceJointTransforms, jointNodeNamesToIndices);
	}
}

// assumes vertex data has normals and tex coords and are layed out in this order: position, normal, texCoord
// can handle if it has extra data per vertex for color
// can handle polylist or triangle data format
bool LoadDAE(RiggedModel* rModel, const char* modelPath, u32* animationStartFrameIndices = NULL, s32 numAnimations = 1) {
	// load file
	XMLNode* root = loadXmlFile(modelPath);

	// fill invJointTransforms, jointIndices and jointWeights from the first controller section
	XMLNode* controllers = findNode(root, "library_controllers");
	// if no controllers, it's not an animated model
	if(controllers != NULL) {
		XMLNode* controller = findNode(controllers, "controller");
		XMLNode* skin = findNode(controller, "skin");

		// fill invJointTransforms
		XMLNode* jointsNode = findNode(skin, "joints");
		XMLNode* invBindMatrixInput = findNode(jointsNode, "input", "semantic", "INV_BIND_MATRIX");

		XMLNode* invBindMatrixSource = findNode(skin, "source", "id", invBindMatrixInput->attributes["source"].substr(1));

		string invBindMatrixData = invBindMatrixSource->childNodes[0]->data;

		{
			istringstream iss(invBindMatrixData);
			string token;
			while(getline(iss, token, ' ')) {
				mat4 curMat;
				curMat[0][0] = stof(token);
				for(int i = 1; i < 16; i++) {
					getline(iss, token, ' ');
					curMat[i % 4][i / 4] = stof(token);
				}
				rModel->invJointTransforms.push_back(curMat);
			}
		}

		// fill jointIndices and jointWeights
		XMLNode* vertexWeights = findNode(skin, "vertex_weights");
		
		XMLNode* jointNamesInput = findNode(vertexWeights, "input", "semantic", "JOINT");
		XMLNode* jointWeightsInput = findNode(vertexWeights, "input", "semantic", "WEIGHT");

		XMLNode* jointNamesSource = findNode(skin, "source", "id", jointNamesInput->attributes["source"].substr(1));
		XMLNode* jointWeightsSource = findNode(skin, "source", "id", jointWeightsInput->attributes["source"].substr(1));

		string jointNamesData = jointNamesSource->childNodes[0]->data;
		string jointWeightsData = jointWeightsSource->childNodes[0]->data;

		// get joint names
		// rModel->jointNamesToIndices;
		{
			istringstream iss(jointNamesData);
			string token;
			int i = 0;
			while(getline(iss, token, ' ')) {
				rModel->jointNamesToIndices[token] = i;
				i++;
			}
		}

		// get joint weights
		vector<float> jointWeights;
		{
			istringstream iss(jointWeightsData);
			string token;
			while(getline(iss, token, ' ')) {
				jointWeights.push_back(stof(token));
			}
		}

		XMLNode* vcountNode = findNode(vertexWeights, "vcount");
		XMLNode* v = findNode(vertexWeights, "v");

		string vcountData = vcountNode->data;
		string vData = v->data;
		istringstream iss(vcountData);
		istringstream iss2(vData);
		string vToken;
		while(getline(iss, vToken, ' ')) {
			const int vcount = stoi(vToken);
			// these are the indices and weights of up to 4 joints that influence this vertex
			vec4 indices = ivec4(-1, -1, -1, -1);
			vec4 weights = vec4(-1, -1, -1, -1);
			for(int i = 0; i < vcount; i++) {
				string tokens[2];
				getline(iss2, tokens[0], ' ');
				getline(iss2, tokens[1], ' ');
				int jointIndex = stoi(tokens[0]);
				int weightsIndex = stoi(tokens[1]);
				if(i < 4) {
					indices[i] = jointIndex;
					weights[i] = jointWeights[weightsIndex];
					continue;
				}
				// if i >= 4, and this new weight is greater than our current min weight, replace it
				int minIndex = -1;
				float minWeight = 10.0f;
				for(int j = 0; j < 4; j++) {
					if(weights[j] < minWeight) {
						minWeight = weights[j];
						minIndex = j;
					}
				}
				if(jointWeights[weightsIndex] > minWeight) {
					indices[minIndex] = jointIndex;
					weights[minIndex] = jointWeights[weightsIndex];
				}
			}
			// TODO: normalize weights
			// if there were more than 4 weights, make the sum equal to 1
			// if(vcount > 4) {
			// 	r32 sum = weights.x + weights.y + weights.z + weights.w;
			// 	r32 scalar = 1.0f / sum;
			// 	weights = weights * scalar;
			// }
			rModel->jointIndices.push_back(indices);
			rModel->jointWeights.push_back(weights);
		}
	}

	// fill indexed model's positions, uvCoords, normals, and indices from the first mesh section
	XMLNode* library_geometries = findNode(root, "library_geometries");
	XMLNode* geometry = findNode(library_geometries, "geometry");
	XMLNode* mesh = findNode(geometry, "mesh");

	XMLNode* vertices = findNode(mesh, "vertices");
	XMLNode* positionInput = findNode(vertices, "input", "semantic", "POSITION");
	if(positionInput == NULL) {
		printf("LoadDAE(%s) failed: no vertex position data found\n", modelPath);
		return false;
	}

	XMLNode* polylist = findNode(mesh, "polylist");
	if(polylist == NULL) {
		polylist = findNode(mesh, "triangles");
	}
	if(polylist == NULL) { // unsupported data format
		printf("LoadDAE(%s) failed: unsupported data format\n", modelPath);
		return false;
	}
	XMLNode* normalInput = findNode(polylist, "input", "semantic", "NORMAL");
	XMLNode* texCoordInput = findNode(polylist, "input", "semantic", "TEXCOORD");
	XMLNode* colorInput = findNode(polylist, "input", "semantic", "COLOR");

	OBJModel oModel;
	{
		XMLNode* positionSource = findNode(mesh, "source", "id", positionInput->attributes["source"].substr(1));
		string positionData = positionSource->childNodes[0]->data;
		istringstream iss(positionData);
		string tokens[3];
		while(getline(iss, tokens[0], ' ') && getline(iss, tokens[1], ' ') && getline(iss, tokens[2], ' ')) {
			vec3 pos;
			pos.x = stof(tokens[0]);
			pos.y = stof(tokens[1]);
			pos.z = stof(tokens[2]);
			oModel.positions.push_back(pos);
		}
	}

	if(normalInput != NULL) {
		XMLNode* normalSource = findNode(mesh, "source", "id", normalInput->attributes["source"].substr(1));
		string normalData = normalSource->childNodes[0]->data;
		istringstream iss(normalData);
		string tokens[3];
		while(getline(iss, tokens[0], ' ') && getline(iss, tokens[1], ' ') && getline(iss, tokens[2], ' ')) {
			vec3 normals;
			normals.x = stof(tokens[0]);
			normals.y = stof(tokens[1]);
			normals.z = stof(tokens[2]);
			oModel.normals.push_back(normals);
		}
	}

	if(texCoordInput != NULL) {
		XMLNode* texCoordSource = findNode(mesh, "source", "id", texCoordInput->attributes["source"].substr(1));
		string texCoordData = texCoordSource->childNodes[0]->data;
		istringstream iss(texCoordData);
		string tokens[2];
		while(getline(iss, tokens[0], ' ') && getline(iss, tokens[1], ' ')) {
			vec2 uvCoords;
			uvCoords.x = stof(tokens[0]);
			uvCoords.y = stof(tokens[1]);
			oModel.uvCoords.push_back(uvCoords);
		}
	}
	
	if(polylist->tag == "triangles") {
		XMLNode* p = findNode(polylist, "p");
		string indexData = p->data;

		int triangleCount = stoi(polylist->attributes["count"]);
		istringstream iss2(indexData);
		for(int t = 0; t < triangleCount; t++) {
			const int vcount = 3;
			Face f;
			for(int i = 0; i < vcount; i++) {
				string tokens[3];
				getline(iss2, tokens[0], ' ');
				f.posIndices.push_back((IndexType) stoi(tokens[0]));

				if(normalInput != NULL) {
					getline(iss2, tokens[1], ' ');
					f.normalIndices.push_back((IndexType) stoi(tokens[1]));
				}

				if(texCoordInput != NULL) {
					getline(iss2, tokens[2], ' ');
					f.uvCoordIndices.push_back((IndexType) stoi(tokens[2]));
				}

				// skip color data
				if(colorInput != NULL) {
					getline(iss2, tokens[2], ' ');
				}
			}
			oModel.faces.push_back(f);
		}
	}
	else { // polylist->tag == "polylist"
		XMLNode* vcountNode = findNode(polylist, "vcount");
		XMLNode* p = findNode(polylist, "p");
		string vcountData = vcountNode->data;
		string indexData = p->data;

		istringstream iss(vcountData);
		istringstream iss2(indexData);
		string vToken;
		while(getline(iss, vToken, ' ')) {
			// printf("hit: %s\n", vToken.c_str());
			const int vcount = stoi(vToken);
			Face f;
			for(int i = 0; i < vcount; i++) {
				string tokens[3];
				getline(iss2, tokens[0], ' ');
			// printf("hit1: %s\n", tokens[0].c_str());
				f.posIndices.push_back((IndexType) stoi(tokens[0]));

				if(normalInput != NULL) {
					getline(iss2, tokens[1], ' ');
			// printf("hit2: %s\n", tokens[1].c_str());
					f.normalIndices.push_back((IndexType) stoi(tokens[1]));
				}

				if(texCoordInput != NULL) {
					getline(iss2, tokens[2], ' ');
			// printf("hit3: %s\n", tokens[2].c_str());
					f.uvCoordIndices.push_back((IndexType) stoi(tokens[2]));
				}

				// skip color data
				if(colorInput != NULL) {
					getline(iss2, tokens[2], ' ');
				}
			}
			oModel.faces.push_back(f);
		}
	}

	fillIndexedModel(rModel, oModel);

	// fill jointParents from the first visual scene section
	XMLNode* library_visual_scene = findNode(root, "library_visual_scenes");
	XMLNode* visual_scene = findNode(library_visual_scene, "visual_scene");
	XMLNode* rootJointNode = findNode(visual_scene, "node", "type", "JOINT");
	if(rootJointNode != NULL) {
		int rootJointIndex = rModel->jointNamesToIndices[rootJointNode->attributes["sid"]];
		rModel->jointNodeNamesToIndices[rootJointNode->attributes["id"]] = rootJointIndex;
		rModel->jointParents.push_back(-1);

		string matrixNodeData = rootJointNode->childNodes[0]->data;
		{
			istringstream iss(matrixNodeData);
			string token;
			while(getline(iss, token, ' ')) {
				mat4 curMat;
				curMat[0][0] = stof(token);
				for(int i = 1; i < 16; i++) {
					getline(iss, token, ' ');
					curMat[i % 4][i / 4] = stof(token);
				}
				rModel->boneSpaceJointTransforms.push_back(curMat);
			}
		}
		fillJointParents(rModel->jointParents, rModel->jointNamesToIndices, rootJointNode, rModel->boneSpaceJointTransforms, rModel->jointNodeNamesToIndices);
	}

	// load animations
	XMLNode* library_animations = findNode(root, "library_animations");
	// handle case of root animation node
	if(library_animations->childNodes.size() == 1) {
		library_animations = findNode(library_animations, "animation");
	}
	for(int i = 0; i < rModel->jointParents.size(); i++) {
		rModel->animationKeyFrameTransforms.push_back(vector<vector<mat4>>());
	}
	for(int i = 0; i < numAnimations; i++) {
		rModel->animationKeyFrameTimestamps.push_back(vector<r32>());
		for(int j = 0; j < rModel->jointParents.size(); j++) {
			rModel->animationKeyFrameTransforms[j].push_back(vector<mat4>());
		}
	}
	for(int i = 0; i < library_animations->childNodes.size(); i++) {
		XMLNode* animation = library_animations->childNodes[i];
		if(animation->tag != "animation") continue;

		XMLNode* channel = findNode(animation, "channel");
		string jointNodeName = channel->attributes["target"];
		jointNodeName.erase(jointNodeName.end() - 10, jointNodeName.end()); // remove the /transform at the end
		int jointIndex = rModel->jointNodeNamesToIndices[jointNodeName];

		XMLNode* sampler = findNode(animation, "sampler");

		// only need to load the timing data once - it's the same for every joint
		if(i == 0) {
			XMLNode* timingInput = findNode(sampler, "input", "semantic", "INPUT");
			XMLNode* timingSource = findNode(animation, "source", "id", timingInput->attributes["source"].substr(1));
			string timingData = timingSource->childNodes[0]->data;
			{
				istringstream iss(timingData);
				string token;
				int curAnimation = 0;
				int curFrame = 0;
				while(getline(iss, token, ' ')) {
					if(curAnimation + 1 < numAnimations && curFrame >= animationStartFrameIndices[curAnimation + 1]) {
						curAnimation++;
					}
					r32 timestamp = stof(token);
					rModel->animationKeyFrameTimestamps[curAnimation].push_back(timestamp);
					curFrame++;
				}
			}
		}
		// load the joint transforms for this current joint for all animations
		XMLNode* jointTransformOutput = findNode(sampler, "input", "semantic", "OUTPUT");
		XMLNode* jointTransformSource = findNode(animation, "source", "id", jointTransformOutput->attributes["source"].substr(1));
		string jointTransformData = jointTransformSource->childNodes[0]->data;
		{
			istringstream iss(jointTransformData);
			string token;
			int curAnimation = 0;
			int curFrame = 0;
			while(getline(iss, token, ' ')) {
				if(curAnimation + 1 < numAnimations && curFrame >= animationStartFrameIndices[curAnimation + 1]) {
					curAnimation++;
				}
				mat4 curMat;
				curMat[0][0] = stof(token);
				for(int j = 1; j < 16; j++) {
					getline(iss, token, ' ');
					curMat[j % 4][j / 4] = stof(token);
				}
				rModel->animationKeyFrameTransforms[jointIndex][curAnimation].push_back(curMat);
				curFrame++;
			}
		}
	}

	// // verify lengths are conisitent
	// int numVerts = rModel->iModel.positions.size();
	// if(rModel->iModel.normals.size() != numVerts) {
	// 	printf("pos and normals size dont match: %d, %d\n", numVerts, rModel->iModel.normals.size());
	// 	return false;
	// }
	// if(rModel->iModel.uvCoords.size() != numVerts) {
	// 	printf("pos and uvCoords size dont match: %d, %d\n", numVerts, rModel->iModel.uvCoords.size());
	// 	return false;
	// }
	// if(rModel->iModel.jointIndices.size() != numVerts) {
	// 	printf("pos and jointIndices size dont match: %d, %d\n", numVerts, rModel->iModel.jointIndices.size());
	// 	return false;
	// }
	// if(rModel->iModel.jointWeights.size() != numVerts) {
	// 	printf("pos and jointWeights size dont match: %d, %d\n", numVerts, rModel->iModel.jointWeights.size());
	// 	return false;
	// }

	// int numJoints = rModel->invJointTransforms.size();
	// if(rModel->jointParents.size() != numJoints) {
	// 	printf("invJointTransforms and jointParents size dont match: %d, %d\n", numJoints, rModel->jointParents.size());
	// 	return false;
	// }

	// int numUniquePositions = oModel.positions.size();
	// if(rModel->jointIndices.size() != numUniquePositions) {
	// 	printf("unique pos and jointIndices size dont match: %d, %d\n", numUniquePositions, rModel->jointIndices.size());
	// 	return false;
	// }
	// if(rModel->jointWeights.size() != numUniquePositions) {
	// 	printf("unique pos and jointWeights size dont match: %d, %d\n", numUniquePositions, rModel->jointWeights.size());
	// 	return false;
	// }

	freeNode(root);

	return true;
}
