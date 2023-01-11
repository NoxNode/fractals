#pragma once
#include "../core/types.h"

struct Vertex {
    v3 pos;
    v2 uvCoords;
    v3 normal;
    v3 tangent;
    // v3 bitangent;
    vec4 jointIndices;
    vec4  jointWeights;
};
