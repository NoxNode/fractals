#pragma once
#include "types.h"
#include <SFML/Graphics.hpp>

#define NUM_KEYS sf::Keyboard::KeyCount
struct KeyInput {
	bool down[NUM_KEYS];
	bool pressed[NUM_KEYS];  // true for one frame
	bool released[NUM_KEYS]; // true for one frame
};

#define NUM_MOUSE_BUTTONS sf::Mouse::ButtonCount
struct Mouse {
	bool buttonDown[NUM_MOUSE_BUTTONS];
	bool buttonPressed[NUM_MOUSE_BUTTONS];  // true for one frame
	bool buttonReleased[NUM_MOUSE_BUTTONS]; // true for one frame
	v2 pos;
	v2 scrollDelta; // amount scrolled this frame (scrollDelta.y is the vertical scroll wheel, .x is horizontal (if there is one))
};

#define NUM_GAMEPADS sf::Joystick::Count
#define NUM_GAMEPAD_BUTTONS sf::Joystick::ButtonCount
#define NUM_GAMEPAD_AXES sf::Joystick::AxisCount
struct Gamepad {
	bool connected;
	bool buttonDown[NUM_GAMEPAD_BUTTONS];
	bool buttonPressed[NUM_GAMEPAD_BUTTONS];  // true for one frame
	bool buttonReleased[NUM_GAMEPAD_BUTTONS]; // true for one frame
	r32 axes[NUM_GAMEPAD_AXES];
};

struct Input {
	KeyInput keys;
	Mouse mouse;
	Gamepad gamepads[NUM_GAMEPADS];
	bool windowClosed;
	bool windowInFocus;
	bool windowLostFocus;  // true for one frame
	bool windowGainedFocus; // true for one frame
	bool windowResized; // true for one frame
};

namespace XBoxButton {
	enum {
		X = 0,
		Circle,
		Square,
		Triangle,
		L1,
		R1,
		Select,
		Start,
		L3,
		R3
	};
}

namespace XBoxAxis {
	enum {
		LeftX = 0,
		LeftY,
		Triggers,
		Unknown,
		RightX,
		RightY,
		DPadX,
		DPadY
	};
}

namespace PS3Button {
	enum {
		Select = 0,
		L3,
		R3,
		Start,
		DPadUp,
		DPadRight,
		DPadDown,
		DPadLeft,
		L2,
		R2,
		L1,
		R1,
		Triangle,
		Circle,
		X,
		Square,
		PSButton
	};
}

namespace PS3Axis {
	enum {
		LeftX = 0,
		LeftY,
		RightX,
		RightY
	};
}

Gamepad* GetFirstConnectedGamePad(Input& input) {
	Gamepad* gamepads = input.gamepads;
	for(u32 i = 0; i < NUM_GAMEPADS; i++) {
		if(gamepads[i].connected)
			return &gamepads[i];
	}
	return NULL;
}

void HandleSFMLInput(Input& input, sf::Event& event) {
	KeyInput& keys = input.keys;
	Mouse& mouse = input.mouse;
	Gamepad* gamepads = input.gamepads;

	switch(event.type) {
		// general window events
		case sf::Event::Closed: {
			input.windowClosed = true;
		} break;
		case sf::Event::GainedFocus: {
			input.windowInFocus = true;
			input.windowGainedFocus = true;
		} break;
		case sf::Event::LostFocus: {
			input.windowInFocus = false;
			input.windowLostFocus = true;
		} break;
		case sf::Event::Resized: {
			input.windowResized = true;
		} break;
		// key input
		case sf::Event::KeyPressed: {
			if(event.key.code < 0 || event.key.code >= NUM_KEYS)
				break;
			keys.down[event.key.code] = true;
			keys.pressed[event.key.code] = true;
		} break;
		case sf::Event::KeyReleased: {
			if(event.key.code < 0 || event.key.code >= NUM_KEYS)
				break;
			keys.down[event.key.code] = false;
			keys.released[event.key.code] = true;
		} break;
		// gamepad input
		case sf::Event::JoystickConnected: {
			if(event.joystickConnect.joystickId < 0 || event.joystickConnect.joystickId >= NUM_GAMEPADS)
				break;
			gamepads[event.joystickConnect.joystickId].connected = true;
		} break;
		case sf::Event::JoystickDisconnected: {
			if(event.joystickConnect.joystickId < 0 || event.joystickConnect.joystickId >= NUM_GAMEPADS)
				break;
			gamepads[event.joystickConnect.joystickId].connected = false;
		} break;
		case sf::Event::JoystickButtonPressed: {
			if(event.joystickConnect.joystickId < 0 || event.joystickConnect.joystickId >= NUM_GAMEPADS)
				break;
			if(event.joystickButton.button < 0 || event.joystickButton.button >= NUM_GAMEPAD_BUTTONS)
				break;
			gamepads[event.joystickButton.joystickId].buttonDown[event.joystickButton.button] = true;
			gamepads[event.joystickButton.joystickId].buttonPressed[event.joystickButton.button] = true;
			gamepads[event.joystickButton.joystickId].connected = true;
		} break;
		case sf::Event::JoystickButtonReleased: {
			if(event.joystickConnect.joystickId < 0 || event.joystickConnect.joystickId >= NUM_GAMEPADS)
				break;
			if(event.joystickButton.button < 0 || event.joystickButton.button >= NUM_GAMEPAD_BUTTONS)
				break;
			gamepads[event.joystickButton.joystickId].buttonDown[event.joystickButton.button] = false;
			gamepads[event.joystickButton.joystickId].buttonReleased[event.joystickButton.button] = true;
			gamepads[event.joystickButton.joystickId].connected = true;
		} break;
		case sf::Event::JoystickMoved: {
			if(event.joystickConnect.joystickId < 0 || event.joystickConnect.joystickId >= NUM_GAMEPADS)
				break;
			if(event.joystickMove.axis < 0 || (unsigned int) event.joystickMove.axis >= NUM_GAMEPAD_AXES)
				break;
			gamepads[event.joystickMove.joystickId].axes[event.joystickMove.axis] = event.joystickMove.position;
			gamepads[event.joystickMove.joystickId].connected = true;
		} break;
		// mouse input
		case sf::Event::MouseMoved: {
			mouse.pos.x = event.mouseMove.x;
			mouse.pos.y = event.mouseMove.y;
		} break;
		case sf::Event::MouseButtonPressed: {
			if(event.mouseButton.button < 0 || event.mouseButton.button >= NUM_MOUSE_BUTTONS)
				break;
			mouse.buttonDown[event.mouseButton.button] = true;
			mouse.buttonPressed[event.mouseButton.button] = true;
		} break;
		case sf::Event::MouseButtonReleased: {
			if(event.mouseButton.button < 0 || event.mouseButton.button >= NUM_MOUSE_BUTTONS)
				break;
			mouse.buttonDown[event.mouseButton.button] = false;
			mouse.buttonReleased[event.mouseButton.button] = true;
		} break;
		case sf::Event::MouseWheelScrolled: {
			if (event.mouseWheelScroll.wheel == sf::Mouse::VerticalWheel) {
				mouse.scrollDelta.y += event.mouseWheelScroll.delta;
			}
			else if (event.mouseWheelScroll.wheel == sf::Mouse::HorizontalWheel) {
				mouse.scrollDelta.x += event.mouseWheelScroll.delta;
			}
		} break;
		default:
			break;
	}
}

void ResetInput(Input& input) {
	KeyInput& keys = input.keys;
	Mouse& mouse = input.mouse;
	Gamepad* gamepads = input.gamepads;

	input.windowLostFocus = false;
	input.windowGainedFocus = false;
	input.windowResized = false;
	for(u32 i = 0; i < NUM_KEYS; i++) {
		keys.pressed[i] = false;
		keys.released[i] = false;
	}
	for(u32 i = 0; i < NUM_MOUSE_BUTTONS; i++) {
		mouse.buttonPressed[i] = false;
		mouse.buttonReleased[i] = false;
	}
	for(u32 i = 0; i < NUM_GAMEPADS; i++) {
		for(u32 j = 0; j < NUM_GAMEPAD_BUTTONS; j++) {
			gamepads[i].buttonPressed[j] = false;
			gamepads[i].buttonReleased[j] = false;
		}
	}
}
