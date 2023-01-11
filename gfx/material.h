#pragma once
#include "texture.h"
#include "../core/types.h"

struct Material {
    Texture* texture;
    Texture* normalMap;
    Texture* dispMap; // parallax displacement map
    v3 color;
    r32 shininess; // TODO: can make this a specular map
    r32 dispMapScale;
    r32 dispMapBias;

    // for physics
    r32 restitution; // aka bounciness or how easily something bounces off it
    r32 friction;
};

void InitMaterial(Material& material, Texture* texture = NULL, Texture* normalMap = NULL, Texture* dispMap = NULL,
v3 color = v3(1,1,1), r32 shininess = 100.0f, r32 dispMapScale = 0.04f, r32 dispMapOffset = 0.0f, r32 restitution = 0.5f, r32 friction = 1.0f) {
    material.texture = texture;
    material.normalMap = normalMap;
    material.dispMap = dispMap;
    material.dispMapScale = dispMapScale;
    r32 baseBias = dispMapScale / 2.0f;
    material.dispMapBias = -baseBias + baseBias * dispMapOffset;
    material.color = color;
    material.shininess = shininess;
    material.restitution = restitution;
    material.friction = friction;
}

void DeinitMaterial(Material& material) {
    if(material.texture != NULL) {
        DeinitTexture(material.texture->texture);
    }
    if(material.normalMap != NULL) {
        DeinitTexture(material.normalMap->texture);
    }
    if(material.dispMap != NULL) {
        DeinitTexture(material.dispMap->texture);
    }
}
