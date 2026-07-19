/*


Talk.cpp
*/

#include "Talk.hpp"

#if defined(__linux__) || defined(__unix__)
static const pico_Uint32 OUT_BUF_SIZE = 1024;

PicoTTS::PicoTTS(SDL_AudioDeviceID dev) {
    this->dev = dev;

    picoMemory = std::unique_ptr<char[]>(new char[PICO_MEM_SIZE]);
    system = nullptr;

    if (pico_initialize(picoMemory.get(), PICO_MEM_SIZE, &system) != PICO_OK) {
        std::cerr << "Error to init PicoTTS\n";
        SDL_CloseAudioDevice(dev);
        return;
    }

    textResource = nullptr;
    voiceResource = nullptr;
    auto ta_path = reinterpret_cast<const pico_Char*>("/usr/share/pico/lang/es-ES_ta.bin");
    auto lh_path = reinterpret_cast<const pico_Char*>("/usr/share/pico/lang/es-ES_zl0_sg.bin");

    if (pico_loadResource(system, ta_path, &textResource) != PICO_OK ||
        pico_loadResource(system, lh_path, &voiceResource) != PICO_OK) {
        std::cerr << "Error to load resources\n";
        pico_terminate(&system);
        SDL_CloseAudioDevice(dev);
        return;
    }

    pico_Retstring textResourceName;
    pico_Retstring voiceResourceName;
    pico_getResourceName(system, textResource, textResourceName);
    pico_getResourceName(system, voiceResource, voiceResourceName);

    auto voice_name = reinterpret_cast<const pico_Char*>("es-US");
    auto ta_res_name = reinterpret_cast<const pico_Char*>(textResourceName);
    auto lh_res_name = reinterpret_cast<const pico_Char*>(voiceResourceName);

    if (pico_createVoiceDefinition(system, voice_name) != PICO_OK ||
        pico_addResourceToVoiceDefinition(system, voice_name, ta_res_name) != PICO_OK ||
        pico_addResourceToVoiceDefinition(system, voice_name, lh_res_name) != PICO_OK) {

        std::cerr << "Error to associate voice resources.\n";
        pico_releaseVoiceDefinition(system, voice_name);
        pico_unloadResource(system, &textResource);
        pico_unloadResource(system, &voiceResource);
        pico_terminate(&system);
        SDL_CloseAudioDevice(dev);
        return;
    }

    if (pico_newEngine(system, voice_name, &engine) != PICO_OK) {
        std::cerr << "Error to create engine instance.\n";
        pico_releaseVoiceDefinition(system, voice_name);
        pico_unloadResource(system, &textResource);
        pico_unloadResource(system, &voiceResource);
        pico_terminate(&system);
        SDL_CloseAudioDevice(dev);
        return;
    }

    std::cout << "PicoTTS initialized successfully." << std::endl;
    worker = std::thread(&PicoTTS::WorkerLoop, this);
}

void PicoTTS::Speak(const std::string& message) {
    if (message.empty()) {
        return;
    }

    {
        std::lock_guard<std::mutex> lock(queueMutex);
        if (stopRequested) {
            return;
        }
        pendingMessages.push(message);
    }

    queueCondition.notify_one();
}

void PicoTTS::WorkerLoop() {
    while (true) {
        std::string message;
        {
            std::unique_lock<std::mutex> lock(queueMutex);
            queueCondition.wait(lock, [this] {
                return stopRequested || !pendingMessages.empty();
            });

            if (stopRequested && pendingMessages.empty()) {
                break;
            }

            message = std::move(pendingMessages.front());
            pendingMessages.pop();
        }

        if (!message.empty()) {
            SpeakChunk(message);
        }
    }
}

void PicoTTS::SpeakChunk(const std::string& message) {
    if (system == nullptr || engine == nullptr) {
        std::cerr << "PicoTTS is not initialized properly." << std::endl;
        return;
    }

    if (message.empty()) {
        return;
    }

    if (dev != 0) {
        SDL_PauseAudioDevice(dev, 0);
        SDL_ClearQueuedAudio(dev);
        SDL_Delay(20);
    }

    if (pico_resetEngine(engine, PICO_RESET_SOFT) != PICO_OK) {
        std::cerr << "Failed to reset Pico engine before speaking." << std::endl;
    }

    const auto chunks = SplitTextIntoChunks(message);
    for (const auto& chunk : chunks) {
        pico_Int16 bytesPut = 0;
        auto text_ptr = reinterpret_cast<const pico_Char*>(chunk.c_str());
        if (pico_putTextUtf8(engine, text_ptr, static_cast<pico_Int16>(chunk.size() + 1), &bytesPut) != PICO_OK) {
            std::cerr << "Failed to send text to Pico engine." << std::endl;
            return;
        }

        std::vector<pico_Char> outBuffer(OUT_BUF_SIZE);
        pico_Int16 bytesReceived = 0;
        pico_Int16 dataType = 0;
        pico_Status status = PICO_STEP_BUSY;

        while (status == PICO_STEP_BUSY) {
            status = pico_getData(engine, outBuffer.data(), OUT_BUF_SIZE, &bytesReceived, &dataType);
            if (bytesReceived > 0) {
                SDL_QueueAudio(dev, outBuffer.data(), bytesReceived);

                while (SDL_GetQueuedAudioSize(dev) > 32768) {
                    SDL_Delay(10);
                }
            }
        }
    }

    while (SDL_GetQueuedAudioSize(dev) > 0) {
        SDL_Delay(10);
    }
}

std::vector<std::string> PicoTTS::SplitTextIntoChunks(const std::string& text, size_t maxChars) const {
    std::vector<std::string> chunks;
    if (text.empty()) {
        return chunks;
    }

    size_t start = 0;
    while (start < text.size()) {
        while (start < text.size() && std::isspace(static_cast<unsigned char>(text[start]))) {
            ++start;
        }
        if (start >= text.size()) {
            break;
        }

        size_t end = std::min(text.size(), start + maxChars);
        size_t split = end;

        if (end < text.size()) {
            for (size_t i = end; i > start; --i) {
                const unsigned char ch = static_cast<unsigned char>(text[i - 1]);
                if (std::ispunct(ch) || std::isspace(ch)) {
                    split = i;
                    break;
                }
            }
        }

        if (split > start) {
            chunks.emplace_back(text.substr(start, split - start));
            start = split;
        } else {
            chunks.emplace_back(text.substr(start, end - start));
            start = end;
        }
    }

    return chunks;
}

PicoTTS::~PicoTTS() {
    stopRequested = true;
    queueCondition.notify_all();
    if (worker.joinable()) {
        worker.join();
    }

    if (engine != nullptr && system != nullptr) {
        pico_disposeEngine(system, &engine);
        engine = nullptr;
    }

    if (system != nullptr) {
        auto voice_name = reinterpret_cast<const pico_Char*>("es-ES");
        pico_releaseVoiceDefinition(system, voice_name);

        if (textResource != nullptr) {
            pico_unloadResource(system, &textResource);
            textResource = nullptr;
        }
        if (voiceResource != nullptr) {
            pico_unloadResource(system, &voiceResource);
            voiceResource = nullptr;
        }

        pico_terminate(&system);
        system = nullptr;
    }
}


#endif


#if defined(_WIN32) || defined(_WIN64)
AdaVoice::AdaVoice(){

    ::CoInitialize(NULL);
    
    hr = ::CoCreateInstance(CLSID_SpVoice, NULL, CLSCTX_ALL, IID_ISpVoice, (void**)&pVoice);
    
    if (SUCCEEDED(hr) && pVoice != NULL) {
        IEnumSpObjectTokens* pEnum = NULL;
        if (SUCCEEDED(SpEnumTokens(SPCAT_VOICES, L"Gender=Female;Name=Microsoft Sabina", NULL, &pEnum))) {
            ISpObjectToken* pToken = NULL;
            if (pEnum->Next(1, &pToken, NULL) == S_OK) {
                pVoice->SetVoice(pToken);
                pToken->Release();
            }
            pEnum->Release();
        }
    }
}
#else
AdaVoice::AdaVoice(SDL_AudioDeviceID dev) : picoTTS(dev) {
    this->dev = dev;
}
#endif

AdaVoice::~AdaVoice(){
#if defined(_WIN32) || defined(_WIN64)
    if (pVoice != NULL) {
        pVoice->Release();
        pVoice = NULL;
    }
    ::CoUninitialize();

#else

#endif
}

void AdaVoice::TalkAda(std::string message) {

#if defined(_WIN32) || defined(_WIN64)

    if (pVoice == NULL || message.empty()) return;

    pVoice->Resume();

    pVoice->Speak(NULL, SPF_PURGEBEFORESPEAK, NULL);

    int size_needed = MultiByteToWideChar(CP_UTF8, 0, &message[0], (int)message.size(), NULL, 0);
    
    std::wstring wide_text(size_needed, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, &message[0], (int)message.size(), &wide_text[0], size_needed);

    pVoice->Speak(wide_text.c_str(), SPF_ASYNC | SPF_PURGEBEFORESPEAK, NULL);
#else
    if (picoTTS.dev != 0) {
        SDL_PauseAudioDevice(picoTTS.dev, 0);
    }
    picoTTS.Speak(message);
#endif
}

void AdaVoice::ShutUpAda(){
#if defined(_WIN32) || defined(_WIN64)
    pVoice->Speak(NULL, SPF_PURGEBEFORESPEAK, NULL);
    pVoice->Pause();
#else
    static bool isPaused = true;
    if (!isPaused) {
        SDL_PauseAudioDevice(picoTTS.dev, 1);
        isPaused = true;
        //std::cout << "Audio paused." << std::endl;
    } else {
        SDL_PauseAudioDevice(picoTTS.dev, 0);
        isPaused = false;
        //std::cout << "Audio resumed." << std::endl;
    }
#endif
}

std::string AdaVoice::CleanTextForTalk(const std::string& message) {
    std::regex commands("\\[[^\\]]*\\]");
    std::string text = std::regex_replace(message, commands, "");

    std::regex gestures("\\([^\\)]*\\)");
    text = std::regex_replace(text, gestures, "");

    std::regex markdown("[\\*_#`\\-]");
    text = std::regex_replace(text, markdown, " ");

    std::regex code_blocks("```[\\s\\S]*?```");
    text = std::regex_replace(text, code_blocks, "");

    std::string cleaned_emojis = "";
    cleaned_emojis.reserve(text.size());
    for (size_t i = 0; i < text.size(); ++i) {
        unsigned char c = text[i];
        if (c >= 0xF0 && c <= 0xF4) {
            i += 3;
            continue;
        }
        else if (c == 0xE2 && (i + 2 < text.size())) {
            unsigned char next1 = text[i+1];
            if (next1 >= 0x80) {
                i += 2;
                continue;
            }
        }
        cleaned_emojis += text[i];
    }

    std::regex spaces(" +");
    std::string clean_text = std::regex_replace(cleaned_emojis, spaces, " ");

    if (!clean_text.empty() && clean_text.front() == ' ') clean_text.erase(0, 1);
    if (!clean_text.empty() && clean_text.back() == ' ') clean_text.pop_back();

    return clean_text;
}