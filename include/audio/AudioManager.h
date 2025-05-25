#ifndef AUDIO_MANAGER_H
#define AUDIO_MANAGER_H

#include <string>
#include <vector>
#include <AL/al.h>

#include "core/threads/ThreadSafeQueue.h"
#include "core/world/Block.h"

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

    void playBlockSound(BLOCKTYPE blockType, const glm::vec3& soundPos, const glm::vec3& playerPos, const std::string& soundType);
    void playSoundEffect(const std::string& path, float gain, const glm::vec3* position);

    int getFreeSourceIndex();

    void startAudioThread();
    void startSoundThread();

    bool loadAudioFile(const std::string& path, ALuint& buffer, ALenum& format, ALsizei& freq, std::vector<short>& pcm);
}

#endif
