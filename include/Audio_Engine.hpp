#ifndef AUDIO_ENGINE_HPP
#define AUDIO_ENGINE_HPP
#include <iostream>
#include <SDL2/SDL_mixer.h>

class AudioEngine {

public:
    SDL_AudioDeviceID dev;
    SDL_AudioSpec spec;
    AudioEngine();
    ~AudioEngine();

};

#endif