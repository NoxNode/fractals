#pragma once
#include "../core/input.h"
#include "../gfx/camera.h"
#include "../core/packet.h"

struct NetInput {
	u64 timestamp;
	v3 swordTargetLookAtPos;
};

void PrintNetInput(NetInput& nInput) {
	printf("timestamp: %llu\n", nInput.timestamp);
	printf("swordTargetLookAtPos: (%f, %f, %f)\n", nInput.swordTargetLookAtPos.x, nInput.swordTargetLookAtPos.y, nInput.swordTargetLookAtPos.z);
}

Packet& operator<<(Packet& packet, const NetInput& data) {
	packet << data.timestamp;
	packet << data.swordTargetLookAtPos;
	return packet;
}

Packet& operator>>(Packet& packet, NetInput& data) {
	packet >> data.timestamp;
	packet >> data.swordTargetLookAtPos;
	return packet;
}

struct GameInput : public NetInput {
	v3 cameraMove;
	v2 cameraRotate;
	r32 cameraMoveSpeedChange;
	r32 cameraRotateSpeedChange;
};

void ResetGameInput(GameInput& gInput) {
	gInput.timestamp = 0;
	gInput.swordTargetLookAtPos = v3(0.0f);

	gInput.cameraMove = v3(0.0f);
	gInput.cameraRotate = v2(0.0f);
	gInput.cameraMoveSpeedChange = 0.0f;
	gInput.cameraRotateSpeedChange = 0.0f;
}

#include <chrono>
using namespace std::chrono;
// gets time in milliseconds
u64 MillisSinceEpoch() {
	return system_clock::now().time_since_epoch() / milliseconds(1);
}

void GatherGameInput(GameInput& gInput, Input& input, Camera& camera, bool& usingGamepad) {
	ResetGameInput(gInput);
	gInput.timestamp = MillisSinceEpoch();

	// // camera move speed
	// if(input.keys.down[sf::Keyboard::Up])
	// 	gInput.cameraMoveSpeedChange += 1;
	// if(input.keys.down[sf::Keyboard::Down])
	// 	gInput.cameraMoveSpeedChange -= 1;

	// // camera rotate speed
	if(input.keys.down[sf::Keyboard::RShift])
		gInput.cameraRotateSpeedChange += 1;
	// if(input.keys.down[sf::Keyboard::Left])
	// 	gInput.cameraRotateSpeedChange -= 1;
	
	// if(input.keys.pressed[sf::Keyboard::G])
	// 	usingGamepad = !usingGamepad;

	Gamepad* gamepad = GetFirstConnectedGamePad(input);
	if(usingGamepad && gamepad != NULL) {
		// rotate camera
		r32 rightX = gamepad->axes[XBoxAxis::RightX] / 100.0f;
		r32 rightY = gamepad->axes[XBoxAxis::RightY] / 100.0f;

		if(abs(rightX) > 0.1f) {
			gInput.cameraRotate += v2(0.0f, rightX);
		}
		if(abs(rightY) > 0.1f) {
			gInput.cameraRotate += v2(rightY, 0.0f);
		}

		// player sword target pos
		r32 x = gamepad->axes[XBoxAxis::LeftX] / 100.0f;
		r32 z = gamepad->axes[XBoxAxis::LeftY] / 100.0f;
		if(abs(x) < 0.05f && abs(z) < 0.05f) {
			x = 0;
			z = -0.001f;
		}
		r32 y = 1 - glm::min(sqrt(x*x + z*z), 0.9f) / 0.9f;
		if(gamepad->buttonDown[XBoxButton::R1]) {
			y = -y;
		}

		vec3 targetLookAtPos = vec3(x, 0, z);
		r32 towardForwardAmt = dot(targetLookAtPos, vec3(0,0,-1));
		vec3 towardForward = vec3(0,0,-1) * towardForwardAmt;

		// TODO: to optimize, we can just take the diff of the x coords
		// r32 towardForwardToPosLen = length(targetLookAtPos - towardForward);
		r32 towardForwardToPosLen = abs(targetLookAtPos.x - towardForward.x);

		vec3 cameraTowardForward = vec3(camera.dir.x, 0, camera.dir.z) * towardForwardAmt;
		vec3 cameraTowardForwardToPos;
		if(x < 0.0f)
			cameraTowardForwardToPos = cameraTowardForward - camera.right * towardForwardToPosLen;
		else
			cameraTowardForwardToPos = cameraTowardForward + camera.right * towardForwardToPosLen;
		gInput.swordTargetLookAtPos = normalize(vec3(cameraTowardForwardToPos.x, y, cameraTowardForwardToPos.z));
	}
	else {
		// player sword target pos
		// TODO: improve keyboard sword controls if we really want to
		gInput.swordTargetLookAtPos = vec3(0.0f, 0.001f, 0.0f);
		if(input.keys.down[sf::Keyboard::Space])
			gInput.swordTargetLookAtPos.y = -0.001f;
		if(input.keys.down[sf::Keyboard::W])
			gInput.swordTargetLookAtPos.z -= 1;
		if(input.keys.down[sf::Keyboard::A])
			gInput.swordTargetLookAtPos.x -= 1;
		if(input.keys.down[sf::Keyboard::S])
			gInput.swordTargetLookAtPos.z += 1;
		if(input.keys.down[sf::Keyboard::D])
			gInput.swordTargetLookAtPos.x += 1;
		gInput.swordTargetLookAtPos = normalize(gInput.swordTargetLookAtPos);

		// // camera move
		// if(input.keys.down[sf::Keyboard::W])
		// 	gInput.cameraMove += v3(0, 0, -1);
		// if(input.keys.down[sf::Keyboard::A])
		// 	gInput.cameraMove += v3(-1, 0, 0);
		// if(input.keys.down[sf::Keyboard::S])
		// 	gInput.cameraMove += v3(0, 0, 1);
		// if(input.keys.down[sf::Keyboard::D])
		// 	gInput.cameraMove += v3(1, 0, 0);
		// if(input.keys.down[sf::Keyboard::Q])
		// 	gInput.cameraMove += v3(0, 1, 0);
		// if(input.keys.down[sf::Keyboard::E])
		// 	gInput.cameraMove += v3(0, -1, 0);

		// camera rotate
		if(input.keys.down[sf::Keyboard::I])
			gInput.cameraRotate += v2(1, 0);
		if(input.keys.down[sf::Keyboard::J])
			gInput.cameraRotate += v2(0, 1);
		if(input.keys.down[sf::Keyboard::K])
			gInput.cameraRotate += v2(-1, 0);
		if(input.keys.down[sf::Keyboard::L])
			gInput.cameraRotate += v2(0, -1);
	}
}
