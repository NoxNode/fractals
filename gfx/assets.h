#pragma once
#include "material.h"
#include "obj_loader.h"
#include "dae_loader.h"
#include "gl_buffers.h"
#include "../core/fileio.h"

#define MAX_MODELS 16
#define MAX_TEXTURES 16
#define MAX_MATERIALS 16

struct Assets {
	u32 nModels;
	Model models[MAX_MODELS];
	u32 nTextures;
	Texture textures[MAX_TEXTURES];
	u32 nMaterials;
	Material materials[MAX_MATERIALS];
};

void InitAssets(Assets& assets) {
    assets.nModels = 0;
    assets.nTextures = 0;
    assets.nMaterials = 0;
}

void DeinitAssets(Assets& assets) {
	for(u32 i = 0; i < assets.nTextures; i++) {
		DeinitTexture(assets.textures[i].texture);
	}
}

void LoadModelAsset(Assets& assets, const char* modelPath, VBO& vbo, IBO& ibo, JointBuffers& jointBuffers, u32* animationStartFrameIndices = NULL, s32 numAnimations = 1) {
	if(assets.nModels >= MAX_MODELS) {
		printf("exceeded max models\n");
		return;
	}
	RiggedModel riggedModel;
	bool loaded = false;
	const char* fileExtension = getFileExtension(modelPath);
	if(strcmp(fileExtension, ".obj") == 0)
		loaded = LoadOBJ(&riggedModel, modelPath);
	else if(strcmp(fileExtension, ".dae") == 0)
		loaded = LoadDAE(&riggedModel, modelPath, animationStartFrameIndices, numAnimations);
	if(!loaded) {
		printf("failed to load model file: %s\n", modelPath);
		assets.nModels++; // still go to next model so indices match up
		return;
	}
	LoadModelToBuffers(assets.models[assets.nModels], &riggedModel, vbo, ibo, jointBuffers);
	assets.nModels++;
}

void LoadTextureAsset(Assets& assets, const char* texturePath) {
	if(assets.nTextures >= MAX_TEXTURES) {
		printf("exceeded max textures\n");
		return;
	}
	InitTexture(assets.textures[assets.nTextures], texturePath);
	assets.nTextures++;
}

void CreateMaterialAsset(Assets& assets, Texture* texture = NULL, Texture* normalMap = NULL, Texture* dispMap = NULL,
v3 color = v3(1,1,1), r32 shininess = 100.0f, r32 dispMapScale = 0.04f, r32 dispMapOffset = 0.0f, r32 friction = 1.0f) {
	if(assets.nMaterials > MAX_MATERIALS) {
			printf("exceeded max materials\n");
			return;
	}
	InitMaterial(assets.materials[assets.nMaterials], texture, normalMap, dispMap, color, shininess, dispMapScale, dispMapOffset, friction);
	assets.nMaterials++;
}

#ifdef _WIN32
	#define EXTERNAL_MODELS_FOLDER "C:\\Users\\Noxide\\Downloads\\game_assets\\"
#else
	#define EXTERNAL_MODELS_FOLDER "/Users/wyatt/Downloads/"
#endif

void InitDefaultAssets(Assets& assets, VBO& vbo, IBO& ibo, JointBuffers& jointBuffers) {
	// init models
	LoadModelAsset(assets, "models/square.obj", vbo, ibo, jointBuffers); // 0
	LoadModelAsset(assets, "models/cube.obj", vbo, ibo, jointBuffers); // 1
	LoadModelAsset(assets, "models/circle.obj", vbo, ibo, jointBuffers); // 2
	u32 monkAnimationStartFrameIndices[11] = {
		0, 41, 60, 91, 119, 134, 155, 179, 209, 239, 314
	};
	//LoadModelAsset(assets, (string(EXTERNAL_MODELS_FOLDER) + "Monk2.dae").c_str(), vbo, ibo, jointBuffers, monkAnimationStartFrameIndices, 11); // 3

	// LoadModelAsset(assets, "models/ike_sword.obj", vbo, ibo, jointBuffers); // 4
	// LoadModelAsset(assets, (string(EXTERNAL_MODELS_FOLDER) + "millenium-falcon.obj").c_str(), vbo, ibo, jointBuffers); // 5
	// LoadModelAsset(assets, (string(EXTERNAL_MODELS_FOLDER) + "star-wars-vader-tie-fighter.obj").c_str(), vbo, ibo, jointBuffers); // 6

	// init textures
	LoadTextureAsset(assets, "textures/bark.jpg"); // 0
	LoadTextureAsset(assets, "textures/bark_normalMap.jpg"); // 1
	LoadTextureAsset(assets, "textures/pebbles.jpg"); // 2
	LoadTextureAsset(assets, "textures/pebbles_normalMap.jpg"); // 3
	LoadTextureAsset(assets, "textures/ike.jpg"); // 4
	LoadTextureAsset(assets, "textures/bricks.jpg"); // 5
	LoadTextureAsset(assets, "textures/bricks_normal.jpg"); // 6
	LoadTextureAsset(assets, "textures/bricks_disp.png"); // 7
	LoadTextureAsset(assets, "textures/bricks2.jpg"); // 8
	LoadTextureAsset(assets, "textures/bricks2_normal.jpg"); // 9
	LoadTextureAsset(assets, "textures/bricks2_disp.jpg"); // 10
	//LoadTextureAsset(assets, (string(EXTERNAL_MODELS_FOLDER) + "Monk_Texture.png").c_str()); // 11

	// LoadTextureAsset(assets, (string(EXTERNAL_MODELS_FOLDER) + "falcon.jpg").c_str()"C:\\Users\\Noxide\\Downloads\\falcon.jpg"); // 11

	// init materials
	CreateMaterialAsset(assets, NULL, NULL); // 0
	CreateMaterialAsset(assets, &assets.textures[2], &assets.textures[3]); // 1
	CreateMaterialAsset(assets, NULL, NULL, NULL, v3(0.4f)); // 2
	CreateMaterialAsset(assets, &assets.textures[4]); // 3
	CreateMaterialAsset(assets, &assets.textures[5], &assets.textures[6], &assets.textures[7],v3(1,1,1),100.0f,0.03f,-0.5f); // 4
	CreateMaterialAsset(assets, &assets.textures[8], &assets.textures[9], &assets.textures[10],v3(1,1,1),100.0f,0.04f,-1.0f); // 5
	CreateMaterialAsset(assets, NULL, NULL, NULL, v3(1.0f, 0.0f, 0.0f)); // 6
	CreateMaterialAsset(assets, NULL, NULL, NULL, v3(1.0f, 1.0f, 1.0f)); // 7
	CreateMaterialAsset(assets, &assets.textures[11]); // 8
}
