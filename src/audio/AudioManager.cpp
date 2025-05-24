#include "audio/AudioManager.h"

#include <vorbis/vorbisfile.h>
#define DR_WAV_IMPLEMENTATION
#include "utils/dr_wav.h"

#include <AL/al.h>
#include <AL/alc.h>
#include <string>
#include <vector>
#include <random>
#include <chrono>
#include <iostream>
#include <cctype>
#include <thread>
#include <atomic>

#include "core/game/Game.h"

static std::atomic<bool> trackPending = false;

namespace AudioManager {

    static ALCdevice* device = nullptr;
    static ALCcontext* context = nullptr;

    static float musicVolume = 1.0f;
    static float soundVolume = 1.0f;

    static float fadeDuration = 3.0f;
    static float currentGain = 0.0f;
    static float fadeTimer = 0.0f;
    static bool isFadingIn = false;
    static bool isFadingOut = false;

    static std::vector<std::string> musicTracks;
    static ALuint musicSource = 0;
    static ALuint musicBuffer = 0;

    static float timeSinceLastTrack = 0.0f;
    static float nextTrackDelay = 30.0f;

    static std::atomic<bool> shutdownAudioThread = false;
    static std::thread audioThread;
    static std::vector<std::thread> soundThreads;

    constexpr int MAX_SOUND_SOURCES = 32;
    static std::vector<ALuint> sourcePool;
    static std::vector<bool> sourceInUse;

    static ThreadSafeQueue<std::string> trackRequestQueue;

    std::default_random_engine rng(std::chrono::system_clock::now().time_since_epoch().count());

    void startAudioThread();
    static std::string getFileExtension(const std::string& path);
    static bool loadAudioFile(const std::string& path, ALuint& buffer, ALenum& format, ALsizei& freq, std::vector<short>& pcm);

    void init() {
        device = alcOpenDevice(nullptr);
        if (!device) {
            std::cerr << "Failed to open audio device.\n";
            return;
        }

        context = alcCreateContext(device, nullptr);
        if (!context || !alcMakeContextCurrent(context)) {
            std::cerr << "Failed to set audio context.\n";
            return;
        }

        alDistanceModel(AL_LINEAR_DISTANCE_CLAMPED);

        alGenSources(1, &musicSource);

        sourcePool.resize(MAX_SOUND_SOURCES);
        sourceInUse.resize(MAX_SOUND_SOURCES, false);
        alGenSources(MAX_SOUND_SOURCES, sourcePool.data());

        startAudioThread();
    }

    void shutdown() {
        shutdownAudioThread = true;
        trackRequestQueue.stop();

        if (audioThread.joinable())
            audioThread.join();

        alSourceStop(musicSource);
        alDeleteSources(1, &musicSource);
        alDeleteBuffers(1, &musicBuffer);

        alSourceStopv(sourcePool.size(), sourcePool.data());
        alDeleteSources(static_cast<ALsizei>(sourcePool.size()), sourcePool.data());
        sourcePool.clear();
        sourceInUse.clear();

        for (auto& t : soundThreads) {
            if (t.joinable()) t.join();
        }
        soundThreads.clear();

        if (context) {
            alcMakeContextCurrent(nullptr);
            alcDestroyContext(context);
        }

        if (device) {
            alcCloseDevice(device);
        }

        trackPending = false;
    }


    void setMusicVolume(float volume) {
        musicVolume = volume;
    }

    void setSoundVolume(float volume) {
        soundVolume = volume;
    }

    float getMusicVolume() { return musicVolume; }
    float getSoundVolume() { return soundVolume; }

    void addMusicTrack(const std::string& filepath) {
        musicTracks.push_back(filepath);
    }

    void requestNextTrack() {
        if (!trackPending && !musicTracks.empty()) {
            std::uniform_int_distribution<int> dist(0, static_cast<int>(musicTracks.size()) - 1);
            trackRequestQueue.push(musicTracks[dist(rng)]);
            trackPending = true;
        }
    }

    void startAudioThread() {
        audioThread = std::thread([] {
            std::string nextPath;
            std::string pendingPath;
            bool hasPending = false;

            std::chrono::steady_clock::time_point lastTime = std::chrono::steady_clock::now();

            while (!shutdownAudioThread.load()) {
                auto now = std::chrono::steady_clock::now();
                float deltaSeconds = std::chrono::duration<float>(now - lastTime).count();
                lastTime = now;

                if (!hasPending) {
                    if (trackRequestQueue.tryPop(pendingPath)) {
                        hasPending = true;
                    }
                }

                ALint state;
                alGetSourcei(musicSource, AL_SOURCE_STATE, &state);

                if (hasPending && state == AL_PLAYING && !isFadingOut) {
                    isFadingOut = true;
                    fadeTimer = 0.0f;
                    lastTime = std::chrono::steady_clock::now();
                }

                if (hasPending && state != AL_PLAYING && !isFadingIn) {
                    std::vector<short> pcm;
                    ALenum format;
                    ALsizei freq;

                    if (musicBuffer != 0) {
                        alSourcei(musicSource, AL_BUFFER, 0);
                        alDeleteBuffers(1, &musicBuffer);
                        musicBuffer = 0;
                    }

                    alGenBuffers(1, &musicBuffer);
                    if (!loadAudioFile(pendingPath, musicBuffer, format, freq, pcm)) {
                        hasPending = false;
                        trackPending = false;
                        continue;
                    }

                    alBufferData(musicBuffer, format, pcm.data(),
                                 static_cast<ALsizei>(pcm.size() * sizeof(short)), freq);

                    currentGain = 0.0f;
                    fadeTimer = 0.0f;
                    isFadingIn = true;
                    lastTime = std::chrono::steady_clock::now();

                    alSourcei(musicSource, AL_BUFFER, musicBuffer);
                    alSourcef(musicSource, AL_GAIN, currentGain);
                    alSourcei(musicSource, AL_LOOPING, AL_FALSE);
                    alSourcePlay(musicSource);

                    std::cout << "Now playing: " << pendingPath << "\n";

                    std::uniform_real_distribution<float> nextDelay(30.0f, 75.0f);
                    nextTrackDelay = nextDelay(rng);
                    timeSinceLastTrack = 0.0f;
                    trackPending = false;
                    hasPending = false;
                }

                if (isFadingIn) {
                    fadeTimer += deltaSeconds * 0.25f;
                    float t = std::min(fadeTimer / fadeDuration, 1.0f);
                    currentGain = t * musicVolume;
                    alSourcef(musicSource, AL_GAIN, currentGain);

                    if (t >= 1.0f) {
                        isFadingIn = false;
                    }
                }

                std::this_thread::sleep_for(std::chrono::milliseconds(50));
            }
        });
    }

    static std::string getFileExtension(const std::string& path) {
        auto dot = path.find_last_of('.');
        if (dot == std::string::npos) return "";
        std::string ext = path.substr(dot + 1);
        for (auto& c : ext) c = static_cast<char>(std::tolower(c));
        return ext;
    }

    static bool loadAudioFile(const std::string& path, ALuint& buffer, ALenum& format, ALsizei& freq, std::vector<short>& pcm) {
        std::string ext = getFileExtension(path);

        if (ext == "wav") {
            unsigned int channels, sampleRate;
            drwav_uint64 frameCount;
            short* data = drwav_open_file_and_read_pcm_frames_s16(path.c_str(), &channels, &sampleRate, &frameCount, nullptr);
            if (!data) {
                std::cerr << "Failed to load WAV: " << path << "\n";
                return false;
            }

            pcm.assign(data, data + frameCount * channels);
            drwav_free(data, nullptr);

            format = (channels == 1) ? AL_FORMAT_MONO16 : AL_FORMAT_STEREO16;
            freq = sampleRate;
            return true;

        } else if (ext == "ogg") {
            FILE* file = fopen(path.c_str(), "rb");
            if (!file) {
                std::cerr << "Failed to open OGG file: " << path << "\n";
                return false;
            }

            OggVorbis_File vf;
            if (ov_open(file, &vf, nullptr, 0) < 0) {
                std::cerr << "Invalid OGG stream: " << path << "\n";
                fclose(file);
                return false;
            }

            vorbis_info* vi = ov_info(&vf, -1);
            format = (vi->channels == 1) ? AL_FORMAT_MONO16 : AL_FORMAT_STEREO16;
            freq = vi->rate;

            char temp[4096];
            int bitstream;
            long bytesRead;
            do {
                bytesRead = ov_read(&vf, temp, sizeof(temp), 0, 2, 1, &bitstream);
                if (bytesRead > 0) {
                    pcm.insert(pcm.end(), reinterpret_cast<short*>(temp), reinterpret_cast<short*>(temp + bytesRead));
                }
            } while (bytesRead > 0);

            ov_clear(&vf);
            return true;
        }

        std::cerr << "Unsupported audio file extension: " << ext << "\n";
        return false;
    }

    void update(float deltaTime) {
        ALint state;
        alGetSourcei(musicSource, AL_SOURCE_STATE, &state);

        if (state != AL_PLAYING) {
            timeSinceLastTrack += deltaTime;

            float minDelay = 30.0f;
            float maxDelay = 330.0f;

            if (timeSinceLastTrack >= maxDelay) {
                requestNextTrack();
            } else if (timeSinceLastTrack >= minDelay) {
                std::uniform_real_distribution<float> chance(0.0f, 1.0f);
                if (chance(rng) < 0.00008f) {
                    requestNextTrack();
                }
            }
        }
    }

    void AudioManager::playBlockSound(BLOCKTYPE blockType, const glm::vec3& soundPos, const glm::vec3& playerPos, const std::string& soundType) {
        float maxDistance = 16.0f;
        float dist = glm::distance(playerPos, soundPos);
        float falloffPower = 1.5f;
        float volumeMultiplier = pow(1.0f - glm::clamp(dist / maxDistance, 0.0f, 1.0f), falloffPower);

        if (volumeMultiplier <= 0.01f) return;

        std::string path = Game::instance().getBasePath() + "/assets/sounds/effects/";
        path += blockTypeToString(blockType);
        path += "/" + soundType + ".wav";

        playSoundEffect(path, soundVolume * volumeMultiplier, &soundPos);
    }

    void AudioManager::playSoundEffect(const std::string& path, float gain, const glm::vec3* position) {
        std::vector<short> pcm;
        ALenum format;
        ALsizei freq;

        ALuint buffer;
        alGenBuffers(1, &buffer);

        if (!loadAudioFile(path, buffer, format, freq, pcm)) {
            alDeleteBuffers(1, &buffer);
            return;
        }

        int idx = getFreeSourceIndex();
        if (idx == -1) {
            alDeleteBuffers(1, &buffer);
            return;
        }

        ALuint source = sourcePool[idx];

        alSourceStop(source);
        alSourcei(source, AL_BUFFER, 0);
        alBufferData(buffer, format, pcm.data(), static_cast<ALsizei>(pcm.size() * sizeof(short)), freq);
        alSourcei(source, AL_BUFFER, buffer);
        alSourcef(source, AL_GAIN, gain);
        std::uniform_real_distribution<float> pitchRange(0.9f, 1.1f);
        alSourcef(source, AL_PITCH, pitchRange(rng));
        alSourcef(source, AL_REFERENCE_DISTANCE, 2.0f);
        alSourcef(source, AL_MAX_DISTANCE, 16.0f);
        alSourcef(source, AL_ROLLOFF_FACTOR, 1.0f);

        if (position) {
            alSource3f(source, AL_POSITION, position->x, position->y, position->z);
        }

        alSourcePlay(source);

        ALint playState;
        alGetSourcei(source, AL_SOURCE_STATE, &playState);
        if (playState != AL_PLAYING) {
            std::cerr << "Failed to play sound: " << path << "\n";
            alSourcei(source, AL_BUFFER, 0);
            alDeleteBuffers(1, &buffer);
            sourceInUse[idx] = false;
            return;
        }

        soundThreads.emplace_back([buffer, idx] {
            ALint state;
            do {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                alGetSourcei(sourcePool[idx], AL_SOURCE_STATE, &state);
            } while (state == AL_PLAYING);
            alSourcei(sourcePool[idx], AL_BUFFER, 0);
            alDeleteBuffers(1, &buffer);
            sourceInUse[idx] = false;
        });

        if (soundThreads.size() > 64) {
            for (auto& t : soundThreads) {
                if (t.joinable()) t.join();
            }
            soundThreads.clear();
        }
    }

    int getFreeSourceIndex() {
        for (int i = 0; i < MAX_SOUND_SOURCES; ++i) {
            ALint state;
            alGetSourcei(sourcePool[i], AL_SOURCE_STATE, &state);
            if (state != AL_PLAYING) {
                sourceInUse[i] = true;
                return i;
            }
        }
        return -1;
    }
}
