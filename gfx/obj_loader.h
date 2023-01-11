#pragma once
#include "model.h"
#include <fstream>
#include <string>
#include <vector>
#include <map>
using namespace std;

vec3 ParseVec3(const string& line) {
	u32 len = line.size();
	u32 startIndex = 0;
	u32 endIndex = 0;
	u32 i = 0;
	vec3 retVec = vec3(0.0f, 0.0f, 0.0f);

	while(startIndex < len) {
		bool isNumber = line[startIndex] >= '0' && line[startIndex] <= '9';
		if(!(isNumber || line[startIndex] == '-')) {
			startIndex++;
			continue;
		}

		// now we have a number on startIndex
		endIndex = startIndex;
		while(endIndex < len) {
			if(line[endIndex] != ' ' && line[endIndex] != '\t' && line[endIndex] != '\r' && line[endIndex] != '\n') {
				endIndex++;
				continue;
			}

			// now we have a space or tab on endIndex
			// so parse the number between the recorded indexes
			retVec[i] = atof(line.substr(startIndex, endIndex - startIndex).c_str());
			i++;
			startIndex = endIndex;
			break;
		}
		if(i == 3) {
			break;
		}
	}

	return retVec;
}

Face ParseFace(const string& line) {
	// add to indices
	u32 len = line.size();
	u32 startIndex = 0;
	u32 endIndex = 0;
	u32 curAttrib = 0;
	Face face;

	while(startIndex < len) {
		bool isNumber = line[startIndex] >= '0' && line[startIndex] <= '9';
		if(!(isNumber || line[startIndex] == '-')) {
			startIndex++;
			continue;
		}

		// now we have a number on startIndex
		endIndex = startIndex;
		while(endIndex <= len) {
			if(endIndex != len && line[endIndex] != '/' && line[endIndex] != ' ' && line[endIndex] != '\t' && line[endIndex] != '\r' && line[endIndex] != '\n') {
				endIndex++;
				continue;
			}

			// sub 1 because obj file indices start at 1
			IndexType index = atoi(line.substr(startIndex, endIndex - startIndex).c_str()) - 1;
			startIndex = endIndex;

			// now we at the end of the number
			if(line[endIndex] == '/') {
				if(curAttrib == 0) {
					// store vertex pos
					face.posIndices.push_back(index);
				}
				else if(curAttrib == 1) {
					// store vertex uvCoord
					face.uvCoordIndices.push_back(index);
				}
				else {
					// store vertex normal
					face.normalIndices.push_back(index);
				}
				// go to next vertex attribute
				curAttrib++;
				// handle the '//' between vertex pos and normals when no uvCoords
				if(line[endIndex + 1] == '/') {
					curAttrib++;
				}
			}
			else {
				// done with a face element
				if(curAttrib == 0) {
					// store vertex pos
					face.posIndices.push_back(index);
				}
				else if(curAttrib == 1) {
					// store vertex uvCoord
					face.uvCoordIndices.push_back(index);
				}
				else {
					// store vertex normal
					face.normalIndices.push_back(index);
				}
				curAttrib = 0;
			}
			break;
		}
	}
	return face;
}

bool LoadOBJ(RiggedModel* rModel, const char* file_path) {
	// IndexedModel* iModel = &rModel->iModel;
	std::ifstream fileStream(file_path, std::ios::in);
	if(!fileStream.is_open()) {
		printf("Failed to open %s.\n", file_path);
		return false;
	}
	OBJModel obj_model;
	std::string line = "";
	while(getline(fileStream, line)) {
		line += "\n";
		// TODO: trim leading whitespace (or iterate over to the first non-whitespace)
		if(line[0] == 'v' && (line[1] == ' ' || line[1] == '\t')) {
			// add to positions
			obj_model.positions.push_back(ParseVec3(line));
		}
		else if(line[0] == 'v' && line[1] == 'n') {
			// add to normals
			obj_model.normals.push_back(normalize(ParseVec3(line)));
		}
		else if(line[0] == 'v' && line[1] == 't') {
			// add to uvCoords
			vec3 tempVec = ParseVec3(line);
			obj_model.uvCoords.push_back(vec2(tempVec.x, tempVec.y));
		}
		else if(line[0] == 'f') {
			obj_model.faces.push_back(ParseFace(line));
		}
		// TODO: 'l' for polyline
	}

	fileStream.close();

	fillIndexedModel(rModel, obj_model);
	return true;
}

// int main() {
// 	Memory mem;
// 	InitMemory(mem, 1 MB);

// 	// Face face = ParseFace("f 3/3/1 2/2/1 4/4/1");
// 	// printf("posIndices\n");
// 	// for(u32 i = 0; i < face.posIndices.size(); i++) {
// 	// 	printf("%u\n", face.posIndices[i]);
// 	// }
// 	// printf("uvCoordIndices\n");
// 	// for(u32 i = 0; i < face.uvCoordIndices.size(); i++) {
// 	// 	printf("%u\n", face.uvCoordIndices[i]);
// 	// }
// 	// printf("normalIndices\n");
// 	// for(u32 i = 0; i < face.normalIndices.size(); i++) {
// 	// 	printf("%u\n", face.normalIndices[i]);
// 	// }

// 	IndexedModel* iModel = LoadOBJ(mem, "../../meshes/monkey3.obj");
// 	printf("positions\n");
// 	for(u32 i = 0; i < iModel->positions.size(); i++) {
// 		if(i % 4 == 0 && i != 0) {
// 			printf("\n");
// 		}
// 		printf("%f, %f, %f\n", iModel->positions[i].x, iModel->positions[i].y, iModel->positions[i].z);
// 	}
// 	printf("uvCoords\n");
// 	for(u32 i = 0; i < iModel->uvCoords.size(); i++) {
// 		if(i % 4 == 0 && i != 0) {
// 			printf("\n");
// 		}
// 		printf("%f, %f\n", iModel->uvCoords[i].x, iModel->uvCoords[i].y);
// 	}
// 	printf("normals\n");
// 	for(u32 i = 0; i < iModel->normals.size(); i++) {
// 		if(i % 4 == 0 && i != 0) {
// 			printf("\n");
// 		}
// 		printf("%f, %f, %f\n", iModel->normals[i].x, iModel->normals[i].y, iModel->normals[i].z);
// 	}
// 	printf("indices\n");
// 	for(u32 i = 0; i < iModel->indices.size(); i++) {
// 		if(i % 3 == 0 && i != 0) {
// 			printf("\n");
// 		}
// 		printf("%u\n", iModel->indices[i]);
// 	}

// 	return 0;
// }
