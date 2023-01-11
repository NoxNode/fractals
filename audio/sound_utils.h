
#pragma once

#include "math.h"

static float dBToVolume(float dB)
{
    return powf(10.0f, 0.05f * dB);
}

static float volumeToDB(float volume)
{
    return 20.0f * log10f(volume);
}