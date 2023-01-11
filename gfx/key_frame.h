#pragma once
#include "transform.h"

#define MAX_JOINTS_PER_MODEL 32

struct KeyFrame {
    Transform jointTransforms[MAX_JOINTS_PER_MODEL];
    r32 timestamp;
};
