#ifndef AUDIO_MANAGER_H
#define AUDIO_MANAGER_H

#include <string>
#include <vector>
#include <unordered_map>
#include <AL/al.h>
#include <AL/alc.h>

namespace AudioManager {

    void init();

    void shutdown();

    void setMusicVolume(float volume);
    void setSoundVolume(float volume);

    float getMusicVolume();
    float getSoundVolume();

    void addMusicTrack(const std::string& filepath);

    void playRandomMusic();

    void playSoundEffect(const std::string& filepath);

    void update(float deltaTime);

}

#endif
