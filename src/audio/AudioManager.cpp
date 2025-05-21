#include "audio/AudioManager.h"
#include "stb_vorbis.c"  // Must be compiled as .c or before cpp includes

#include <AL/al.h>
#include <AL/alc.h>
#include <random>
#include <chrono>
#include <iostream>

namespace AudioManager {

    static ALCdevice* device = nullptr;
    static ALCcontext* context = nullptr;

    static float musicVolume = 1.0f;
    static float soundVolume = 1.0f;

    static std::vector<std::string> musicTracks;
    static ALuint musicSource = 0;
    static ALuint musicBuffer = 0;

    static float timeSinceLastTrack = 0.0f;
    static float nextTrackDelay = 30.0f;

    std::default_random_engine rng(std::chrono::system_clock::now().time_since_epoch().count());

    void init() {
        device = alcOpenDevice(nullptr);
        if (!device) {
            std::cerr << "[Audio] Failed to open device.\n";
            return;
        }

        context = alcCreateContext(device, nullptr);
        if (!context || !alcMakeContextCurrent(context)) {
            std::cerr << "[Audio] Failed to set context.\n";
            return;
        }

        alGenSources(1, &musicSource);
    }

    void shutdown() {
        alDeleteSources(1, &musicSource);
        alDeleteBuffers(1, &musicBuffer);

        if (context) {
            alcMakeContextCurrent(nullptr);
            alcDestroyContext(context);
        }

        if (device) {
            alcCloseDevice(device);
        }
    }

    void setMusicVolume(float volume) {
        musicVolume = volume;
        alSourcef(musicSource, AL_GAIN, musicVolume);
    }

    void setSoundVolume(float volume) {
        soundVolume = volume;
    }

    float getMusicVolume() { return musicVolume; }
    float getSoundVolume() { return soundVolume; }

    void addMusicTrack(const std::string& filepath) {
        musicTracks.push_back(filepath);
    }

    static bool loadOGG(const std::string& path, ALuint& buffer, ALenum& format, ALsizei& freq, std::vector<short>& pcm) {
        int channels, sampleRate;
        short* output;
        int sampleCount = stb_vorbis_decode_filename(path.c_str(), &channels, &sampleRate, &output);
        if (sampleCount <= 0) {
            std::cerr << "[Audio] Failed to load: " << path << "\n";
            return false;
        }

        pcm.assign(output, output + (sampleCount * channels));
        free(output);

        format = (channels == 1) ? AL_FORMAT_MONO16 : AL_FORMAT_STEREO16;
        freq = sampleRate;

        alBufferData(buffer, format, pcm.data(), pcm.size() * sizeof(short), freq);
        return true;
    }

    void playRandomMusic() {
        if (musicTracks.empty()) return;

        std::uniform_int_distribution<int> dist(0, static_cast<int>(musicTracks.size()) - 1);
        int index = dist(rng);
        const std::string& path = musicTracks[index];

        std::vector<short> pcm;
        ALenum format;
        ALsizei freq;

        if (musicBuffer != 0) {
            alDeleteBuffers(1, &musicBuffer);
        }

        alGenBuffers(1, &musicBuffer);
        if (!loadOGG(path, musicBuffer, format, freq, pcm)) return;

        alSourcei(musicSource, AL_BUFFER, musicBuffer);
        alSourcef(musicSource, AL_GAIN, musicVolume);
        alSourcei(musicSource, AL_LOOPING, AL_FALSE);
        alSourcePlay(musicSource);

        std::uniform_real_distribution<float> nextDelay(30.0f, 75.0f);
        nextTrackDelay = nextDelay(rng);
        timeSinceLastTrack = 0.0f;
    }

    void playSoundEffect(const std::string& path) {
        ALuint buffer, source;
        alGenBuffers(1, &buffer);
        alGenSources(1, &source);

        std::vector<short> pcm;
        ALenum format;
        ALsizei freq;

        if (!loadOGG(path, buffer, format, freq, pcm)) {
            alDeleteBuffers(1, &buffer);
            alDeleteSources(1, &source);
            return;
        }

        alSourcei(source, AL_BUFFER, buffer);
        alSourcef(source, AL_GAIN, soundVolume);
        alSourcePlay(source);

        ALint state;
        do {
            alGetSourcei(source, AL_SOURCE_STATE, &state);
        } while (state == AL_PLAYING);

        alDeleteSources(1, &source);
        alDeleteBuffers(1, &buffer);
    }

    void update(float deltaTime) {
        ALint state;
        alGetSourcei(musicSource, AL_SOURCE_STATE, &state);
        if (state != AL_PLAYING) {
            timeSinceLastTrack += deltaTime;
            if (timeSinceLastTrack >= nextTrackDelay) {
                playRandomMusic();
            }
        }
    }

}
