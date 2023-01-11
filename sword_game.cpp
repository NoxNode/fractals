#include "core/udp_socket.h"
#include "core/rdll.h"

#ifndef COMPILE_SERVER
	#include <GL/glew.h>
	#include <SFML/Window.hpp>
	#include <SFML/OpenGL.hpp>
	#include "core/window.h"
	#include "gfx/shader.h"
	#include "gfx/camera.h"
	#include "gfx/light.h"
	#include "gfx/default_shader.h"
	#include "gfx/obj_loader.h"
	#include "gfx/dae_loader.h"
	#include "gfx/material.h"
#else
	// since the server doesn't need these definitions, just declare them so we can use render_obj.h
	struct Model;
	struct Material;
#endif
#include "gfx/render_obj.h"
#include "core/memory.h"
#include "game/game_input.h"
#include "physics/collision.h"

/*
TODOs

star wars shooter in fractal world

skeletal animation
	multiple skeletons
		find node in scene with instance controller
		instance controller's skin has source that links to geometries
	multiple geometries (what do?)

	load animations
	play animations
		to mesh animations maybe just have each animation add their pos, rot, and scale for each joint
		and at the end just divide each joint's variables by the number of animations currently being meshed together to get the average keyframe
particles
networking

deferred shading
text rendering
multiple lights
dynamic materials / decals / painting
scenes
twitch integration
	battle royale mini games like fall guys but with viewers of streams and the streamer has a ton of admin options before and during the game
	maybe can bet bits or channel points or even crypto on winners of the viewer games
	can also make matches with just streamers and viewers can donate in order to throw some obstacle in their way or something

load models and edit scenes on the fly
users can upload models and scenes and only approved ones get into the game
different combat systems to fight in based on the following games
	For Honor / Dark Souls
	Smash Bros
	League
	TFT
	Domugioh
	Star Craft
procedurally generated worlds with dwarf fortress like detail
	aka No Man's Sky meets Dwarf Fortress for the world exploration aspect

TODO: bump maps - see why I need to inverse y direction and if that affects the normal map



Graphics Engine
	Texture
	Material
		textures
		properties
	Light
	Camera
	Vertex
		pos, uvCoords,
		normal, tangent
		jointIndices, jointWeights
	Model
		verticesIndex, numVertices
		indicesIndex, numIndices
		invJointTransformsIndex, numInvJointTransforms
	RenderObj
		pos, rot, scale, pivot
		modelMatrix, normalMatrix
		model, material
		curJointTransforms
		rootJoint : jointNode

	JointNode
		DynamicJoint
		children
	JointTransform
		pos
		rot : quaternion
	DynamicJoint
		JointTransform
		dynamics
	KeyFrame
		jointTransforms[MAX_JOINTS_PER_MODEL]
		timestamp
	Animation
		keyFrames[MAX_STATIC_KEY_FRAMES]
		numKeyFrames
	
Physics Engine
	RigidBody (TODO: SoftBody - which manipulates the mesh itself in its reaction)
		mesh
		pos
		vel
		drag (air friction / global friction)
		impulseAccumulator
		forceAccumulator
		restitution (bounciness)
		density
		mass, inv_mass
	UpdateBody(body)
	DetectCollision(body1, body2) : CollisionInfo
	ReactToCollision(body1, body2, collisionInfo)
	*/

// graphics defines
#define MAX_MODELS 16
#define MAX_TEXTURES 16
#define MAX_MATERIALS 16
#define MAX_RENDER_OBJS 32
#define MAX_LIGHTS 5
#define MAX_VERTICES 500000
#define MAX_INDICES  2000000
#define MAX_JOINTS 100

// gameplay defines
#define PLAYER_SCALE 0.4f
#define SWORD_SCALE 0.2f
#define TICK_RATE 1000.0 / 60.0
#define GRAVITY -0.04f
#define FRICTION 0.98f
#define PLAYER_SPEED 0.3f
#define SWORD_LERP_SPEED 0.3f
#define SWORD_SLERP_SPEED 0.3f
#define SWORD_COLLISION_INTERVAL 10

// multiplayer defines
#define SERVER_TICK_RATE 1000.0 / 60.0
#define SERVER_IP "18.144.67.72"
// #define SERVER_IP "localhost"
#define SERVER_PORT 9001
#ifndef CLIENT_PORT
	#define CLIENT_PORT 9000
#endif
#define MAX_PACKET_SIZE 1 KB
#define MAX_CLIENTS 2

#define INPUT_DELAY 50
#define UPDATE_DELAY 30
#define INPUT_BUFFER_SIZE 20
#define SNAPSHOT_BUFFER_SIZE 20

#define DISABLE_TICK_COUNTER

bool playerHasHitOther(v3 lastPlayerPos, v3 playerPos, v3 lastSwordDir, v3 swordDir, v3 otherPlayerPos) {
	const r32 rayLen = 14.3f * SWORD_SCALE + 0.6f;

	const r32 halfPlayerScale = PLAYER_SCALE / 2.0f;
	v3 max = otherPlayerPos + v3(halfPlayerScale, halfPlayerScale, halfPlayerScale);
	v3 min = otherPlayerPos - v3(halfPlayerScale, halfPlayerScale, halfPlayerScale);
	v3 rayOrig = playerPos;
	v3 rayDir = swordDir;
	if(intersect(max, min, rayOrig, rayDir, rayLen))
		return true;
	r32 ratioInc = 1.0 / (r32)SWORD_COLLISION_INTERVAL;
	r32 ratio = ratioInc;
	for(int i = 1; i <= SWORD_COLLISION_INTERVAL; i++) {
		rayOrig = lastPlayerPos * ratio + playerPos * (1.0f - ratio);
		rayDir = lastSwordDir * ratio + swordDir * (1.0f - ratio);
		if(intersect(max, min, rayOrig, rayDir, rayLen))
			return true;
		ratio += ratioInc;
	}
	return false;
}

void InitPlayerObj(RenderObj& playerObj, Model* model, Material* material) {
	InitRenderObj(playerObj, model, material, v3(0.0f, 20.0f, 5.0f), v3(0,0,0), v3(PLAYER_SCALE, PLAYER_SCALE, PLAYER_SCALE));
}

void InitSwordObj(RenderObj& swordObj, Model* model, Material* material) {
	InitRenderObj(swordObj, model, material, v3(0.0f, 2.0f, 0.6f), v3(0,0,0), v3(SWORD_SCALE, SWORD_SCALE, SWORD_SCALE), v3(0.0f, 0.0f, 0.6f));
}

void UpdateSwordMatrices(RenderObj& swordRenderObj, vec3 lookAtPos) {
	// look at vec can't == up or -up or we get NaNs
	if(lookAtPos == vec3(0.0f, 1.0f, 0.0f) || lookAtPos == vec3(-0.0f, -1.0f, 0.0f)) {
		lookAtPos.z += -0.0001f;
	}
	mat4 lookAtCalc = lookAt(
		vec3(0.0f, 0.0f, 0.0f),
		lookAtPos,
		vec3(0.0f, 1.0f, 0.0f));
	swordRenderObj.transform.matrix = translate(swordRenderObj.transform.pos);
	swordRenderObj.transform.matrix *= inverse(lookAtCalc);
	swordRenderObj.transform.matrix *= translate(-swordRenderObj.transform.pivot);
	swordRenderObj.transform.matrix *= scale(swordRenderObj.transform.scale);
	swordRenderObj.normalMatrix = transpose(inverse(swordRenderObj.transform.matrix));
}

vec3 Lerp(vec3 from, vec3 to, r32 howMuch) {
	return from * (1.0f - howMuch) + to * howMuch;
}

vec3 CLerp(vec3 from, vec3 to, r32 howMuch, r32 angle) {
    const r32 DOT_THRESHOLD = 0.8f;
	r32 dotProd = dot(from, to);

	// if from and to are really close, just lerp
	if(dotProd > DOT_THRESHOLD)
		return Lerp(from, to, howMuch);

	if(from == -to) {
		from.z += -0.0001f;
	}

	vec3 rotAxis = cross(from, to);

	return rotate(from, angle, rotAxis);
}

void roundVec3(vec3& v) {
	v.x = roundf(v.x * 1000) / 1000;
	v.y = roundf(v.y * 1000) / 1000;
	v.z = roundf(v.z * 1000) / 1000;
}

void UpdateSword(NetInput& nInput, vec3& lastLookAtPos, RenderObj& swordRenderObj, vec3& playerPos) {
	// lerp to the target look at pos
	vec3 lookAtPos = CLerp(lastLookAtPos, nInput.swordTargetLookAtPos, SWORD_LERP_SPEED, SWORD_SLERP_SPEED);

	// round so we can handle the case that lookAtPos is == up or -up and we can prevent NaNs
	roundVec3(lookAtPos);

	lastLookAtPos = lookAtPos;

	// update sword pos and rot
	swordRenderObj.transform.pos = playerPos;
	UpdateSwordMatrices(swordRenderObj, lookAtPos);
}

void UpdatePlayer(RenderObj& player, vec3& charVel, RenderObj& sword, vec3& lastSwordLookAtPos, vec3& lastSwordTipPos) {
	// get sword tip position
	vec3 swordTipPos = vec3(0.0f, 0.0f, -14.3f);
	swordTipPos = vec3(sword.transform.matrix * vec4(swordTipPos, 1.0f));

	vec3 swordTipPosDiff = swordTipPos - lastSwordTipPos;
	if(swordTipPos.y < 0) {
	    // apply sword-to-ground force
	    vec3 downFromLastSwordTip = vec3(lastSwordTipPos.x, 0, lastSwordTipPos.z) - lastSwordTipPos;
	    r32 downAmt = dot(downFromLastSwordTip, swordTipPosDiff);
	    vec3 toGround = swordTipPosDiff * downAmt;
	    vec3 howMuchInGround = swordTipPosDiff - toGround;
	    charVel += -howMuchInGround * PLAYER_SPEED;

	    // correct position
	    player.transform.pos.y -= howMuchInGround.y;
		swordTipPos.y = 0;
	}
	lastSwordTipPos = swordTipPos;

	// apply velocity
	player.transform.pos += charVel;

	// apply gravity if not grounded
	if(player.transform.pos.y < PLAYER_SCALE / 2.0f) {
	    player.transform.pos.y = PLAYER_SCALE / 2.0f;
	}
	else {
	    charVel += vec3(0.0f, GRAVITY, 0.0f);
	}

	// apply friction
	charVel *= FRICTION;

	// update player pos
	UpdateMatrices(player);
	
	// update sword pos
	sword.transform.pos = player.transform.pos;
	UpdateSwordMatrices(sword, lastSwordLookAtPos);
}

#include <thread>
void LimitTickRate(u64 millisPerTick, u64& lastTickTime) {
	u64 curTickTime = MillisSinceEpoch();
	u64 millisSinceLastTick = curTickTime - lastTickTime;
	if(millisSinceLastTick < millisPerTick) {
		std::this_thread::sleep_for(std::chrono::milliseconds(millisPerTick - millisSinceLastTick));
		curTickTime = MillisSinceEpoch();
		millisSinceLastTick = curTickTime - lastTickTime;
	}
	// extraMillisFromLastTick = millisSinceLastTick - millisPerTick;
	// printf("extraMillisFromLastTick: %f\n", extraMillisFromLastTick);
	lastTickTime = curTickTime;
}

struct Game {
	Window* window;

	//// shaders
	DefaultShader shader;
	// GLuint simpleShader;
	GLuint shadowShader;
	GLuint fractalShader;

	FBO fractalMap;

	//// buffers
	GLuint vbo, ibo;
	u32 verticesOffset;
	Vertex vertices[MAX_VERTICES];
	u32 indicesOffset;
	IndexType indices[MAX_INDICES];

	// this is the join info we use to calculate the joint info we need to send to the shader
	u32 jointsOffset;
	mat4 invJointTransforms[MAX_JOINTS];
	IndexType jointParents[MAX_JOINTS];
	mat4 boneSpaceJointTransforms[MAX_JOINTS];

	// this is the joint info we sent to the shader
	mat4 jointTransforms[MAX_JOINTS_PER_MODEL];

	//// assets
	u32 nModels;
	Model models[MAX_MODELS];
	u32 nTextures;
	Texture textures[MAX_TEXTURES];
	u32 nMaterials;
	Material materials[MAX_MATERIALS];

	//// game objects
	Camera camera;
	DirLight dirLight;
	u32 nRenderObjs;
	RenderObj renderObjs[MAX_RENDER_OBJS];
	u32 nLights;
	Light lights[MAX_LIGHTS];

	//// temp code
	u64 lastTickTime;
	double extraMillisFromLastTick;

	GameInput gInput;
	vec3 lastSwordTargetLookAtPos;
	vec3 lastSwordTipPos;
	vec3 charVel;

	r32 cameraMoveSpeed;
	r32 cameraRotateSpeed;
	bool usingGamepad;

#ifndef DISABLE_TICK_COUNTER
	u64 ticks;
	u64 lastTickPrint;
#endif

	bool debugDrawModel;
	int debugCubeIndex;
	r32 debugTime;
	u8 reserved[1 KB];
};

bool pointInTraingle(r32 x, r32 y, v2 p1, v2 p2, v2 p3) {
    v2 point = v2(x, y);
    v3 p1_p = v3(point - p1, 0.0f);
    v3 p2_p = v3(point - p2, 0.0f);

    v3 p1_2 = v3(p2 - p1, 0.0f);
    v3 p1_3 = v3(p3 - p1, 0.0f);

    v3 up = v3(0, 0, 1);

    v3 n1_2 = cross(p1_2, up);
    v3 n1_3 = cross(up, p1_3);

    r32 dir1_2 = dot(p1_p, n1_2);
    r32 dir1_3 = dot(p1_p, n1_3);

    if(dir1_2 > 0 || dir1_3 > 0)
        return false;

    // now we know the point is inside p1_2 and p1_3
    // just have to check p2_3 now
    v3 p2_3 = v3(p3 - p2, 0.0f);

    v3 n2_3 = cross(p2_3, up);

    r32 dir2_3 = dot(p2_p, n2_3);

    if(dir2_3 > 0)
        return false;

    // the point is inside the triangle

    return true;
}

bool collisionFractalSphere(Game* game, RenderObj& obj1) {
	Model* model1 = obj1.model;

	int indicesOffset1 = model1->indicesOffset;
	int verticesOffset1 = model1->verticesOffset;
	
	r32 closestDist = 10000.0;
	int closestIndex = -1;
	for(int i = 0; i < model1->numVertices; i++) {
		vec3 vertex = game->vertices[game->indices[indicesOffset1 + i]].pos;
		r32 dist = length(vertex - vec3(0.0, 1.0, 0.0)) - 0.5;
		if(dist < closestDist) {
			closestDist = dist;
			closestIndex = i;
		}
		if(dist < 0) return true;
	}
	// have closest point
	// loop through all triangles this point belongs to
	// find the closest point on those triangles to the geometry
	// average those points
	// then just 3d march in all directions towards the geometry

	// for fractal to fractal collision, just find any point on the first geometry, then 3d march within that first geomoetry towards the second
		// only works for sdfs that describe one contiguous object

	return false;
}

bool collision3d(Game* game, RenderObj& obj1, RenderObj& obj2) {
	Model* model1 = obj1.model;
	Model* model2 = obj2.model;

	// TODO: control points - call this a second time with the guy model as the first object
		// draw lines from the nearest control point to the vertex and use that in the below calculations
		// make another control point for each shoulder
// 	-5.399997, -0.400000, 3.499999
// 3.999999, 0.500000, 3.699999
// -1.200000, 0.600000, 39.499969
// 23.300053, -0.500000, 35.100037
// -22.800051, -0.600000, 35.300034
// -0.500000, 2.500000, 75.399422

	int indicesOffset1 = model1->indicesOffset;
	int indicesOffset2 = model2->indicesOffset;

	int verticesOffset1 = model1->verticesOffset;
	int verticesOffset2 = model2->verticesOffset;

	// we want the center of model1
	vec3 center = vec3(0.0f);
	for(int i = 0; i < model1->numVertices; i++) {
		vec3 vertex = vec3(game->vertices[verticesOffset1 + i].pos.x,
			game->vertices[verticesOffset1 + i].pos.y,
			game->vertices[verticesOffset1 + i].pos.z);
		// transform vertex based on the model matrix
		// vertex = vec3(obj1.transform.matrix * vec4(vertex, 1.0f));
		center += vertex;
	}
	center /= model1->numVertices;

	// printf("center: %f, %f, %f\n", center.x, center.y, center.z);

	// we want lines to each of the vertices of model1 from the center
	for(int i = 0; i < model1->numIndices; i++) {
		vec3 vertex = game->vertices[game->indices[indicesOffset1 + i]].pos;
		// transform vertex based on the model matrix
		// vertex = vec3(obj1.transform.matrix * vec4(vertex, 1.0f));

		vec3 vectorToVertexFromCenter = vertex - center;
		// we want to iterate over each of those lines and each of the triangles of model2
		int numTriangles2 = model2->numIndices / 3;
		for(int j = 0; j < model2->numIndices; j += 3) {
			// for each of these pairs, do a line to triangle test
			vec3 vertex1 = game->vertices[game->indices[indicesOffset2 + j]].pos;
			// transform vertex based on the model matrix
			// vertex1 = vec3(obj2.transform.matrix * vec4(vertex1, 1.0f));

			vec3 vertex2 = game->vertices[game->indices[indicesOffset2 + j + 1]].pos;
			// transform vertex based on the model matrix
			// vertex2 = vec3(obj2.transform.matrix * vec4(vertex2, 1.0f));

			vec3 vertex3 = game->vertices[game->indices[indicesOffset2 + j + 2]].pos;
			// transform vertex based on the model matrix
			// vertex3 = vec3(obj2.transform.matrix * vec4(vertex3, 1.0f));

			vec3 normal = cross(vertex2 - vertex1, vertex3 - vertex1);

			// printf("vertex1: %f, %f, %f\n", vertex1.x, vertex1.y, vertex1.z);
			// printf("vertex2: %f, %f, %f\n", vertex2.x, vertex2.y, vertex2.z);
			// printf("vertex3: %f, %f, %f\n", vertex3.x, vertex3.y, vertex3.z);

			if(dot(vectorToVertexFromCenter,normal) != 0) {
				// TODD HOWARD
				vec3 diff = center - vertex1;
				r32 prod1 = dot(diff, normal);
				r32 prod2 = dot(vectorToVertexFromCenter, normal);
				r32 prod3 = prod1 / prod2;

				vec3 intersectionPoint = center - vectorToVertexFromCenter * prod3;

				if(length(intersectionPoint - center) > length(vectorToVertexFromCenter)) {
					continue;
				}
				
				// printf("intersectionPoint: %f, %f, %f\n", intersectionPoint.x, intersectionPoint.y, intersectionPoint.z);

				// point in triangle
				v3 p1_p = intersectionPoint - vertex1;
				v3 p2_p = intersectionPoint - vertex2;

				vec3 vec21 = vertex2 - vertex1;
				vec3 vec31 = vertex3 - vertex1;
				vec3 up = cross(vec21, vec31);
				vec3 normal21 = cross(vec21, up);
				if(dot(normal21, p1_p) > 0)
					continue;
				vec3 normal31 = cross(up, vec31);
				if(dot(normal31, p1_p) > 0)
					continue;
				vec3 vec23 = vertex3 - vertex2;
				vec3 normal23 = cross(vec23, up);
				if(dot(normal23, p2_p) > 0)
					continue;

				// if any collide, the objects are colliding
				return true;
			}
		}
	}
	return false;
}

void InitAttributes(Game* game) {
	GLint posLoc = glGetAttribLocation(game->shader.program, "a_position");
	GLint uvCoordsLoc = glGetAttribLocation(game->shader.program, "a_uvCoords");
	GLint noramlLoc = glGetAttribLocation(game->shader.program, "a_normal");
	GLint tangentLoc = glGetAttribLocation(game->shader.program, "a_tangent");
	// GLint bitangentLoc = glGetAttribLocation(game->shader.program, "a_bitangent");
	GLint jointIndicesLoc = glGetAttribLocation(game->shader.program, "a_jointIndices");
	GLint jointWeightsLoc = glGetAttribLocation(game->shader.program, "a_jointWeights");

	glVertexAttribPointer(posLoc, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*) offsetof(Vertex, pos));
	glVertexAttribPointer(uvCoordsLoc, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*) offsetof(Vertex, uvCoords));
	glVertexAttribPointer(noramlLoc, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*) offsetof(Vertex, normal));
	glVertexAttribPointer(tangentLoc, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*) offsetof(Vertex, tangent));
	// glVertexAttribPointer(bitangentLoc, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*) offsetof(Vertex, bitangent));
	// TODO: may need to change to glVertexAttribIPointer
	glVertexAttribPointer(jointIndicesLoc, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*) offsetof(Vertex, jointIndices));
	glVertexAttribPointer(jointWeightsLoc, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*) offsetof(Vertex, jointWeights));

	glEnableVertexAttribArray(posLoc);
	glEnableVertexAttribArray(uvCoordsLoc);
	glEnableVertexAttribArray(noramlLoc);
	glEnableVertexAttribArray(tangentLoc);
	// glEnableVertexAttribArray(bitangentLoc);
	glEnableVertexAttribArray(jointIndicesLoc);
	glEnableVertexAttribArray(jointWeightsLoc);
}

void InitBuffers(Game* game) {
	glGenBuffers(1, &game->vbo);
	glBindBuffer(GL_ARRAY_BUFFER, game->vbo);

	InitAttributes(game);

	glGenBuffers(1, &game->ibo);
}

void FillBuffers(Game* game) {
	glBindBuffer(GL_ARRAY_BUFFER, game->vbo);
	glBufferData(GL_ARRAY_BUFFER, game->verticesOffset * sizeof(Vertex), game->vertices, GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, game->ibo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, game->indicesOffset * sizeof(IndexType), game->indices, GL_STATIC_DRAW);
}

void DeinitBuffers(Game* game) {
	glDeleteBuffers(1, &game->vbo);
	glDeleteBuffers(1, &game->ibo);
}

void InitGameModel(Game* game, const char* modelPath) {
	if(game->nModels >= MAX_MODELS) {
		printf("exceeded max models\n");
		return;
	}
	RiggedModel riggedModel;
	bool loaded = false;
	const char* fileExtension = getFileExtension(modelPath);
	if(strcmp(fileExtension, ".obj") == 0)
		loaded = LoadOBJ(&riggedModel, modelPath);
	else if(strcmp(fileExtension, ".dae") == 0)
		loaded = LoadDAE(&riggedModel, modelPath);
	if(!loaded) {
		printf("failed to load model file: %s\n", modelPath);
		game->nModels++;
		return;
	}
	LoadModelToBuffers(game->models[game->nModels], &riggedModel, game->vertices, game->verticesOffset, MAX_VERTICES, game->indices, game->indicesOffset, MAX_INDICES, game->jointParents, game->invJointTransforms, game->jointsOffset, MAX_JOINTS, game->boneSpaceJointTransforms);
	game->nModels++;
}

void InitGameTexture(Game* game, const char* texturePath) {
	if(game->nTextures >= MAX_TEXTURES) {
		printf("exceeded max textures\n");
		return;
	}
	InitTexture(game->textures[game->nTextures], texturePath);
	game->nTextures++;
}

void InitGameMaterial(Game* game, Texture* texture = NULL, Texture* normalMap = NULL, Texture* dispMap = NULL,
v3 color = v3(1,1,1), r32 shininess = 100.0f, r32 dispMapScale = 0.04f, r32 dispMapOffset = 0.0f, r32 friction = 1.0f) {
	if(game->nMaterials > MAX_MATERIALS) {
			printf("exceeded max materials\n");
			return;
	}
	InitMaterial(game->materials[game->nMaterials], texture, normalMap, dispMap, color, shininess, dispMapScale, dispMapOffset, friction);
	game->nMaterials++;
}

void BindMaterial(Game* game, Material& material) {
	if(material.texture != NULL) {
		BindTexture(game->shader.u_texture, material.texture->texture, 0);
	}
	if(material.normalMap != NULL) {
		BindTexture(game->shader.u_normalMap, material.normalMap->texture, 1);
	}
	if(material.dispMap != NULL) {
		BindTexture(game->shader.u_dispMap, material.dispMap->texture, 2);
	}
	glUniform3fv(game->shader.u_color, 1, &material.color[0]);
	glUniform1f(game->shader.u_dispMapScale, material.dispMapScale);
	glUniform1f(game->shader.u_dispMapBias, material.dispMapBias);
}

void InitGameRenderObj(Memory& mem, Game* game, Model* model, Material* material, v3 pos = v3(0,0,0), v3 rot = v3(0,0,0), v3 scale = v3(1,1,1), v3 pivot = v3(0,0,0)) {
	if(game->nRenderObjs >= MAX_RENDER_OBJS) {
		printf("exceeded max render objs\n");
		return;
	}
	InitRenderObj(game->renderObjs[game->nRenderObjs], model, material, pos, rot, scale, pivot);
	// allocate joint states if rigged model is used
	if(model->numJoints > 0) {
		game->renderObjs[game->nRenderObjs].jointStates = (JointState*)Alloc(mem, sizeof(JointState) * MAX_JOINTS_PER_MODEL);
		CalcInitialJointStates(game->renderObjs[game->nRenderObjs], game->jointParents, game->invJointTransforms, game->boneSpaceJointTransforms);
	}
	game->nRenderObjs++;
}

extern "C" void Init(Memory& mem) {
	ReloadableDLL* myDLL = (ReloadableDLL*) (mem.start + sizeof(Window) + sizeof(sf::RenderWindow));
	myDLL->mem = Alloc(mem, sizeof(Game));
	Game* game = (Game*) myDLL->mem;
	game->window = (Window*) mem.start;

	// init shaders
	InitDefaultShader(game->shader);
	InitShader(game->shadowShader, "shaders/shadowVS.glsl", "shaders/shadowFS.glsl");
	InitShader(game->fractalShader, "shaders/simpleVS.glsl", "shaders/fractalFS3.glsl");
	// InitShader(game->simpleShader, "shaders/simpleVS.glsl", "shaders/simpleFS.glsl");
	glUseProgram(game->shader.program);
	

	// init models
	game->nModels = 0;
	game->verticesOffset = 0;
	game->indicesOffset = 0;
	InitBuffers(game);
	InitGameModel(game, "models/square.obj"); // 0
	InitGameModel(game, "models/cube.obj"); // 1
	InitGameModel(game, "models/circle.obj"); // 2
	InitGameModel(game, "models/ike_sword.obj"); // 3
	InitGameModel(game, "C:\\Users\\Noxide\\Downloads\\model.dae"); // 4
	InitGameModel(game, "models/cube.obj"); // 5
	for(int i = 0; i < 6; i++) {
		InitGameModel(game, "models/cube.obj"); // 6-11
	}
	// InitGameModel(game, "C:\\Users\\Noxide\\Downloads\\Ike\\FitIke00.dae"); // 4
	InitGameModel(game, "models/millenium-falcon.obj"); // 12
	// InitGameModel(game, "models/cube.obj"); // 13
	// InitGameModel(game, "models/tie-fighter.dae"); // 13
	InitGameModel(game, "C:\\Users\\Noxide\\Downloads\\star-wars-vader-tie-fighter.obj"); // 13

	FillBuffers(game);

	// init textures
	game->nTextures = 0;
	InitGameTexture(game, "textures/bark.jpg"); // 0
	InitGameTexture(game, "textures/bark_normalMap.jpg"); // 1
	InitGameTexture(game, "textures/pebbles.jpg"); // 2
	InitGameTexture(game, "textures/pebbles_normalMap.jpg"); // 3
	InitGameTexture(game, "textures/ike.jpg"); // 4
	InitGameTexture(game, "textures/bricks.jpg"); // 5
	InitGameTexture(game, "textures/bricks_normal.jpg"); // 6
	InitGameTexture(game, "textures/bricks_disp.png"); // 7
	InitGameTexture(game, "textures/bricks2.jpg"); // 8
	InitGameTexture(game, "textures/bricks2_normal.jpg"); // 9
	InitGameTexture(game, "textures/bricks2_disp.jpg"); // 10
	InitGameTexture(game, "textures/falcon.jpg"); // 11
	InitGameTexture(game, "textures/tie-fighter-albedo.jpg"); // 12
	InitGameTexture(game, "textures/tie-fighter-normal.png"); // 13

	// init materials
	game->nMaterials = 0;
	// InitMaterial(game, &game->textures[0], &game->textures[1]); // 0
	InitGameMaterial(game, NULL, NULL); // 0
	InitGameMaterial(game, &game->textures[2], &game->textures[3]); // 1
	InitGameMaterial(game, NULL, NULL, NULL, v3(0.4f)); // 2
	InitGameMaterial(game, &game->textures[4]); // 3
	InitGameMaterial(game, &game->textures[5], &game->textures[6], &game->textures[7],v3(1,1,1),100.0f,0.03f,-0.5f); // 4
	InitGameMaterial(game, &game->textures[8], &game->textures[9], &game->textures[10],v3(1,1,1),100.0f,0.04f,-1.0f); // 5
	InitGameMaterial(game, NULL, NULL, NULL, v3(1.0f, 0.0f, 0.0f)); // 6
	InitGameMaterial(game, NULL, NULL, NULL, v3(1.0f, 1.0f, 1.0f)); // 7

	// init render objs
	game->nRenderObjs = 0;
	game->nRenderObjs++; // 0
	// InitGameRenderObj(mem, game, &game->models[1], &game->materials[1], v3(0.029765,0.057498,0.024097), v3(0,0,0), v3(0.1f,0.1f,0.1f)); // 1
	InitGameRenderObj(mem, game, &game->models[1], &game->materials[1], v3(-0.897014,0.364757,0.959163), v3(0,0,0), v3(0.1f,0.1f,0.1f)); // 1
	InitGameRenderObj(mem, game, &game->models[0], &game->materials[2], v3(0,0,0), v3(-3.14159265f / 2.0f, 0.0f, 3.14159265f), v3(500,500,1)); // 2
	InitGameRenderObj(mem, game, &game->models[0], &game->materials[3], v3(2,2,0)); // 3
	InitGameRenderObj(mem, game, &game->models[1], &game->materials[4], v3(-6,2,0)); // 4
	InitGameRenderObj(mem, game, &game->models[1], &game->materials[5], v3(-8,2,0)); // 5
	InitGameRenderObj(mem, game, &game->models[1], &game->materials[6], v3(0,2,0), v3(0,0,0), v3(0.01f, 0.01f, 0.01f)); // 6
	game->nRenderObjs++; // 7

	// init player and sword
	InitSwordObj(game->renderObjs[0], &game->models[3], &game->materials[0]);
	InitPlayerObj(game->renderObjs[7], &game->models[1], &game->materials[6]);

	// init dir light
	InitDirLight(game->dirLight);
	glUniform3fv(game->shader.u_lightDir, 1, &game->dirLight.cameraForShadows.dir[0]);
	glUniform3fv(game->shader.u_lightColor, 1, &game->dirLight.color[0]);
	BindTexture(game->shader.u_shadowMap, game->dirLight.shadowMap.texture, 3);

	// InitGameRenderObj(mem, game, &game->models[4], &game->materials[6], v3(4,2,0), v3(radians(-90.0f),0,0), v3(0.1f, 0.1f, 0.1f)); // 8?
	InitGameRenderObj(mem, game, &game->models[4], &game->materials[6], v3(4,2,0), v3(radians(-90.0f),0,0), v3(1.0f, 1.0f, 1.0f)); // 8?
	// InitGameRenderObj(mem, game, &game->models[4], &game->materials[6], v3(4,1,0), v3(radians(-90.0f),0,0), v3(-0.9f, 0.9f, 0.9f)); // 8?

	// init another debug cube
	InitGameRenderObj(mem, game, &game->models[1], &game->materials[6], v3(0,4,0)); // 9

	InitGameRenderObj(mem, game, &game->models[1], &game->materials[6], v3(0,4,0)); // 10
	InitGameRenderObj(mem, game, &game->models[5], &game->materials[6], v3(0,0,0)); // 11
	for(int i = 0; i < 6; i++) {
		InitGameRenderObj(mem, game, &game->models[5 + i], &game->materials[6], v3(0,0,0), v3(0,0,0), v3(0.1f,0.1f,0.1f)); // 12-17
	}
	game->debugCubeIndex = 0;
	game->debugDrawModel = true;

	InitGameRenderObj(mem, game, &game->models[0], &game->materials[7], v3(0, 2, -0.2f), v3(0,0,0), v3(10,10,10)); // 18

	
    InitFractalMap(game->fractalMap, game->window->sfml_window->getSize().x, game->window->sfml_window->getSize().y);
	glUniform1i(game->shader.shadow_mapping_enabled, 0);


	
	game->renderObjs[8].transform.matrix = mat4(1.0f);

	game->renderObjs[9].transform.scale = vec3(0.9f, 0.3f, 0.9f);

	mat4 jointMat = inverse(game->invJointTransforms[15]);
	vec3 translation = vec3(jointMat[0][3], jointMat[1][3], jointMat[2][3]);
	vec3 rotated = rotate(translation, radians(-90.0f), v3(1.0f, 0.0f, 0.0f));
	
	game->renderObjs[9].transform.pos = v3(4,2,0) + rotated;
	UpdateMatrices(game->renderObjs[9]);

	InitGameMaterial(game, NULL, NULL, NULL, v3(0.0f, 1.0f, 0.0f)); // 8
	game->renderObjs[9].material = &game->materials[8];


	InitGameMaterial(game, &game->textures[11]); // 9
	InitGameRenderObj(mem, game, &game->models[12], &game->materials[9], v3(0.0f, 1.0f, 5.0f), v3(0,0,0), v3(0.001f,0.001f,0.001f)); // 19
	
	InitGameMaterial(game, &game->textures[12], &game->textures[13]); // 10
	InitGameRenderObj(mem, game, &game->models[13], &game->materials[8], v3(-2.0f, 1.0f, 0.0f), v3(0,0,0), v3(0.5f,0.5f,0.5f)); // 20



	// init point / spot lights
	game->nLights = 0;

	// init camera
	InitCamera(game->camera, v3(0, 1, 3));
	game->cameraMoveSpeed = 0.4f;
	game->cameraRotateSpeed = 0.12f;
	game->camera.aspect = game->window->sfml_window->getSize().x / (r32) game->window->sfml_window->getSize().y;

	// misc OpenGL init stuff
	glEnable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);
	glClearColor(0.3f, 0.3f, 0.3f, 1.0f);

	// misc init stuff
	game->renderObjs[6].transform.scale = game->renderObjs[0].transform.scale * 0.1f;

	game->lastSwordTargetLookAtPos = vec3(0.0f, 1.0f, 0.0f);
	
	game->camera.dir = vec3(0,0,-1);
	// game->camera.dir = -normalize(vec3(0,3,8));
	game->camera.right = vec3(1,0,0);
	game->camera.up = cross(game->camera.right, game->camera.dir);
	// game->camera.pos = game->renderObjs[7].transform.pos - game->camera.dir * 7.0f;
	// game->camera.pos = 1.2f*vec3( 0.0f, 1.0f, 1.0f );
	game->camera.pos = vec3( 0.0f, 1.0f, 5.0f );
	UpdateMatrices(game->camera);

	game->usingGamepad = false;
	if(game->lastTickTime == 0)
		game->lastTickTime = MillisSinceEpoch();
}
extern "C" void Reinit(Memory& mem) {
	ReloadableDLL* myDLL = (ReloadableDLL*) (mem.start + sizeof(Window) + sizeof(sf::RenderWindow));
	Game* game = (Game*) myDLL->mem;

	printf("%f,%f,%f\n", game->camera.pos.x, game->camera.pos.y, game->camera.pos.z);
	
	// game->renderObjs[19].transform.pos

	// mat4 jointMat = inverse(game->invJointTransforms[14]);
	// vec3 translation2 = vec3(jointMat[0][3], jointMat[1][3], jointMat[2][3]);
	// vec3 rotated2 = rotate(translation2, radians(-90.0f), v3(1.0f, 0.0f, 0.0f));

	// jointMat = inverse(game->invJointTransforms[13]) * (inverse(game->invJointTransforms[14]) * game->invJointTransforms[13]);
	// vec3 translation = vec3(jointMat[0][3], jointMat[1][3], jointMat[2][3]);
	// vec3 rotated = rotate(translation, radians(-90.0f), v3(1.0f, 0.0f, 0.0f));
	
	// game->renderObjs[9].transform.pos = v3(4,2,0) + rotated + rotated2;
	// UpdateMatrices(game->renderObjs[9]);

	// mat4 local = inverse(game->invJointTransforms[13]) * game->invJointTransforms[0];
	// game->renderObjs[8].jointStates[13].transform.matrix = inverse(game->invJointTransforms[0]) * local * rotate(radians(20.0f), vec3(1, 0, 0)) * game->invJointTransforms[13];
	// local = inverse(game->invJointTransforms[14]) * game->invJointTransforms[13];
	// game->renderObjs[8].jointStates[14].transform.matrix = game->renderObjs[8].jointStates[13].transform.matrix * local * game->invJointTransforms[14];
	// local = inverse(game->invJointTransforms[15]) * game->invJointTransforms[14];
	// game->renderObjs[8].jointStates[15].transform.matrix = game->renderObjs[8].jointStates[14].transform.matrix * local * game->invJointTransforms[15];
	
	// game->renderObjs[8].jointStates[13].transform.matrix = game->renderObjs[8].jointStates[13].transform.matrix * rotate(radians(20.0f), vec3(1, 0, 0));
	// game->renderObjs[8].jointStates[15].transform.matrix = inverse(game->invJointTransforms[15]) * rotate(radians(20.0f), vec3(1, 0, 0)) * game->invJointTransforms[15];


	// game->renderObjs[9].transform.scale = vec3(0.9f, 0.3f, 0.9f);

	// mat4 jointMat = inverse(game->invJointTransforms[15]);
	// vec3 translation = vec3(jointMat[0][3], jointMat[1][3], jointMat[2][3]);
	// vec3 rotated = rotate(translation, radians(-90.0f), v3(1.0f, 0.0f, 0.0f));
	
	// game->renderObjs[9].transform.pos = v3(4,2,0) + rotated;
	// UpdateMatrices(game->renderObjs[9]);

	// game->renderObjs[8].jointStates[0].transform.scale = v3(2.0f);
	// CalcTransformMatrix(game->renderObjs[8].jointStates[0].transform);

	// printf("%f, %f, %f\n", translation.x, translation.y, translation.z);
	// printf("%f, %f, %f\n", rotated.x, rotated.y, rotated.z);

	// for(int i = 0; i < 4; i++) {
	// 	for(int j = 0; j < 4; j++) {
	// 		printf("%f, ", jointMat[i][j]);
	// 	}
	// 	printf("\n");
	// }

	// printf("%f\n", game->cameraMoveSpeed);
	// printf("%f\n", game->cameraRotateSpeed);

	// glUniform1i(game->shader.displacement_mapping_enabled, 1);
	// glUniform1i(game->shader.normal_mapping_enabled, 0);
	// glUniform1i(game->shader.specular_lighting_enabled, 0);
	// glUniform1i(game->shader.shadow_mapping_enabled, 1);
	// glUniform1i(game->shader.texture_mapping_enabled, 1);
	// glUniform1i(game->shader.lighting_enabled, 1);
	// glUniform1i(game->shader.ambient_lighting_enabled, 0);
	// glUniform1f(game->shader.u_shine, 150.0f);
	// glUniform1i(game->shader.pcf_enabled, 1);
	// glUniform1i(game->shader.shadow_mapping_enabled, 0);

	DeinitShader(game->fractalShader);
	InitShader(game->fractalShader, "shaders/simpleVS.glsl", "shaders/fractalFS3.glsl");

	// for(int i = 0; i < 6; i++) {
	// 	Model& secondCube = game->models[5 + i];
	// 	printf("%f, %f, %f\n", game->vertices[secondCube.verticesOffset].pos.x, game->vertices[secondCube.verticesOffset].pos.y, game->vertices[secondCube.verticesOffset].pos.z);
	// }

	printf("reinit called\n");
}

void ShadowRender(Game* game, FBO& shadowMap, Camera& cameraForShadows, GLuint u_mvpMatrixShadowPos) {
	glViewport(0, 0, shadowMap.width, shadowMap.height);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, shadowMap.id);
	glClear(GL_DEPTH_BUFFER_BIT);
	// glCullFace(GL_FRONT);


	// init attributes for shadow shader
	GLint posLoc = glGetAttribLocation(game->shadowShader, "a_position");
	GLint jointIndicesLoc = glGetAttribLocation(game->shadowShader, "a_jointIndices");
	GLint jointWeightsLoc = glGetAttribLocation(game->shadowShader, "a_jointWeights");

	glVertexAttribPointer(posLoc, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*) offsetof(Vertex, pos));
	glVertexAttribPointer(jointIndicesLoc, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*) offsetof(Vertex, jointIndices));
	glVertexAttribPointer(jointWeightsLoc, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*) offsetof(Vertex, jointWeights));

	glEnableVertexAttribArray(posLoc);
	glEnableVertexAttribArray(jointIndicesLoc);
	glEnableVertexAttribArray(jointWeightsLoc);



	GLuint skeletal_animations_enabledLoc = glGetUniformLocation(game->shadowShader, "skeletal_animations_enabled");
	GLuint jointTransformsLoc = glGetUniformLocation(game->shadowShader, "u_jointTransforms");
	for(u32 i = 0; i < game->nRenderObjs; i++) {
		RenderObj& obj = game->renderObjs[i];
		mat4 mvpMatrix = cameraForShadows.vpMatrix * obj.transform.matrix;
		glUniformMatrix4fv(u_mvpMatrixShadowPos, 1, GL_FALSE, &mvpMatrix[0][0]);

		if(obj.model->numJoints > 0) {
			CalcJointTransforms(obj, game->jointParents, game->invJointTransforms, game->jointTransforms, game->boneSpaceJointTransforms);
			glUniformMatrix4fv(jointTransformsLoc, MAX_JOINTS_PER_MODEL, GL_FALSE, &game->jointTransforms[0][0][0]);
    		glUniform1i(skeletal_animations_enabledLoc, 1);
		}
		else {
    		glUniform1i(skeletal_animations_enabledLoc, 0);
		}

		glDrawElements(GL_TRIANGLES, game->renderObjs[i].model->numIndices, GL_UNSIGNED_INT, (void*)(game->renderObjs[i].model->indicesOffset * sizeof(IndexType)));
	}

	// glCullFace(GL_BACK);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	glViewport(0, 0, game->window->sfml_window->getSize().x, game->window->sfml_window->getSize().y);
}


void FractalRender(Game* game) {
	glUseProgram(game->fractalShader);
	
	glDisable(GL_DEPTH_TEST);
	
	// glBindFramebuffer(GL_DRAW_FRAMEBUFFER, game->fractalMap.fbo);
	glClear(GL_COLOR_BUFFER_BIT);

	GLuint u_renderMap = glGetUniformLocation(game->fractalShader, "u_renderMap");
	BindTexture(u_renderMap, game->fractalMap.texture, 0);
	
	GLuint u_depthMap = glGetUniformLocation(game->fractalShader, "u_depthMap");
	BindTexture(u_depthMap, game->dirLight.shadowMap.texture, 1);

	GLint posLoc = glGetAttribLocation(game->fractalShader, "a_position");
	glVertexAttribPointer(posLoc, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*) offsetof(Vertex, pos));
	glEnableVertexAttribArray(posLoc);
	
	// GLuint iResolution = glGetUniformLocation(game->fractalShader, "iResolution");
	// vec2 resolution = vec2(game->window->sfml_window->getSize().x, game->window->sfml_window->getSize().y);
	// glUniform3fv(iResolution, 1, &resolution[0]);

	GLuint iTimeLoc = glGetUniformLocation(game->fractalShader, "iTime");
	glUniform1f(iTimeLoc, game->debugTime);
	
	// u_cameraPos
	GLuint u_cameraPos = glGetUniformLocation(game->fractalShader, "u_cameraPos");
	glUniform3fv(u_cameraPos, 1, &game->camera.pos[0]);
	// u_cameraDir
	GLuint u_cameraDir = glGetUniformLocation(game->fractalShader, "u_cameraDir");
	glUniform3fv(u_cameraDir, 1, &game->camera.dir[0]);

	// GLuint u_mvpMatrix = glGetUniformLocation(game->fractalShader, "u_mvpMatrix");
	// mat4 mvpMatrix = game->camera.vpMatrix * game->renderObjs[18].transform.matrix;
	// glUniformMatrix4fv(u_mvpMatrix, 1, GL_FALSE, &mvpMatrix[0][0]);

	GLuint u_vpMatrix = glGetUniformLocation(game->fractalShader, "u_vpMatrix");
	glUniformMatrix4fv(u_vpMatrix, 1, GL_FALSE, &game->camera.vpMatrix[0][0]);

	glDrawElements(GL_TRIANGLES, game->renderObjs[18].model->numIndices, GL_UNSIGNED_INT, (void*)(game->renderObjs[18].model->indicesOffset * sizeof(IndexType)));

	// glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

	glEnable(GL_DEPTH_TEST);

	glUseProgram(game->shader.program);
}

void UpdateCamera(GameInput& gInput, Camera& camera, vec3& playerPos, r32& moveSpeed, r32& rotateSpeed, bool usingGamepad) {
	moveSpeed += gInput.cameraMoveSpeedChange * 0.01f;
	// rotateSpeed += gInput.cameraRotateSpeedChange * 0.001f;
	rotateSpeed = 0.005f;

	if(usingGamepad) {
		RotateCamera(camera, -gInput.cameraRotate.x * 0.1f * rotateSpeed, -gInput.cameraRotate.y * rotateSpeed);
		camera.pos = playerPos - camera.dir * 7.0f;
	}
	else {
		if(gInput.cameraRotateSpeedChange != 0.0f) {
			rotateSpeed = 0.05f;
		}
		else {
			rotateSpeed = 0.005f;
		}
		camera.pos += camera.dir * -gInput.cameraMove.z * moveSpeed;
		camera.pos += camera.right * gInput.cameraMove.x * moveSpeed;
		camera.pos += camera.up * gInput.cameraMove.y * moveSpeed;

		RotateCamera(camera, gInput.cameraRotate.x * rotateSpeed, gInput.cameraRotate.y * rotateSpeed);
	}

	UpdateMatrices(camera);
}

extern "C" void Update(Memory& mem) {
	ReloadableDLL* myDLL = (ReloadableDLL*) (mem.start + sizeof(Window) + sizeof(sf::RenderWindow));
	Game* game = (Game*) myDLL->mem;

	Window* window = game->window;
	Input& input = window->input;

	window->sfml_window->setFramerateLimit(60);
	// LimitTickRate(TICK_RATE, game->lastTickTime);

#ifndef DISABLE_TICK_COUNTER
	game->ticks++;
	u64 curTime = MillisSinceEpoch();
	if(curTime - game->lastTickPrint > 1000.0) {
		printf("ticks: %lu\n", game->ticks);
		game->ticks = 1;
		game->lastTickPrint = curTime;
	}
#endif
	
	//// update

	// update input
	UpdateWindowInput(window);
	if(window->input.keys.down[sf::Keyboard::Escape]) {
		window->sfml_window->close();
		return;
	}
	if(window->input.windowResized) {
		game->camera.aspect = game->window->sfml_window->getSize().x / (r32) game->window->sfml_window->getSize().y;
	}

	GatherGameInput(game->gInput, window->input, game->camera, game->usingGamepad);

	// update camera
	UpdateCamera(game->gInput, game->camera, game->renderObjs[7].transform.pos, game->cameraMoveSpeed, game->cameraRotateSpeed, game->usingGamepad);
	glUniform3fv(game->shader.u_cameraPos, 1, &game->camera.pos[0]);

	// update sword
	UpdateSword(game->gInput, game->lastSwordTargetLookAtPos, game->renderObjs[0], game->renderObjs[7].transform.pos);

	// update player
	UpdatePlayer(game->renderObjs[7], game->charVel, game->renderObjs[0], game->lastSwordTargetLookAtPos, game->lastSwordTipPos);

	// update camera after player update
	// game->camera.pos = game->renderObjs[7].transform.pos - game->camera.dir * 7.0f;
	// UpdateMatrices(game->camera);
	// glUniform3fv(game->shader.u_cameraPos, 1, &game->camera.pos[0]);

	// update debug sword tip cube
	vec3 swordTipPos = vec3(0.0f, 0.0f, -14.3f);
	swordTipPos = vec3(game->renderObjs[0].transform.matrix * vec4(swordTipPos, 1.0f));
	game->renderObjs[6].transform.pos = swordTipPos;
	UpdateMatrices(game->renderObjs[6]);

	// headbob animation
	game->debugTime += 0.03f;
	// game->renderObjs[8].jointStates[0].transform.scale = v3(3.0f);
	// game->renderObjs[8].jointStates[2].transform.rot = v3(sin(game->debugTime * 0.01f), 0.0f, 0.0f);
	// game->renderObjs[8].jointStates[2].transform.rotq = rotate(game->renderObjs[8].jointStates[2].transform.rotq, 0.01f, vec3(1.0f, 0.0f, 0.0f));
	// for(int i = 2; i < game->renderObjs[8].model->numJoints; i++) {
	// 	game->renderObjs[8].jointStates[i].transform.scale = v3(1);
	// }
	// game->renderObjs[8].jointStates[1].transform.scale = v3(5.0f);
	// CalcTransformMatrix(game->renderObjs[8].jointStates[0].transform);
	// game->renderObjs[8].jointStates[2].transform.matrix = game->renderObjs[8].jointStates[0].transform.matrix * rotate(0.01f,vec3(1.0f, 0.0f, 0.0f));

	// game->renderObjs[8].jointStates[2].transform.matrix = game->boneSpaceJointTransforms[2] * rotate(sin(game->debugTime), vec3(0.0f, 0.0f, 1.0f));
	// game->renderObjs[8].jointStates[13].transform.matrix = game->boneSpaceJointTransforms[13] * rotate(sin(game->debugTime), vec3(1.0f, 0.0f, 0.0f));
	// game->renderObjs[8].jointStates[14].transform.matrix = game->boneSpaceJointTransforms[14] * rotate(-sin(game->debugTime), vec3(1.0f, 0.0f, 0.0f));
	// game->renderObjs[8].jointStates[10].transform.matrix = game->boneSpaceJointTransforms[10] * rotate(sin(game->debugTime), vec3(1.0f, 0.0f, 0.0f));


	Model& secondCube = game->models[5 + game->debugCubeIndex];
	if(window->input.keys.down[sf::Keyboard::T]) {
		for(int i = secondCube.verticesOffset; i < secondCube.verticesOffset + secondCube.numVertices; i++) {
			game->vertices[i].pos.y += 0.1f;
		}
	}
	if(window->input.keys.down[sf::Keyboard::F]) {
		for(int i = secondCube.verticesOffset; i < secondCube.verticesOffset + secondCube.numVertices; i++) {
			game->vertices[i].pos.x -= 0.1f;
		}
	}
	if(window->input.keys.down[sf::Keyboard::G]) {
		for(int i = secondCube.verticesOffset; i < secondCube.verticesOffset + secondCube.numVertices; i++) {
			game->vertices[i].pos.y -= 0.1f;
		}
	}
	if(window->input.keys.down[sf::Keyboard::H]) {
		for(int i = secondCube.verticesOffset; i < secondCube.verticesOffset + secondCube.numVertices; i++) {
			game->vertices[i].pos.x += 0.1f;
		}
	}
	if(window->input.keys.down[sf::Keyboard::R]) {
		for(int i = secondCube.verticesOffset; i < secondCube.verticesOffset + secondCube.numVertices; i++) {
			game->vertices[i].pos.z += 0.1f;
		}
	}
	if(window->input.keys.down[sf::Keyboard::Y]) {
		for(int i = secondCube.verticesOffset; i < secondCube.verticesOffset + secondCube.numVertices; i++) {
			game->vertices[i].pos.z -= 0.1f;
		}
	}
	if(window->input.keys.pressed[sf::Keyboard::M]) {
		game->debugCubeIndex = (game->debugCubeIndex + 1) % 6;
	}
	if(window->input.keys.pressed[sf::Keyboard::N]) {
		game->debugDrawModel = !game->debugDrawModel;
	}


	FillBuffers(game);
	bool colliding = collisionFractalSphere(game, game->renderObjs[11]);
	if(colliding) {
		game->materials[6].color = vec3(0.0f, 1.0f, 0.0f);
	}
	else {
		game->materials[6].color = vec3(1.0f, 0.0f, 0.0f);
	}

	if(window->input.keys.down[sf::Keyboard::Up]) {
		game->camera.pos += game->camera.dir * 0.01f;
	}
	if(window->input.keys.down[sf::Keyboard::Down]) {
		game->camera.pos -= game->camera.dir * 0.01f;
	}
	if(window->input.keys.down[sf::Keyboard::Left]) {
		game->camera.pos -= game->camera.right * 0.01f;
	}
	if(window->input.keys.down[sf::Keyboard::Right]) {
		game->camera.pos += game->camera.right * 0.01f;
	}
	if(window->input.keys.down[sf::Keyboard::N]) {
		game->camera.pos += game->camera.up * 0.01f;
	}
	if(window->input.keys.down[sf::Keyboard::M]) {
		game->camera.pos -= game->camera.up * 0.01f;
	}
	UpdateMatrices(game->camera);
	glUniform3fv(game->shader.u_cameraPos, 1, &game->camera.pos[0]);


	//// render

	// glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	// glUseProgram(game->fractalShader);
	// int i = 18;
	// GLint posLoc = glGetAttribLocation(game->fractalShader, "a_position");
	// glVertexAttribPointer(posLoc, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*) offsetof(Vertex, pos));
	// glEnableVertexAttribArray(posLoc);
    // GLuint iTimeLoc = glGetUniformLocation(game->fractalShader, "iTime");
    // glUniform1f(iTimeLoc, game->debugTime);
	// glDrawElements(GL_TRIANGLES, game->renderObjs[i].model->numIndices, GL_UNSIGNED_INT, (void*)(game->renderObjs[i].model->indicesOffset * sizeof(IndexType)));

	// fractal render
	// FractalRender(game);

	// shadow render
	glUseProgram(game->shadowShader);
	GLuint u_mvpMatrixShadowPos = glGetUniformLocation(game->shadowShader, "u_mvpMatrix");
	// shadow render for dir light
	// ShadowRender(game, game->dirLight.shadowMap, game->dirLight.cameraForShadows, u_mvpMatrixShadowPos);
	// // shadow render for point / spot lights
	// for(u32 i = 0; i < game->nLights; i++) {
	// 	ShadowRender(game, game->lights[i].shadowMap, game->lights[i].cameraForShadows, u_mvpMatrixShadowPos);
	// }
	glUseProgram(game->shader.program);
	BindTexture(game->shader.u_shadowMap, game->dirLight.shadowMap.texture, 3);
	InitAttributes(game);

	// standard render
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, game->fractalMap.id);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	// glDepthFunc(GL_LESS);
	for(u32 i = 0; i < game->nRenderObjs; i++) {
		RenderObj& obj = game->renderObjs[i];
		if(i == 8 && !game->debugDrawModel)
			continue;
		if(i != 1 && i != 3 && i != 4 && i != 5 && i != 11 && i != 19 && i != 20)
			continue;
		// if(i == 18) {
		// 	glUseProgram(game->fractalShader);

		// 	GLint posLoc = glGetAttribLocation(game->fractalShader, "a_position");
		// 	glVertexAttribPointer(posLoc, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*) offsetof(Vertex, pos));
		// 	glEnableVertexAttribArray(posLoc);

		// 	GLuint iTimeLoc = glGetUniformLocation(game->fractalShader, "iTime");
		// 	glUniform1f(iTimeLoc, game->debugTime);
			
		// 	// u_cameraPos
		// 	GLuint u_cameraPos = glGetUniformLocation(game->fractalShader, "u_cameraPos");
		// 	glUniform3fv(u_cameraPos, 1, &game->camera.pos[0]);
		// 	// u_cameraDir
		// 	GLuint u_cameraDir = glGetUniformLocation(game->fractalShader, "u_cameraDir");
		// 	glUniform3fv(u_cameraDir, 1, &game->camera.dir[0]);

		// 	GLuint u_mvpMatrix = glGetUniformLocation(game->fractalShader, "u_mvpMatrix");
		// 	mat4 mvpMatrix = game->camera.vpMatrix * obj.transform.matrix;
		// 	glUniformMatrix4fv(u_mvpMatrix, 1, GL_FALSE, &mvpMatrix[0][0]);

		// 	glDrawElements(GL_TRIANGLES, game->renderObjs[i].model->numIndices, GL_UNSIGNED_INT, (void*)(game->renderObjs[i].model->indicesOffset * sizeof(IndexType)));
		// 	glUseProgram(game->fractalShader);
		// 	continue;
		// }
		BindMaterial(game, *game->renderObjs[i].material);

		// disable mapping for objects without that map
		GLint texture_mapping_enabled;
		glGetUniformiv(game->shader.program, game->shader.texture_mapping_enabled, &texture_mapping_enabled);
		if(game->renderObjs[i].material->texture == NULL)
			glUniform1i(game->shader.texture_mapping_enabled, 0);
		GLint normal_mapping_enabled;
		glGetUniformiv(game->shader.program, game->shader.normal_mapping_enabled, &normal_mapping_enabled);
		if(game->renderObjs[i].material->normalMap == NULL)
			glUniform1i(game->shader.normal_mapping_enabled, 0);
		GLint displacement_mapping_enabled;
		glGetUniformiv(game->shader.program, game->shader.displacement_mapping_enabled, &displacement_mapping_enabled);
		if(game->renderObjs[i].material->dispMap == NULL)
			glUniform1i(game->shader.displacement_mapping_enabled, 0);

		if(i == 18) {
			BindTexture(game->shader.u_texture, game->fractalMap.texture, 0);
			glUniform1i(game->shader.texture_mapping_enabled, 1);
		}

		// when drawing renderObj 3, use the light's shadow map as the texture
		if(i == 3) {
			BindTexture(game->shader.u_texture, game->dirLight.shadowMap.texture, 0);
			// BindTexture(game->shader.u_texture, game->renderObjs[game->nRenderObjs - 1].material->dispMap->texture, 0);
		}

		// update model uniforms
		glUniformMatrix4fv(game->shader.u_modelMatrix, 1, GL_FALSE, &obj.transform.matrix[0][0]);
		glUniformMatrix4fv(game->shader.u_normalMatrix, 1, GL_FALSE, &obj.normalMatrix[0][0]);
		mat4 mvpMatrix = game->camera.vpMatrix * obj.transform.matrix;
		glUniformMatrix4fv(game->shader.u_mvpMatrix, 1, GL_FALSE, &mvpMatrix[0][0]);

		// load jointTransforms
		if(obj.model->numJoints > 0) {
			// for(int j = 0; j < obj.model->numJoints; j++) {
			// 	game->jointTransforms[j] = obj.jointStates[j].transform.matrix;
			// }
			CalcJointTransforms(obj, game->jointParents, game->invJointTransforms, game->jointTransforms, game->boneSpaceJointTransforms);
			glUniformMatrix4fv(game->shader.u_jointTransforms, MAX_JOINTS_PER_MODEL, GL_FALSE, &game->jointTransforms[0][0][0]);
    		glUniform1i(game->shader.skeletal_animations_enabled, 1);
		}
		else {
    		glUniform1i(game->shader.skeletal_animations_enabled, 0);
		}

		// update light uniforms
		mvpMatrix = game->dirLight.cameraForShadows.vpMatrix * obj.transform.matrix;
		glUniformMatrix4fv(game->shader.u_mvpMatrixFromLight, 1, GL_FALSE, &mvpMatrix[0][0]);
		// TODO: update point / spot light uniforms

		// draw the render obj
		glDrawElements(GL_TRIANGLES, game->renderObjs[i].model->numIndices, GL_UNSIGNED_INT, (void*)(game->renderObjs[i].model->indicesOffset * sizeof(IndexType)));

		// reset back to what we had for mapping
		glUniform1i(game->shader.normal_mapping_enabled, normal_mapping_enabled);
		glUniform1i(game->shader.texture_mapping_enabled, texture_mapping_enabled);
		glUniform1i(game->shader.displacement_mapping_enabled, displacement_mapping_enabled);
	}
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	
	FractalRender(game);

	UpdateWindowDisplay(window);
}
extern "C" void Deinit(Memory& mem) {
	ReloadableDLL* myDLL = (ReloadableDLL*) (mem.start + sizeof(Window) + sizeof(sf::RenderWindow));
	Game* game = (Game*) myDLL->mem;

	DeinitBuffers(game);

	for(u32 i = 0; i < game->nMaterials; i++) {
		DeinitMaterial(game->materials[i]);
	}

	DeinitShader(game->shadowShader);
	// DeinitShader(game->simpleShader);
	DeinitShader(game->shader.program);
}

/*
#include "gfx/dae_loader.h"
#ifndef MEM_SIZE
#define MEM_SIZE 50 MB
#endif

#ifndef WINDOW_WIDTH
// #define WINDOW_WIDTH 1920
#define WINDOW_WIDTH 1200
#endif

#ifndef WINDOW_HEIGHT
// #define WINDOW_HEIGHT 1080
#define WINDOW_HEIGHT 800
#endif

int main() {
	// Memory mem;
	// InitMemory(mem, MEM_SIZE);

	// void* win = Alloc(mem, sizeof(Window)); // alloc this separately to make it the first thing allocated
	// Window* window = InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, win, Alloc(mem, sizeof(sf::RenderWindow)));
	// glewInit();

	// Init(mem);
	
	// while(window->sfml_window->isOpen()) {
	// 	Update(mem);
	// }
	

	// RiggedModel rModel;
	// LoadDAE(&rModel, "C:\\Users\\Noxide\\Downloads\\model.dae");
	
	// printf("vertex data\n");
	// for(int i = 0; i < rModel.iModel.positions.size() / 10; i++) {
	// 	printf("%f, %f, %f\n", rModel.iModel.positions[i].x, rModel.iModel.positions[i].y, rModel.iModel.positions[i].z);
	// 	printf("%f, %f, %f\n", rModel.iModel.normals[i].x, rModel.iModel.normals[i].y, rModel.iModel.normals[i].z);
	// 	printf("%f, %f\n\n", rModel.iModel.uvCoords[i].x, rModel.iModel.uvCoords[i].y);
	// }
	// printf("\nindices\n");
	// for(int i = 0; i < rModel.iModel.indices.size(); i++) {
	// 	printf("%d\n", rModel.iModel.indices[i]);
	// }

	// printf("\njointIndices\n");
	// for(int i = 0; i < rModel.jointIndices.size(); i++) {
	// 	printf("%d, %d, %d, %d\n", rModel.jointIndices[i].x, rModel.jointIndices[i].y, rModel.jointIndices[i].z, rModel.jointIndices[i].w);
	// }
	// printf("\njointWeights\n");
	// for(int i = 0; i < rModel.jointWeights.size(); i++) {
	// 	printf("%f, %f, %f, %f\n", rModel.jointWeights[i].x, rModel.jointWeights[i].y, rModel.jointWeights[i].z, rModel.jointWeights[i].w);
	// }
	// printf("\ninvJointTransforms\n");
	// for(int i = 0; i < rModel.invJointTransforms.size(); i++) {
	// 	printf("%f, %f, %f, %f\n", rModel.invJointTransforms[i][0][0], rModel.invJointTransforms[i][0][1], rModel.invJointTransforms[i][0][2], rModel.invJointTransforms[i][0][3]);
	// 	printf("%f, %f, %f, %f\n", rModel.invJointTransforms[i][1][0], rModel.invJointTransforms[i][1][1], rModel.invJointTransforms[i][1][2], rModel.invJointTransforms[i][1][3]);
	// 	printf("%f, %f, %f, %f\n", rModel.invJointTransforms[i][2][0], rModel.invJointTransforms[i][2][1], rModel.invJointTransforms[i][2][2], rModel.invJointTransforms[i][2][3]);
	// 	printf("%f, %f, %f, %f\n", rModel.invJointTransforms[i][3][0], rModel.invJointTransforms[i][3][1], rModel.invJointTransforms[i][3][2], rModel.invJointTransforms[i][3][3]);
	// }
	// printf("\njointParents\n");
	// for(int i = 0; i < rModel.jointParents.size(); i++) {
	// 	printf("%d ", rModel.jointParents[i]);
	// }
	// printf("\n");
	// printf("\njointNamesToIndices\n");
	// for(auto i = rModel.jointNamesToIndices.begin(); i != rModel.jointNamesToIndices.end(); i++) {
	// 	printf("(%s, %d) ", i->first.c_str(), i->second);
	// }
	// printf("\n");
	

	// vector<ivec4> jointIndices;
	// vector<vec4> jointWeights;
	// vector<mat4> invJointTransforms;
	// vector<int> jointParents;
	// map<string, int> jointNamesToIndices;

	// Deinit(mem);

	// DeinitWindow(window);

	// DeinitMemory(mem);
}
*/
