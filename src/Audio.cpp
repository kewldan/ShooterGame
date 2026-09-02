#include "Audio.h"

#include <plog/Log.h>
#include <random>
#include <string>
#include <unordered_map>

// miniaudio is header-only; this is its one implementation unit. Only WAV decoding is needed.
#define MINIAUDIO_IMPLEMENTATION
#define MA_NO_FLAC
#define MA_NO_MP3
#define MA_NO_ENCODING
#define MA_NO_GENERATION
#ifdef _MSC_VER
#pragma warning(push, 1)
#endif
#include "miniaudio.h"
#ifdef _MSC_VER
#pragma warning(pop)
#endif

namespace {
    // How many times one clip may play at once; the oldest voice is reused after that.
    constexpr int VOICES = 4;
}

struct Audio::Impl {
    struct Clip {
        ma_sound voices[VOICES]{};
        int initialized = 0; // voices[0..initialized) are valid
        int next = 0;        // round robin over the voices when all of them are busy
    };

    ma_engine engine{};
    bool ready = false;
    float masterVolume = 1.f;
    std::unordered_map<std::string, Clip> clips;
    std::mt19937 rng{std::random_device{}()};

    Impl() {
        ma_engine_config config = ma_engine_config_init();
        config.listenerCount = 1;
        const ma_result result = ma_engine_init(&config, &engine);
        if (result != MA_SUCCESS) {
            PLOGW << "Audio disabled: ma_engine_init failed (" << ma_result_description(result) << ")";
            return;
        }
        ready = true;
        PLOGI << "Audio: " << ma_engine_get_channels(&engine) << " channels @ "
              << ma_engine_get_sample_rate(&engine) << " Hz";
    }

    ~Impl() {
        for (auto &[name, clip]: clips) {
            for (int i = 0; i < clip.initialized; i++) {
                ma_sound_uninit(&clip.voices[i]);
            }
        }
        if (ready) {
            ma_engine_uninit(&engine);
        }
    }
};

Audio::Audio() : impl(std::make_unique<Impl>()) {
}

Audio::~Audio() = default;

bool Audio::load(const char *name) {
    if (!impl->ready) {
        return false;
    }
    if (impl->clips.contains(name)) {
        return true;
    }
    // Same lookup rule as the meshes: data/ in the working directory (never embedded as a resource).
    const std::string path = std::string("data/sounds/") + name + ".wav";
    Impl::Clip &clip = impl->clips[name];
    for (int i = 0; i < VOICES; i++) {
        // DECODE keeps the PCM in memory (short clips, started often); the resource manager shares the
        // decoded data between the voices of one file. Pitch shifting needs the resampler, so
        // MA_SOUND_FLAG_NO_PITCH must not be set.
        const ma_result result = ma_sound_init_from_file(&impl->engine, path.c_str(),
                                                         MA_SOUND_FLAG_DECODE | MA_SOUND_FLAG_NO_SPATIALIZATION,
                                                         nullptr, nullptr, &clip.voices[i]);
        if (result != MA_SUCCESS) {
            PLOGE << "Failed to load sound [" << path << "]: " << ma_result_description(result);
            break;
        }
        clip.initialized++;
    }
    if (clip.initialized == 0) {
        impl->clips.erase(name);
        return false;
    }
    return true;
}

void Audio::play(const char *name, float volume, float pitchVariation) {
    if (!impl->ready) {
        return;
    }
    const auto it = impl->clips.find(name);
    if (it == impl->clips.end()) {
        PLOGW << "Sound [" << name << "] was not loaded";
        return;
    }
    Impl::Clip &clip = it->second;
    // Prefer a voice that is not playing; otherwise steal the next one in the ring.
    int voice = -1;
    for (int i = 0; i < clip.initialized; i++) {
        if (!ma_sound_is_playing(&clip.voices[i])) {
            voice = i;
            break;
        }
    }
    if (voice < 0) {
        voice = clip.next;
        clip.next = (clip.next + 1) % clip.initialized;
    }
    ma_sound &sound = clip.voices[voice];
    float pitch = 1.f;
    if (pitchVariation > 0.f) {
        std::uniform_real_distribution<float> spread(1.f - pitchVariation, 1.f + pitchVariation);
        pitch = spread(impl->rng);
    }
    ma_sound_stop(&sound);
    ma_sound_seek_to_pcm_frame(&sound, 0);
    ma_sound_set_volume(&sound, volume);
    ma_sound_set_pitch(&sound, pitch);
    ma_sound_start(&sound);
}

void Audio::setMasterVolume(float volume) {
    impl->masterVolume = volume;
    if (impl->ready) {
        ma_engine_set_volume(&impl->engine, volume);
    }
}

float Audio::getMasterVolume() const {
    return impl->masterVolume;
}

bool Audio::isReady() const {
    return impl->ready;
}
