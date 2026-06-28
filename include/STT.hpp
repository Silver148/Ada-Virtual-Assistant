#ifndef STT_HPP
#define STT_HPP
#include "vosk_api.h"
#include "json.hpp"
#if defined(__linux__) || defined(__unix__)
#include "main.hpp"
#endif
#include <iostream>
#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>
#include <mutex>

using namespace nlohmann;

class STT{

private:
    VoskModel* model;
    SDL_AudioDeviceID mic;
    SDL_AudioSpec desiredSpec, obtainedSpec;

    json MicText;
    std::mutex mtx;

public:
    VoskRecognizer* recognizer;

    STT();
    ~STT();

    bool Init();
    bool ResetMic();
    std::string TextFromMic();
    void PushText(const std::string& raw_json);
    void StartListening();
    void StopListening();

};

#endif