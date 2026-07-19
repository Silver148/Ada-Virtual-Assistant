#ifndef TALK_HPP
#define TALK_HPP
#include <iostream>
#include <string>
#include <thread>
#include <unistd.h>
#include <cstdio>
#include <regex>
#include <vector>
#include <algorithm>
#include <cctype>

#if defined(__linux__) || defined(__unix__)
#include <signal.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <pthread.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>

#include <atomic>
#include <memory>
#include <mutex>
#include <condition_variable>
#include <queue>

#include <fstream>
#include <iostream>

#include <picoapi.h>
#include "main.hpp"
#endif

#if defined(_WIN32) || defined(_WIN64)
    #include <windows.h>
    #include <windows.h>
    #include <sapi.h>
    #include <sphelper.h>
#endif

#if defined(__linux__) || defined(__unix__)
class PicoTTS {

private:

    const pico_Uint32 PICO_MEM_SIZE = 2500000;
    const pico_Uint32 OUT_BUF_SIZE = 1024;

    pico_System system = nullptr;
    pico_Resource textResource = nullptr;
    pico_Resource voiceResource = nullptr;
    pico_Engine engine = nullptr;
    std::unique_ptr<char[]> picoMemory;

    std::thread worker;
    std::mutex queueMutex;
    std::condition_variable queueCondition;
    std::queue<std::string> pendingMessages;
    std::atomic<bool> stopRequested{false};

    void WorkerLoop();
    void SpeakChunk(const std::string& message);
    std::vector<std::string> SplitTextIntoChunks(const std::string& text, size_t maxChars = 180) const;

public:
    PicoTTS(SDL_AudioDeviceID Dev);
    ~PicoTTS();
    void Speak(const std::string& message);

    SDL_AudioDeviceID dev;

};
#endif

class AdaVoice{

private:
#if defined(_WIN32) || defined(_WIN64)
    ISpVoice* pVoice = nullptr;
    HRESULT hr;
#else
    SDL_AudioDeviceID dev;
    PicoTTS picoTTS;
#endif

public:
    
    void TalkAda(std::string message);

#if defined(_WIN32) || defined(_WIN64)
    
    AdaVoice();
#else
    AdaVoice(SDL_AudioDeviceID Dev);
#endif

    ~AdaVoice();
    std::string CleanTextForTalk(const std::string& message);
    void ShutUpAda();

};

#endif