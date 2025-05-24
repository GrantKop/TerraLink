#ifndef AUDIO_MANAGER_H
#define AUDIO_MANAGER_H

#include <string>
#include <vector>
#include <AL/al.h>

#include "core/threads/ThreadSafeQueue.h"

struct DecodedAudio {
    std::vector<short> pcm;
    ALenum format;
    ALsizei freq;
    std::string trackPath;
};

namespace AudioManager {

    void init();
    void shutdown();

    void setMusicVolume(float volume);
    void setSoundVolume(float volume);

    float getMusicVolume();
    float getSoundVolume();

    void addMusicTrack(const std::string& filepath);

    void requestNextTrack();
    void update(float deltaTime);
}

#endif
