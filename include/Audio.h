#pragma once

#include <memory>

// Sound playback on top of miniaudio (one ma_engine on the default output device). Clips are WAV files
// in data/sounds/, decoded once into memory and kept in a few voices each so that a clip can overlap
// with itself (two quick shots). Without an output device the object is inert: play() is a no-op.
class Audio {
    struct Impl;
    std::unique_ptr<Impl> impl;

public:
    Audio();

    ~Audio();

    Audio(const Audio &) = delete;

    Audio &operator=(const Audio &) = delete;

    // Loads data/sounds/<name>.wav; returns false (and logs) when the file is missing or undecodable.
    bool load(const char *name);

    // Starts `name` (loaded with load()) at `volume` (0..1) with a random pitch factor in
    // [1 - pitchVariation, 1 + pitchVariation], so repeated footsteps and shots do not sound identical.
    void play(const char *name, float volume = 1.f, float pitchVariation = 0.f);

    void setMasterVolume(float volume);

    [[nodiscard]] float getMasterVolume() const;

    [[nodiscard]] bool isReady() const;
};
