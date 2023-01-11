#include "core/udp_socket.h"
#include "core/rdll.h"

#ifndef COMPILE_SERVER
	#include <GL/glew.h>
	#include <SFML/Window.hpp>
	#include <SFML/Audio.hpp>
	#include <SFML/OpenGL.hpp>
	#include "core/window.h"
	#include "gfx/shader.h"
	#include "gfx/camera.h"
	#include "gfx/light.h"
	#include "gfx/default_shader.h"
	#include "gfx/obj_loader.h"
	#include "gfx/dae_loader.h"
	#include "gfx/material.h"
	#include "gfx/default_renderer.h"
	#include "gfx/raymarch_renderer.h"
	#include "gfx/assets.h"
#else
	// since the server doesn't need these definitions, just declare them so we can use render_obj.h
	struct Model;
	struct Material;
#endif
#include "gfx/render_obj.h"
#include "core/memory.h"
#include "game/game_input.h"
#include "physics/collision.h"
#include "physics/othergjk.h"
#include "audio/audio_engine.h"
#include "core/map.h"
#include "gfx/text_renderer.h"

/*
TODOs

games TODOs
	influence game
	star wars shooter in fractal world
	pvp furi
	skyrim 6

features TODOs are below

Mark TODOs
	Conv hull decomp
	Physics
	physics based animations / ragdolls
		start with simple box or sphere collider
		move with simple particle physics
		face toward velocity
		tilt in direction of acceleration (rotate around COM)
		pass / reach pose
		stride wheel
		walking at slow speeds
		blend between walk and run
		bounce up and down (larger bounce when walk, smaller bounce when run)
		bicubic interpolation instead of linear (maybe can use splines?)
		crouch keyframe (spring interpolation)
		jump keyframes (use crouch for landing)
		flip keyframe (change curve to have some anticipation and slowly go back to falling pose)
		rolling (front and side keyframe, interpolate between)
		13 keyframes (2 walk, 2 run, 2 crouching walk, 2 jump, 2 roll, idle, crouch, flip)
		IK (foot to uneven ground, hands to sloped ledge - have 1 keyframe for grab ledge, move hands up or down for slope)
			head/torso/eye lookAt
		secondary physics - cloth sims or just wobbling equipment as you move
		wall run
		active ragdolls
			ragdolls but still trying to do things
			3 flail keys, 1 protect and protect front keyframe
			gangbeasts always active ragdoll with invisible support at the hips
			https://www.youtube.com/watch?v=HF-cp6yW3Iw
				lerp from ragdoll to target rotations from a keyframed animation
				make the hips more closely match it and make it an exponential curve instead of linear to really snap fast
	IK Rig
		animations that apply to many different models and even different rigs of those models
		abstract out key joints and connections (hip->neck, neck->head, l_hand->neck, r_hand->neck, l_foot->hip, r_foot->hip)
		animations only track positions and rotations of those joints
		joints in between and beyond are updating using IK and constraints and custom coded rules (like more mass means more hunched over or the foot hitting the ground harder)

If we want terrain or any large scale thing to repeat a texture, we can just make a uniform for scaling up the uv coordinates

Voxel Octree Renderer - https://www.youtube.com/watch?v=mcpLSHU8M1c

idea for making layered equipment/clothes not clip weirdly
	pre-processing step
		loop through each pair of equipment/clothes
			for each vertex, check if it's inside the other mesh
			if all vertices are inside, there's no problem
			if all vertices are outside, there's no problem
			otherwise, determine which one should be seen as the outer one by counting how many are outside
			then store all faces (really the elements of the faces) of the inner one that includes any vertices that go outside
	rendering step
		if there's a currently equipped pair of clothing that had some faces stored in the pre-processing step
		render the subset of faces that exclude vertices that go outside

instanced rendering - example in OldAttempts/final_modularity/src/graphics.cpp
	InstanceData class
	gl_buffers.h
		instance data vbo
		instance data array
		MAX_INSTANCES
		instancesOffset
	add instancesOffset, numInstances and maxInstances to Model
		fill when we load model
	add instanceIndex to renderObj
		when creating renderObj, check the numInstances and maxInstances of the model to see if it can get a valid instanceIndex and increment numInstances if so
		when rendering, first loop through the renderObjs and fill the instance data array for at the model's instancesOffset
			then fill the instance data vbo with that instance data array (might have to have the vbo just have that model's data)
			then loop through the models and glDrawElementsInstanced
	when filling the instance data vbo, loop through all renderObjs and fill the instance data array at the instanceIndex with the matrix from its Transform
		also calculate and store the normal matrix
		also any other instance data we want to store

physics
	fix gjk or just do a custom version
		can maybe just find the 3 vertices closest to the origin
		find which side the origin is on by doing the dot product with the normal of the face
		find the shortest line perpendicular to that face that passes through the origin
		use that as the penetration vector for collision response
	broad phase
		have models store their max extents
		have function for finding max extents (just go through all vertices keeping track of max x, max y, min x, min y)
		sort objects by their min x extent
		fill map from object to list of potentially intersecting objects
		for each key, sort the list of potentially intersecting by their min y extent
		loop through and filter out any objects that's min/max y extends don't overlap with the key's y extents

		alternative

		sort objects by min x extent
		split objects by median x extent (duplicate any if the split line is within their min/max x extents)
		for both splits, sort by min y extent, split by median y extent
		continue to build a tree until splitting no longer removes objects
		iterate through the tree to the leaves and do collision with whatever list of objects are at the leaves (should only be ones that could collide)
		
boss fight adaptive AI - queue learning
	map from fight state to most effective action
	each time the boss wants to do the next action, it captures what the current fight state is
	it then runs a comparison function between that current fight state to all fight states in its storage
	that comparison function gives a similiarity score between 0 and 1
	we take that similiarity score, multiply it by some "trying something new" factor (like 0.9 if it wants to guarantee at least 10% of the time it tries something new)
	1 - the result of that multiplication is the probability it will try something new, otherwise it goes with the stored most effective action
	when trying something new, we can keep track of the previous most effective action and if the new action is more effective after some tries, replace the previous one
		so maybe it's a map from fight state to a pair (most effective action and in trial for most effective action)
	whether we're trying something new or old, we measure the effectiveness by not only measuring if the action had a direct desirable result (like hitting the player)
	but we also measure how effective the actions following up / leading up to this one was
	this makes it so moving toward the player is an action that can have a measurable reward (if it led to an attack that landed)
	tunable parameters
		what makes up a fight state
		how to generate the similarity score between 2 fight states
		what actions the boss has
		what variables can be tuned within the actions (like not just choosing what attack, but how you aim an attack)
		how to evaluate the effectiveness of an action, maybe if the effectiveness is dependent on follow ups, it weights the effectiveness of future moves more heavily
		experimentChance aka the "trying something new" factor
		how many tries of an experiment and what effectiveness does it take to replace a currently effective action
	can also start with no pre-defined fight states and it needs to fill the map with fight states
	so when encountering a fight state that doesn't have over a certain similiarity score to any of the currently stored fight states, maybe replace one of them (or add if you have space)
	the criteria for what fight state you replace could be as simple as how many times of the previous x queries did you use that fight state aka how often does that fight state come up
	could also instead of only replacing if the current fight state doesn't match well to the ones you have stored, you could just replace fight states that haven't matched to one in a long time

	could also try to incorporate chains of fight states (or at least just the previous one) instead of just using the current fight state

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

General API/Component Guidelines (https://www.youtube.com/watch?v=ZQ5_u8Lgvyk)
	Granularity - A or BC (best to have tiers)
		Flexibility vs simplicity
	Redundancy - A or B (multiple versions of functions that take different parameters - good until it's too much)
		Convenience vs orthogonality
	Coupling - A implies B
		Less is always better
	Retention - A mirrors B (best to have none at most granular tier)
		Synchronization vs automation
		Good example at https://youtu.be/ZQ5_u8Lgvyk?t=2338
	Flow Control - A invokes B
		More game control is always better
	
	Usually at the start of a project, you want low granularity and high retention (ez to use)
		later on you want high granularity and low retention (more control in the game code)

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
#define MAX_RENDER_OBJS 32
#define MAX_LIGHTS 5

/// collision defines
#define MAX_COLLISION_OBJS 100

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

// #include <thread>
// void LimitTickRate(u64 millisPerTick, u64& lastTickTime) {
// 	u64 curTickTime = MillisSinceEpoch();
// 	u64 millisSinceLastTick = curTickTime - lastTickTime;
// 	if(millisSinceLastTick < millisPerTick) {
// 		std::this_thread::sleep_for(std::chrono::milliseconds(millisPerTick - millisSinceLastTick));
// 		curTickTime = MillisSinceEpoch();
// 		millisSinceLastTick = curTickTime - lastTickTime;
// 	}
// 	// extraMillisFromLastTick = millisSinceLastTick - millisPerTick;
// 	// printf("extraMillisFromLastTick: %f\n", extraMillisFromLastTick);
// 	lastTickTime = curTickTime;
// }
struct CollisionObj
{
	u32 parent;
	v3 vertices[8]; // convex
	Transform transform;
};
struct Game {
	Window* window;

	//// renderers
	DefaultRenderer renderer;
	RaymarchRenderer raymarchRenderer;
	TextRenderer text_renderer;

	//// gl buffers
	VBO vbo;
	IBO ibo;
	JointBuffers jointBuffers;

	//// assets
	Assets assets;

	u32 nSounds;
	u32 nSoundBuffers;

	//// game objects
	Camera camera;
	DirLight dirLight;
	u32 nRenderObjs;
	RenderObj renderObjs[MAX_RENDER_OBJS];
	Map* characters;
	u32 nLights;
	Light lights[MAX_LIGHTS];


	//// sound data
	sf::SoundBuffer** sound_buffers;
	sf::Sound* sounds[MAX_SOUNDS];

	///// collision crap
	CollisionObj* collision_objs[MAX_COLLISION_OBJS];
	u32 collision_object_size;

	//// temp code
	r32 debugTime;
	GLuint testShader;
	GLuint textShader;

	vec3 lookAtDir;

	u8 reserved[1 KB];
};


void InitGameSound(Memory& mem, Game* game, const char* soundPath)
{
	if(game->nSoundBuffers >= MAX_SOUND_BUFFERS) {
		printf("exceeded max sounds\n");
		return;
	}
	sf::SoundBuffer* buffer = game->sound_buffers[game->nSoundBuffers];
	bool loaded = false;
	loaded = buffer->loadFromFile(soundPath);
	if(!loaded) {
		printf("failed to load sound file: %s\n", soundPath);
		game->nSoundBuffers++;
		return;
	}
	game->nSoundBuffers++;
}

void LoadSound(Memory& mem, Game* game, u8 buffer_index)
{
	sf::Sound* sound = new
		(Alloc(mem, sizeof(sf::Sound)))
		(sf::Sound)();
	game->sounds[game->nSounds] = sound;
	sound->setBuffer(*game->sound_buffers[buffer_index]);
	game->nSounds++;
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
		CalcInitialJointStates(game->renderObjs[game->nRenderObjs], game->jointBuffers);
	}
	game->nRenderObjs++;
}

void InitCharacters(Map*& map, Memory& mem) {
	map = InitMap(mem, 130, 3);
	return loadCharacters(map, mem);
}

void CreateBoneCollisionBBox(Memory& mem, Game* game, RenderObj& renderObj, CollisionObj& collisionObj) {
	// psuedocode
	/*
		game->array for collision objects
		int for collision object size

		go through each bone
			add collision object for this bone to game->array
			set the parent of this collision object to the bone parent

			max for each direction init here
			go through each vertex
				if the current vertex is most influenced by the current bone
					add vertex to collision obj
					if exceeds max in certain direction
						change max for each direction init before
			replace current collision object vertices with max
	*/

	auto beginning_offset = game->collision_object_size;
	for (int i = 0; i < renderObj.model->numJoints; i++) {
		if (game->collision_object_size >= MAX_COLLISION_OBJS) {
			printf("YOIKES\n");
			exit(1);
		}
		game->collision_objs[game->collision_object_size] = (CollisionObj*)Alloc(mem, sizeof(CollisionObj));
		auto parent = game->jointBuffers.jointParents[i];
		auto starting_joint_idx = renderObj.model->jointsOffset;
		auto diff = parent - starting_joint_idx;
		game->collision_objs[game->collision_object_size]->parent = diff + beginning_offset;
		
		float max_x = FLT_MAX;
		float max_y = FLT_MAX;
		float max_z = FLT_MAX;
		float min_x = -FLT_MAX;
		float min_y = -FLT_MAX;
		float min_z = -FLT_MAX;
		
		for (int j = 0; j < renderObj.model->numVertices; j++) {
			float maxWeight = 0;
			int maxWeightIndex = 0;
			for(int k = 0; k < 4; k++) {
				if(game->vbo.vertices[renderObj.model->verticesOffset].jointWeights[k] > maxWeight) {
					maxWeight = game->vbo.vertices[renderObj.model->verticesOffset].jointWeights[k];
					maxWeightIndex = k;
				}
			}
			// if the joint most influencing the current vertex is the current joint
			if(game->vbo.vertices[renderObj.model->verticesOffset].jointIndices[maxWeightIndex] == i) { // might be renderObj.model->jointsOffest + i
				// update max vertex pos values
				if(max_x < game->vbo.vertices[renderObj.model->verticesOffset].pos.x) {
					max_x = game->vbo.vertices[renderObj.model->verticesOffset].pos.x;
				}
				if(max_y < game->vbo.vertices[renderObj.model->verticesOffset].pos.y) {
					max_y = game->vbo.vertices[renderObj.model->verticesOffset].pos.y;
				}
				if(max_z < game->vbo.vertices[renderObj.model->verticesOffset].pos.z) {
					max_z = game->vbo.vertices[renderObj.model->verticesOffset].pos.z;
				}
				if(min_x < game->vbo.vertices[renderObj.model->verticesOffset].pos.x) {
					min_x = game->vbo.vertices[renderObj.model->verticesOffset].pos.x;
				}
				if(min_y < game->vbo.vertices[renderObj.model->verticesOffset].pos.y) {
					min_y = game->vbo.vertices[renderObj.model->verticesOffset].pos.y;
				}
				if(min_z < game->vbo.vertices[renderObj.model->verticesOffset].pos.z) {
					min_z = game->vbo.vertices[renderObj.model->verticesOffset].pos.z;
				}
			}
		}
		// replace current collision object vertices with max
		game->collision_objs[game->collision_object_size]->vertices[0] = v3(min_x, min_y, min_z); // bottom left front
		game->collision_objs[game->collision_object_size]->vertices[1] = v3(min_x, min_y, max_z); // bottom left back
		game->collision_objs[game->collision_object_size]->vertices[2] = v3(max_x, min_y, max_z); // bottom right back
		game->collision_objs[game->collision_object_size]->vertices[3] = v3(max_x, min_y, min_z); // bottom right front
		game->collision_objs[game->collision_object_size]->vertices[4] = v3(min_x, max_y, min_z); // top left front
		game->collision_objs[game->collision_object_size]->vertices[5] = v3(min_x, max_y, max_z); // top left back
		game->collision_objs[game->collision_object_size]->vertices[6] = v3(max_x, max_y, max_z); // top right back
		game->collision_objs[game->collision_object_size]->vertices[7] = v3(max_x, max_y, min_z); // top right front

		game->collision_object_size++;
	}
}

extern "C" void Init(Memory& mem) {
	ReloadableDLL* myDLL = (ReloadableDLL*) (mem.start + sizeof(Window) + sizeof(sf::RenderWindow));
	myDLL->mem = Alloc(mem, sizeof(Game));
	Game* game = (Game*) myDLL->mem;
	game->window = (Window*) mem.start;
	game->window->sfml_window->setFramerateLimit(60);
	game->sound_buffers = (sf::SoundBuffer**) (mem.start + sizeof(Window) + sizeof(sf::RenderWindow) + sizeof(ReloadableDLL));

	InitCharacters(game->characters, mem);
	InitGLBuffers(game->vbo, game->ibo);

	// init renderers and shaders
	InitDefaultRenderer(game->renderer, game->vbo, game->ibo, false, game->window->sfml_window->getSize().x, game->window->sfml_window->getSize().y);
	InitTextRenderer(game->text_renderer);
	InitRaymarchRenderer(game->raymarchRenderer, game->vbo, game->ibo, &game->renderer.fbo);
	InitShader(game->testShader, "shaders/simpleVS.glsl", "shaders/simpleFS.glsl");

	InitDefaultAssets(game->assets, game->vbo, game->ibo, game->jointBuffers);

	FillGLBuffers(game->vbo, game->ibo);

	// init render objs
	game->nRenderObjs = 0;
	// small cube
	InitGameRenderObj(mem, game, &game->assets.models[1], &game->assets.materials[1], v3(-0.897014,0.364757,0.959163), v3(0,0,0), v3(0.1f,0.1f,0.1f)); // 0
	// floor
	InitGameRenderObj(mem, game, &game->assets.models[0], &game->assets.materials[2], v3(0,-1.0f,0), v3(-3.14159265f / 2.0f, 0.0f, 3.14159265f), v3(500,500,1.0f)); // 1
	// ike quad
	InitGameRenderObj(mem, game, &game->assets.models[0], &game->assets.materials[3], v3(2,1,0)); // 2
	// bricks1 cube
	InitGameRenderObj(mem, game, &game->assets.models[1], &game->assets.materials[4], v3(-6,2,0)); // 3
	// bricks2 cube
	InitGameRenderObj(mem, game, &game->assets.models[1], &game->assets.materials[5], v3(-8,2,0)); // 4
	// thinmatrix skeletal animation model
	InitGameRenderObj(mem, game, &game->assets.models[3], &game->assets.materials[8], v3(4,1,0), v3(0,radians(180.0f),0), v3(0.3f, 0.3f, 0.3f)); // 5
	game->lookAtDir = vec3(0,0,-1);
	game->renderObjs[5].curAnimation = 0;

	InitGameRenderObj(mem, game, &game->assets.models[1], &game->assets.materials[6], vec3(0,0,0)); // 6
	InitGameRenderObj(mem, game, &game->assets.models[1], &game->assets.materials[6], vec3(1.1f,0,0)); // 7

	// thinmatrix skeletal animation model 2
	InitGameRenderObj(mem, game, &game->assets.models[3], &game->assets.materials[8], v3(2,1,0), v3(0,radians(180.0f),0), v3(0.3f, 0.3f, 0.3f)); // 8
	game->lookAtDir = vec3(0,0,1);
	game->renderObjs[5].curAnimation = 0;

	// init bounding box collision around each bone (optional)
	CollisionObj collisionObj;
	CreateBoneCollisionBBox(mem, game, game->renderObjs[5], collisionObj);

	// // falcon
	// CreateMaterialAsset(game->assets, &game->assets.textures[11]); // 8
	// InitGameRenderObj(mem, game, &game->assets.models[5], &game->assets.materials[8], v3(0.0f, 1.0f, 5.0f), v3(0,0,0), v3(0.001f,0.001f,0.001f)); // 8
	
	// // vader
	// CreateMaterialAsset(game->assets, NULL, NULL, NULL, vec3(0.0f, 1.0f, 0.0f)); // 9
	// InitGameRenderObj(mem, game, &game->assets.models[6], &game->assets.materials[9], v3(-2.0f, 1.0f, 0.0f), v3(0,0,0), v3(0.5f,0.5f,0.5f)); // 9

	// init dir light
	InitDirLight(game->dirLight);

	// init sounds
	InitGameSound(mem, game, "sounds/noice.wav");
	LoadSound(mem, game, 0);

	// init point / spot lights
	game->nLights = 0;

	// init camera
	InitCamera(game->camera, v3(0, 1, 3), normalize(v3(0.0f, -0.5f, -1.0f)));
	game->camera.aspect = game->window->sfml_window->getSize().x / (r32) game->window->sfml_window->getSize().y;

	// misc OpenGL init stuff
	glEnable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);
	glClearColor(0.3f, 0.3f, 0.3f, 1.0f);
}
extern "C" void Reinit(Memory& mem) {
	ReloadableDLL* myDLL = (ReloadableDLL*) (mem.start + sizeof(Window) + sizeof(sf::RenderWindow));
	Game* game = (Game*) myDLL->mem;

	printf("%f,%f,%f\n", game->camera.pos.x, game->camera.pos.y, game->camera.pos.z);
	
	DeinitShader(game->raymarchRenderer.shader);
	InitShader(game->raymarchRenderer.shader, "shaders/simpleVS.glsl", "shaders/fractalFS3.glsl");

	// game->camera.dir = normalize(v3(0.0f, -0.75f, -1.0f));
	// UpdateMatrices(game->camera);
	// game->renderObjs[5].transform.pos = v3(4,-0.5f,0);
	// UpdateMatrices(game->renderObjs[5]);

    glUniform1i(game->renderer.shader.pcf_enabled, 0);

	printf("reinit called\n");
}

r32 getInputAxis(Input& input, sf::Keyboard::Key down, sf::Keyboard::Key up) {
	return input.keys.down[up] - input.keys.down[down];
}

vec2 getInputXYAxis(Input& input, sf::Keyboard::Key up, sf::Keyboard::Key left, sf::Keyboard::Key down, sf::Keyboard::Key right) {
	return { getInputAxis(input, left, right), getInputAxis(input, down, up) };
}

vec3 getInputXYZAxis(Input& input, sf::Keyboard::Key up, sf::Keyboard::Key left, sf::Keyboard::Key down, sf::Keyboard::Key right, sf::Keyboard::Key zdown, sf::Keyboard::Key zup) {
	return { getInputAxis(input, left, right), getInputAxis(input, down, up), getInputAxis(input, zdown, zup) };
}

vec3 getInputXZYAxis(Input& input, sf::Keyboard::Key up, sf::Keyboard::Key left, sf::Keyboard::Key down, sf::Keyboard::Key right, sf::Keyboard::Key zdown, sf::Keyboard::Key zup) {
	return { getInputAxis(input, left, right), getInputAxis(input, zdown, zup), getInputAxis(input, down, up) };
}

extern "C" void Update(Memory& mem) {
	ReloadableDLL* myDLL = (ReloadableDLL*) (mem.start + sizeof(Window) + sizeof(sf::RenderWindow));
	Game* game = (Game*) myDLL->mem;

	Window* window = game->window;
	Input& input = window->input;

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

	// sound stuff
	if(window->input.keys.down[sf::Keyboard::Space]) {
		game->sounds[0]->play();
	}

	// collision between test cubes at index 6 and 7
	vec3 move6 = getInputXZYAxis(input, sf::Keyboard::W, sf::Keyboard::A, sf::Keyboard::S, sf::Keyboard::D, sf::Keyboard::Q, sf::Keyboard::E);
	move6.z = -move6.z;
	vec3 move7 = getInputXZYAxis(input, sf::Keyboard::T, sf::Keyboard::F, sf::Keyboard::G, sf::Keyboard::H, sf::Keyboard::R, sf::Keyboard::Y);
	move7.z = -move7.z;

	const r32 moveSpeed = 0.01f;
	const r32 gravity = 0.1f;
	const r32 drag = 0.90f;
	game->assets.materials[6].restitution = 0.0f;

	// apply drag
	game->renderObjs[6].velocity *= drag;
	game->renderObjs[7].velocity *= drag;

	game->renderObjs[6].velocity += move6 * moveSpeed;
	game->renderObjs[7].velocity += move7 * moveSpeed;

	game->renderObjs[6].velocity.y -= gravity;
	game->renderObjs[7].velocity.y -= gravity;

	game->renderObjs[6].transform.pos += game->renderObjs[6].velocity; // TODO: * by deltaTime here
	game->renderObjs[7].transform.pos += game->renderObjs[7].velocity;

	UpdateMatrices(game->renderObjs[6]);
	UpdateMatrices(game->renderObjs[7]);

	Collider a = { &game->renderObjs[6], game->vbo.vertices };
	Collider b = { &game->renderObjs[7], game->vbo.vertices };
	vec3 mtv;
	if(gjk(&a, &b, &mtv)) {
		ResolveRigidCollision(game->renderObjs[6], game->renderObjs[7], -mtv);
	}
	// collide with floor (simple way)
	if(game->renderObjs[6].transform.pos.y - 0.5f < -1.0f) {
		game->renderObjs[6].transform.pos.y = -0.5f;
	}
	if(game->renderObjs[7].transform.pos.y - 0.5f < -1.0f) {
		game->renderObjs[7].transform.pos.y = -0.5f;
	}
	UpdateMatrices(game->renderObjs[6]);
	UpdateMatrices(game->renderObjs[7]);

	

	// free move camera
	r32 cameraMoveSpeed = 0.01f;
	r32 rotateUpSpeed = 0.01f;
	r32 rotateRightSpeed = 0.03f;
	
	vec3 moveRightForwardUp = cameraMoveSpeed * getInputXYZAxis(input, sf::Keyboard::Up, sf::Keyboard::Left, sf::Keyboard::Down, sf::Keyboard::Right, sf::Keyboard::N, sf::Keyboard::M);
	vec2 rotRightUp = getInputXYAxis(input, sf::Keyboard::I, sf::Keyboard::J, sf::Keyboard::K, sf::Keyboard::L);
	rotRightUp.x *= rotateRightSpeed;
	rotRightUp.y *= rotateUpSpeed;
	FreeMoveCamera(game->camera, moveRightForwardUp, rotRightUp);
	UpdateMatrices(game->camera);

	// headbob animation
	// game->debugTime += 1.0f / 60.0f;
	// game->renderObjs[5].jointStates[2].transform.matrix = game->jointBuffers.boneSpaceJointTransforms[2] * rotate(0.45f + sin(game->debugTime * 7.225f) / 3, vec3(0.0f, 0.0f, 1.0f));


	// animation
#if 0
	// move guy
	RenderObj& monk = game->renderObjs[5];
	// const r32 moveSpeed = 0.1f;
	// vec3 move6 = getInputXZYAxis(input, sf::Keyboard::W, sf::Keyboard::A, sf::Keyboard::S, sf::Keyboard::D, sf::Keyboard::Q, sf::Keyboard::E);
	// move6.z = -move6.z;
	Camera& camera = game->camera;
	vec3 movement = camera.dir * move6.z + camera.right * move6.x + camera.up * move6 * camera.up;
	movement.y = 0;
	if(movement.x != 0 || movement.y != 0 || movement.z != 0) {
		movement = normalize(movement);
	}
	monk.transform.pos += movement * moveSpeed;
	// monk.transform.rot.z += sin(game->debugTime * 0.0001f);
	// monk.transform.rotq = quat(1.0f, camera.dir.x, camera.dir.y, camera.dir.z);
	if(movement.x != 0 || movement.y != 0 || movement.z != 0) {
		game->lookAtDir = -movement;
		if(monk.curAnimation == 0) {
			monk.curAnimation = 7;
			monk.nextAnimation = 7;
			monk.animationTime = 0;
			monk.prevKeyFrameIndex = 0;
			monk.nextKeyFrameIndex = 1;
		}
	}
	else if(monk.curAnimation == 7) {
		monk.curAnimation = 0;
		monk.nextAnimation = 0;
		monk.animationTime = 0;
		monk.prevKeyFrameIndex = 0;
		monk.nextKeyFrameIndex = 1;
	}

	if(input.mouse.buttonPressed[sf::Mouse::Left]) {
		monk.curAnimation = 1;
		monk.nextAnimation = 0;
		monk.animationTime = 0;
		monk.prevKeyFrameIndex = 0;
		monk.nextKeyFrameIndex = 1;
	}
	else if(input.mouse.buttonPressed[sf::Mouse::Right]) {
		monk.curAnimation = 2;
		monk.nextAnimation = 0;
		monk.animationTime = 0;
		monk.prevKeyFrameIndex = 0;
		monk.nextKeyFrameIndex = 1;
	}


	monk.animationTime += 1.0f / 60.0f;
	FindKeyFramesForInterpolation(monk, game->jointBuffers);
	KeyFrame& prev = game->jointBuffers.animationKeyFrames[monk.curAnimation][monk.prevKeyFrameIndex];
	KeyFrame& next = game->jointBuffers.animationKeyFrames[monk.curAnimation][monk.nextKeyFrameIndex];
	r32 t = GetInterpolationValue(prev, next, monk.animationTime);
	if(t > 1.0f) {
		printf("it outa wack\n");
	}
	InterpolateKeyFrames(prev, next, t, monk.jointStates);


	CalcTransformMatrixWithLookAtRot(monk.transform, game->lookAtDir);
	monk.normalMatrix = transpose(inverse(monk.transform.matrix));

	// follow monk with camera
	game->camera.pos = monk.transform.pos - game->camera.dir * 4.0f;
	UpdateMatrices(game->camera);
#endif

	//// render
	DefaultRender(game->renderer, game->camera, game->renderObjs, game->nRenderObjs, game->dirLight, game->lights, game->nLights, game->jointBuffers, game->window->sfml_window->getSize().x, game->window->sfml_window->getSize().y);
	// RaymarchRender(game->raymarchRenderer, game->camera);
	// printf("CAMERA POS: %d %d %d", game->camera.pos.x, game->camera.pos.y, game->camera.pos.z);
	// TextRender(game->text_renderer, "I FIGHT FOR MY FRIENDS", game->camera, vec3(0,1, 0), vec3(0, 4, 0), .01, 0, 0, 0, game->characters);

	// glUseProgram(game->testShader);
	// GLuint u_mvpMatrix = glGetUniformLocation(game->testShader, "u_mvpMatrix");
	// glUniformMatrix4fv(u_mvpMatrix, 1, GL_FALSE, &game->camera.vpMatrix[0][0]);
	// glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (void*)(0 * sizeof(IndexType)));

	UpdateWindowDisplay(window);
}
extern "C" void Deinit(Memory& mem) {
	ReloadableDLL* myDLL = (ReloadableDLL*) (mem.start + sizeof(Window) + sizeof(sf::RenderWindow));
	Game* game = (Game*) myDLL->mem;

	DeinitAssets(game->assets);
	DeinitTexture(game->dirLight.shadowMap.texture);
	for(u32 i = 0; i < game->nLights; i++) {
		DeinitTexture(game->lights[i].shadowMap.texture);
	}

	DeinitGLBuffers(game->vbo, game->ibo);
	DeinitDefaultRenderer(game->renderer);
	DeinitRaymarchRenderer(game->raymarchRenderer);
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
