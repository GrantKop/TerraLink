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
                std::cerr << "[Audio] Failed to load WAV: " << path << "\n";
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
                std::cerr << "[Audio] Failed to open OGG file: " << path << "\n";
                return false;
            }

            OggVorbis_File vf;
            if (ov_open(file, &vf, nullptr, 0) < 0) {
                std::cerr << "[Audio] Invalid OGG stream: " << path << "\n";
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

        std::cerr << "[Audio] Unsupported file extension: " << ext << "\n";
        return false;
    }

    void playRandomMusic() {
        if (musicTracks.empty()) return;

        std::uniform_int_distribution<int> dist(0, static_cast<int>(musicTracks.size()) - 1);
        const std::string& path = musicTracks[dist(rng)];

        std::vector<short> pcm;
        ALenum format;
        ALsizei freq;

        if (musicBuffer != 0)
            alDeleteBuffers(1, &musicBuffer);

        alGenBuffers(1, &musicBuffer);
        if (!loadAudioFile(path, musicBuffer, format, freq, pcm)) return;

        alBufferData(musicBuffer, format, pcm.data(), static_cast<ALsizei>(pcm.size() * sizeof(short)), freq);
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

        if (!loadAudioFile(path, buffer, format, freq, pcm)) {
            alDeleteSources(1, &source);
            alDeleteBuffers(1, &buffer);
            return;
        }

        alBufferData(buffer, format, pcm.data(), static_cast<ALsizei>(pcm.size() * sizeof(short)), freq);
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
