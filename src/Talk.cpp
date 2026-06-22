/*


Talk.cpp
*/

#include "Talk.hpp"

#if defined(__linux__) || defined(__unix__)
PiperBridge::PiperBridge(const std::string& modelPath, SDL_AudioDeviceID dev) {
    int textPipe[2], audioPipe[2];
    pipe(textPipe);
    pipe(audioPipe);

    piperProcessId = fork();

    if (piperProcessId == 0) {
        dup2(textPipe[0], STDIN_FILENO);
        dup2(audioPipe[1], STDOUT_FILENO);

        close(textPipe[0]); close(textPipe[1]);
        close(audioPipe[0]); close(audioPipe[1]);

        execl("./bin/piper/piper", "piper", "--model", modelPath.c_str(), "--output-raw", 
        "--length-scale", "0.8", NULL);
        exit(1);
    } else {
        close(textPipe[0]); 
        close(audioPipe[1]);
        writePipe[1] = textPipe[1];
        readPipe = audioPipe[0];

        std::thread audioThread(&PiperBridge::StreamAudio, this, dev);

        //Set higher priority for audio thread
        pthread_t handle = audioThread.native_handle();
        int policy = SCHED_FIFO;
        struct sched_param param;
        param.sched_priority = 10;
        pthread_setschedparam(handle, policy, &param);

        audioThread.detach();
    }
}

void PiperBridge::StreamAudio(SDL_AudioDeviceID dev) {
    char buffer[2048];
    
    while(1){
        ssize_t bytes = read(readPipe, buffer, sizeof(buffer));
        if(bytes > 0){
            SDL_QueueAudio(dev, buffer, bytes);
        }else if(bytes == 0)
            break;
    }
}

void PiperBridge::Speak(const std::string& text) {
    Resume(); // Ensure Piper is running
    std::string command = text + "\n";

    if (write(writePipe[1], command.c_str(), command.length()) == -1) {
        perror("Failed to send text to Piper");
    }
}

void PiperBridge::Stop(){
        
    if (piperProcessId > 0)
        kill(piperProcessId, SIGSTOP);
}

void PiperBridge::Resume(){

    if (piperProcessId > 0) 
        kill(piperProcessId, SIGCONT);

}

PiperBridge::~PiperBridge() {
    if (piperProcessId > 0) {
        kill(piperProcessId, SIGTERM);

        int status;
        waitpid(piperProcessId, &status, WNOHANG);
    }

    close(writePipe[0]);
    close(writePipe[1]);
}
#endif


#if defined(_WIN32) || defined(_WIN64)
AdaVoice::AdaVoice(){

    ::CoInitialize(NULL);
    
    HRESULT hr = ::CoCreateInstance(CLSID_SpVoice, NULL, CLSCTX_ALL, IID_ISpVoice, (void**)&pVoice);
    
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
AdaVoice::AdaVoice(SDL_AudioDeviceID dev) {
    this->dev = dev;
    piper = new PiperBridge("./bin/piper/es_AR-daniela-high.onnx", dev);
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
    if(piper) delete piper;
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
    if(piper && !message.empty()){
        piper->Speak(message);
    }
#endif
}

void AdaVoice::ShutUpAda(){
#if defined(_WIN32) || defined(_WIN64)
    pVoice->Speak(NULL, SPF_PURGEBEFORESPEAK, NULL);
    pVoice->Pause();
#else
    piper->Stop();
#endif
}

std::string AdaVoice::CleanTextForTalk(const std::string& message) {
    std::regex commands("\\[[^\\]]*\\]");
    std::string without_commands = std::regex_replace(message, commands, "");

    std::regex gestures("\\([^\\)]*\\)");
    std::string without_gestures = std::regex_replace(without_commands, gestures, "");

    std::regex markdown("[\\*_#`\\-]");
    std::string without_markdown = std::regex_replace(without_gestures, markdown, "");

    std::string without_emojis = "";
    without_emojis.reserve(without_markdown.size());

    for (size_t i = 0; i < without_markdown.size(); ++i) {
        unsigned char c = without_markdown[i];
        if (c >= 0xF0 && c <= 0xF4) {
            i += 3;
            continue;
        }
        else if (c == 0xE2 && (i + 2 < without_markdown.size())) {
            unsigned char next1 = without_markdown[i+1];
            if (next1 >= 0x80) {
                i += 2;
                continue;
            }
        }
        
        without_emojis += without_markdown[i];
    }

    std::regex spaces(" +");
    std::string clean_text = std::regex_replace(without_emojis, spaces, " ");

    if (!clean_text.empty() && clean_text.front() == ' ') {
        clean_text.erase(0, 1);
    }
    if (!clean_text.empty() && clean_text.back() == ' ') {
        clean_text.pop_back();
    }

    return clean_text;
}