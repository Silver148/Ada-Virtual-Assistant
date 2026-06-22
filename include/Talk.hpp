#ifndef TALK_HPP
#define TALK_HPP
#include <iostream>
#include <string>
#include <thread>
#include <unistd.h>
#include <cstdio>
#include <regex>

#if defined(__linux__) || defined(__unix__)
#include <signal.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include <fcntl.h>
#include <pthread.h>
#include <SDL2/SDL_mixer.h>
#endif

#if defined(_WIN32) || defined(_WIN64)
    #include <windows.h>
    #include <windows.h>
    #include <sapi.h>
    #include <sphelper.h>
#endif

#if defined(__linux__) || defined(__unix__)
class PiperBridge {
private:
    int writePipe[2]; // Pipe descriptor for writing to Piper's stdin
    int readPipe; // Pipe descriptor for reading audio data from Piper
    pid_t piperProcessId;
    SDL_AudioDeviceID dev;

public:

    void Stop();
    void Resume();
    void Speak(const std::string& message);
    void StreamAudio(SDL_AudioDeviceID dev);
    PiperBridge(const std::string& modelPath, SDL_AudioDeviceID dev);
    ~PiperBridge();
};
#endif

class AdaVoice{

private:
#if defined(_WIN32) || defined(_WIN64)
    ISpVoice* pVoice = nullptr;
    HRESULT hr;
#else
    PiperBridge* piper = nullptr;
    SDL_AudioDeviceID dev;
#endif

public:
    
    void TalkAda(std::string message);

#if defined(_WIN32) || defined(_WIN64)
    
    AdaVoice();
#else
    AdaVoice(SDL_AudioDeviceID dev);
#endif

    ~AdaVoice();
    std::string CleanTextForTalk(const std::string& message);
    void ShutUpAda();

};

#endif