#include "audio/AudioManager.h"

#include <vorbis/vorbisfile.h>
#define DR_WAV_IMPLEMENTATION
#include "utils/dr_wav.h"

#include <AL/al.h>
#include <AL/alc.h>
#include <string>
#include <vector>
#include <array>
#include <random>
#include <chrono>
#include <iostream>
#include <cctype>
#include <thread>
#include <atomic>
#include <filesystem>
namespace fs = std::filesystem;

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
    static std::vector<std::thread> soundWorkers;
    static std::thread cleanupThread;

    constexpr int MAX_SOUND_SOURCES = 32;
    static std::vector<ALuint> sourcePool;
    static std::array<std::atomic<bool>, MAX_SOUND_SOURCES> sourceInUse;

    constexpr int NUM_SOUND_WORKERS = 4;

    static ThreadSafeQueue<std::string> trackRequestQueue;
    static ThreadSafeQueue<ALuint> allocatedSoundBuffers;

    struct QueuedSound {
        std::string path;
        float gain;
        glm::vec3 position;
        bool hasPosition;
    };

    static ThreadSafeQueue<QueuedSound> soundRequestQueue;
    static ThreadSafeQueue<ALuint> bufferCleanupQueue;
    static std::array<std::atomic<bool>, MAX_SOUND_SOURCES> sourceFinished;

    std::default_random_engine rng(std::chrono::system_clock::now().time_since_epoch().count());

    static std::string getFileExtension(const std::string& path) {
        size_t dot = path.find_last_of('.');
        if (dot == std::string::npos) return "";
        std::string ext = path.substr(dot + 1);
        for (char& c : ext) c = static_cast<char>(std::tolower(c));
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
        }

        if (ext == "ogg") {
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
        alGenSources(MAX_SOUND_SOURCES, sourcePool.data());
        for (auto& flag : sourceInUse) flag.store(false);
        for (auto& flag : sourceFinished) flag.store(true);

        startAudioThread();
        startSoundThread();

        cleanupThread = std::thread([] {
            while (!shutdownAudioThread.load() || !bufferCleanupQueue.empty()) {
                for (int i = 0; i < MAX_SOUND_SOURCES; ++i) {
                    if (!sourceInUse[i]) continue;
                
                    ALint state;
                    alGetSourcei(sourcePool[i], AL_SOURCE_STATE, &state);
                    if (state != AL_PLAYING && state != AL_PAUSED && !sourceFinished[i].exchange(true)) {
                        alSourcei(sourcePool[i], AL_BUFFER, 0);
                        sourceInUse[i] = false;
                    }
                }
            
                ALuint buffer;
                while (bufferCleanupQueue.tryPop(buffer)) {
                    alDeleteBuffers(1, &buffer);
                }
            
                std::this_thread::sleep_for(std::chrono::milliseconds(25));
            }
        });
    }

    void shutdown() {
        shutdownAudioThread = true;
        trackRequestQueue.stop();
        soundRequestQueue.stop();
        bufferCleanupQueue.stop();

        if (audioThread.joinable()) audioThread.join();
        for (auto& worker : soundWorkers) {
            if (worker.joinable()) worker.join();
        }
        soundWorkers.clear();

        if (cleanupThread.joinable()) cleanupThread.join();

        for (int i = 0; i < MAX_SOUND_SOURCES; ++i) {
            ALuint src = sourcePool[i];
            alSourceStop(src);             
            alSourcei(src, AL_BUFFER, 0);
        }

        alSourceStop(musicSource);
        alSourcei(musicSource, AL_BUFFER, 0);
        alDeleteSources(1, &musicSource);
        if (musicBuffer) {
            alDeleteBuffers(1, &musicBuffer);
            musicBuffer = 0;
        }

        alDeleteSources(static_cast<ALsizei>(sourcePool.size()), sourcePool.data());
        sourcePool.clear();

        ALuint leftover;
        while (bufferCleanupQueue.tryPop(leftover)) {
            alDeleteBuffers(1, &leftover);
            ALenum err = alGetError();
            if (err != AL_NO_ERROR) {
                std::cerr << "Failed to delete buffer " << leftover << " (OpenAL error: 0x" << std::hex << err << ")\n";
            }
        }

        ALuint buf;
        while (allocatedSoundBuffers.tryPop(buf)) {
            alDeleteBuffers(1, &buf);
        }

        if (context) {
            alcMakeContextCurrent(nullptr);
            alcDestroyContext(context);
            context = nullptr;
        }

        if (device) {
            alcCloseDevice(device);
            device = nullptr;
        }

        trackPending = false;
    }

    void setMusicVolume(float volume) { musicVolume = volume; }
    void setSoundVolume(float volume) { soundVolume = volume; }
    float getMusicVolume() { return musicVolume; }
    float getSoundVolume() { return soundVolume; }

    void loadMusicTracks(const std::string& directoryPath) {
        for (const auto& entry : fs::directory_iterator(directoryPath)) {
            if (!entry.is_regular_file()) continue;

            std::string path = entry.path().string();
            std::string ext = entry.path().extension().string();

            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
            if (ext == ".ogg" || ext == ".wav") {
                addMusicTrack(path);
            }
        }
    }

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

#undef min
#include <algorithm>

    void startAudioThread() {
        audioThread = std::thread([] {
            std::string pendingPath;
            bool hasPending = false;
            auto lastTime = std::chrono::steady_clock::now();

            while (!shutdownAudioThread.load() || !bufferCleanupQueue.empty()) {
                auto now = std::chrono::steady_clock::now();
                float delta = std::chrono::duration<float>(now - lastTime).count();
                lastTime = now;

                if (!hasPending && trackRequestQueue.tryPop(pendingPath)) {
                    hasPending = true;
                }

                ALint state;
                alGetSourcei(musicSource, AL_SOURCE_STATE, &state);

                if (hasPending && state == AL_PLAYING && !isFadingOut) {
                    isFadingOut = true;
                    fadeTimer = 0.0f;
                }

                if (hasPending && state != AL_PLAYING && !isFadingIn) {
                    std::vector<short> pcm;
                    ALenum format;
                    ALsizei freq;

                    if (musicBuffer) {
                        alSourcei(musicSource, AL_BUFFER, 0);
                        alDeleteBuffers(1, &musicBuffer);
                        musicBuffer = 0;
                    }

                    alGenBuffers(1, &musicBuffer);
                    if (!loadAudioFile(pendingPath, musicBuffer, format, freq, pcm)) {
                        trackPending = false;
                        hasPending = false;
                        continue;
                    }

                    alBufferData(musicBuffer, format, pcm.data(), pcm.size() * sizeof(short), freq);
                    currentGain = 0.0f;
                    fadeTimer = 0.0f;
                    isFadingIn = true;

                    alSourcei(musicSource, AL_BUFFER, musicBuffer);
                    alSourcef(musicSource, AL_GAIN, currentGain);
                    alSourcei(musicSource, AL_LOOPING, AL_FALSE);
                    alSourcePlay(musicSource);

                    trackPending = false;
                    hasPending = false;
                }

                if (isFadingIn) {
                    fadeTimer += delta * 0.15f;
                    float t = std::min(fadeTimer / fadeDuration, 1.0f);
                    currentGain = t * musicVolume;
                    alSourcef(musicSource, AL_GAIN, currentGain);
                    if (t >= 1.0f) isFadingIn = false;
                }

                std::this_thread::sleep_for(std::chrono::milliseconds(50));
            }
        });
    }

    void startSoundThread() {
        for (int i = 0; i < NUM_SOUND_WORKERS; ++i) {
            soundWorkers.emplace_back([] {
                while (!shutdownAudioThread.load()) {
                    QueuedSound req;
                    if (!soundRequestQueue.waitPop(req)) break;

                    std::vector<short> pcm;
                    ALenum format;
                    ALsizei freq;
                    ALuint buffer;
                    alGenBuffers(1, &buffer);

                    if (!loadAudioFile(req.path, buffer, format, freq, pcm)) {
                        alDeleteBuffers(1, &buffer);
                        continue;
                    }

                    int idx = getFreeSourceIndex();
                    if (idx == -1) {
                        alDeleteBuffers(1, &buffer);
                        continue;
                    }

                    allocatedSoundBuffers.push(buffer);

                    ALuint src = sourcePool[idx];
                    alSourcei(src, AL_BUFFER, 0);
                    alBufferData(buffer, format, pcm.data(), pcm.size() * sizeof(short), freq);
                    alSourcei(src, AL_BUFFER, buffer);
                    alSourcef(src, AL_GAIN, req.gain);
                    float pitchMod = 0.0f;
                    if (req.path.find("place") != std::string::npos) {
                        pitchMod = 0.05;
                    } else if (req.path.find("break") != std::string::npos) {
                        pitchMod = -0.05;
                    }
                    alSourcef(src, AL_PITCH, std::uniform_real_distribution<float>(0.9f, 1.1f)(rng) + pitchMod);
                    alSourcef(src, AL_REFERENCE_DISTANCE, 2.0f);
                    alSourcef(src, AL_MAX_DISTANCE, 16.0f);
                    alSourcef(src, AL_ROLLOFF_FACTOR, 1.0f);

                    if (req.hasPosition)
                        alSource3f(src, AL_POSITION, req.position.x, req.position.y, req.position.z);

                    sourceInUse[idx] = true;
                    sourceFinished[idx] = false;
                    alSourcePlay(src);
                }
            });
        }
    }

    int getFreeSourceIndex() {
        for (int i = 0; i < MAX_SOUND_SOURCES; ++i) {
            ALint state;
            alGetSourcei(sourcePool[i], AL_SOURCE_STATE, &state);
            if ((state != AL_PLAYING && state != AL_PAUSED) && !sourceInUse[i].exchange(true))
                return i;
        }
        return -1;
    }

    void update(float deltaTime) {
        ALint state;
        alGetSourcei(musicSource, AL_SOURCE_STATE, &state);

        if (state != AL_PLAYING) {
            timeSinceLastTrack += deltaTime;
            if (timeSinceLastTrack >= 630.0f) {
                requestNextTrack();
            } else if (timeSinceLastTrack >= 30.0f) {
                std::uniform_real_distribution<float> chance(0.0f, 1.0f);
                if (chance(rng) < 0.00008f) {
                    requestNextTrack();
                }
            }
        }
    }

    void playBlockSound(BLOCKTYPE blockType, const glm::vec3& soundPos, const glm::vec3& playerPos, const std::string& soundType) {
        float maxDistance = 16.0f;
        float dist = glm::distance(playerPos, soundPos);
        float falloffPower = 1.5f;
        float volumeMultiplier = pow(1.0f - glm::clamp(dist / maxDistance, 0.0f, 1.0f), falloffPower);
        if (volumeMultiplier <= 0.01f) return;

        std::string path = Game::instance().getBasePath() + "/assets/sounds/effects/";
        path += blockTypeToString(blockType);
        path += "/" + soundType + ".wav";

        soundRequestQueue.push({ path, soundVolume * volumeMultiplier, soundPos, true });
    }
}
