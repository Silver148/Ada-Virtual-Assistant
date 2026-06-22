#include "Audio_Engine.hpp"

AudioEngine::AudioEngine() {
    SDL_AudioSpec desiredSpec;
    desiredSpec.freq = 22050;
    desiredSpec.format = AUDIO_S16SYS;
    desiredSpec.channels = 1;
    desiredSpec.samples = 2048;
    desiredSpec.callback = nullptr;

    dev = SDL_OpenAudioDevice(nullptr, 0, &desiredSpec, &spec, 0);
    if (dev == 0) {
        std::cerr << "Failed to open audio device: " << SDL_GetError() << std::endl;
    } else {
        SDL_PauseAudioDevice(dev, 0); // Start audio playback
    }
}

AudioEngine::~AudioEngine() {
    if (dev != 0) {
        SDL_CloseAudioDevice(dev);
    }
}