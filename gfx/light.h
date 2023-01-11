#pragma once
#include  "../core/types.h"
#include "texture.h"
#include "camera.h"

struct DirLight {
    v3 color;

    // for shadows
    Camera cameraForShadows;
    FBO shadowMap;
};

void InitDirLight(DirLight& dirLight, v3 dir = v3(-1,-1,-1), v3 color = v3(1.0f, 1.0f, 1.0f)) {
    dirLight.color = color;
    InitOrthoCamera(dirLight.cameraForShadows, v3(10,10,10), dir);
    // dirLight.cameraForShadows.farClip = 10000;
    dirLight.cameraForShadows.leftClip = -5.0f;
    dirLight.cameraForShadows.rightClip = 5.0f;
    dirLight.cameraForShadows.topClip = -5.0f;
    dirLight.cameraForShadows.bottomClip = 5.0f;
    UpdateOrthoMatrices(dirLight.cameraForShadows);
    // InitShadowMap(dirLight.shadowMap, 4096, 4096);
    InitShadowMap(dirLight.shadowMap, 1024, 1024);
}

struct Light {
    v3 pos;
    v3 lookDir;
    v3 color;

    // for spot lights
    r32 angle;

    // coefficients that make up the fade out function
    r32 constant;
    r32 linear;
    r32 quadratic;

    // for shadows
    Camera cameraForShadows;
    FBO shadowMap;
};

void InitLight(Light& light, v3 pos, v3 lookDir, v3 color = v3(1,1,1),
r32 angle = 90, r32 constant = 0, r32 linear = 0, r32 quadratic = 0) {
    light.pos = pos;
    light.lookDir = lookDir;
    light.color = color;
    light.angle = angle;
    light.constant = constant;
    light.linear = linear;
    light.quadratic = quadratic;
    // TODO: cube map for shadows
}
