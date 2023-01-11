
#pragma once
#include <string>
#include "../core/types.h"
#include "../core/string.h"

constexpr u8 MAX_SOUND_BUFFERS = 20;
constexpr u8 MAX_SOUNDS = 20;
constexpr u8 MAX_CHANNELS = 8;

class AudioEngine
{
    public:
        static void init();
        static void update();
        static void shutdown();

        void loadSound(const String& sound_name, bool is_3D=true, bool is_looping=false, bool is_stream=false);
        void unloadSound(const String& sound_name);
        void set3DListenerAndOrientation(const v3& position, const v3& look, const v3& up);
        int playSound(const String& sound_name, const v3& position, float volume_dB=0.0f);
        void stopChannel(int channel_id);
        void stopAllChannels();
        void setChannel3DPosition(int channel_id, const v3& position);
        void setChannelVolume(int channel_id, float volume_dB);
        bool isPlaying(int channel_id);

};

class Sound
{

};

class Channel
{
    
};