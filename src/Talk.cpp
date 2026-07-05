/*


Talk.cpp
*/

#include "Talk.hpp"

#if defined(__linux__) || defined(__unix__)

bool PiperBridge::StartPiper() {
    int textPipe[2], audioPipe[2];
    
    if (pipe(textPipe) == -1 || pipe(audioPipe) == -1) {
        perror("Error creating pipes");
        return false;
    }

    char* env[] = {
        (char*)"OMP_NUM_THREADS=2",
        (char*)"OMP_WAIT_POLICY=PASSIVE",
        NULL
    };

    piperProcessId = fork();

    if (piperProcessId == 0) {

        dup2(textPipe[0], STDIN_FILENO);
        dup2(audioPipe[1], STDOUT_FILENO);

        for (int i = 3; i < 1024; ++i) close(i); 

        unsetenv("DISPLAY");
        unsetenv("XAUTHORITY");

        std::string base = get_base_dir();
        std::string piper_path = base + "/bin/piper/piper";

        execle(piper_path.c_str(), piper_path.c_str(),
            "--model", modelPath.c_str(),
            "--output_raw",
            "--noise_scale", "0.2",
            "--noise_w", "0.3",
            "--length_scale", "0.90",
            NULL, env);
        _exit(1);
    } else if (piperProcessId > 0) {
        close(textPipe[0]); 
        close(audioPipe[1]);
        writePipe[1] = textPipe[1];
        readPipe = audioPipe[0];

        std::thread audioThread(&PiperBridge::StreamAudio, this, dev);

        pthread_t handle = audioThread.native_handle();
        struct sched_param param;
        param.sched_priority = 20;

        pthread_setschedparam(handle, SCHED_RR, &param);

        audioThread.detach();
        return true;
    } else {
        perror("Error in fork");
        return false;
    }
}

PiperBridge::PiperBridge(const std::string& modelPath, SDL_AudioDeviceID dev) {
    this->modelPath = modelPath;
    this->dev = dev;
    this->isRunning = true;
    this->readPipe = -1;
    this->writePipe[0] = -1;
    this->writePipe[1] = -1;
    this->piperProcessId = -1;
    
    if (!StartPiper()) {
        fprintf(stderr, "Could not initialize piper\n");
    }
}

void PiperBridge::StreamAudio(SDL_AudioDeviceID dev) {
    char buffer[1024];
    while (isRunning.load()) {
        ssize_t bytes = read(readPipe, buffer, sizeof(buffer));
        
        if (bytes > 0) {
            SDL_LockAudioDevice(dev);
            SDL_QueueAudio(dev, buffer, bytes);
            SDL_UnlockAudioDevice(dev);
        } else {
            break;
        }
    }
}

void PiperBridge::Speak(const std::string& text) {
       
    this->isRunning = false; 
    SDL_PauseAudioDevice(dev, 1);
    
    SDL_LockAudioDevice(dev);
    SDL_ClearQueuedAudio(dev);
    SDL_UnlockAudioDevice(dev);
    
    if (readPipe >= 0) {
        int flags = fcntl(readPipe, F_GETFL, 0);
        fcntl(readPipe, F_SETFL, flags | O_NONBLOCK);
        char dummy[4096];
        while (read(readPipe, dummy, sizeof(dummy)) > 0);
        fcntl(readPipe, F_SETFL, flags);
    }

    Resume(); 
    
    std::string command = text + "\n";
    if (write(writePipe[1], command.c_str(), command.length()) == -1) {
        perror("Failed to send text to Piper");
        piperProcessId = -1; 
    }
}

void PiperBridge::Stop() {
    if (piperProcessId > 0) {
        this->isRunning = false;
        
        SDL_LockAudioDevice(dev);
        SDL_ClearQueuedAudio(dev);
        SDL_PauseAudioDevice(dev, 1);
        
    }
}

void PiperBridge::Resume(){

    if (piperProcessId > 0){
        this->isRunning = true;
        SDL_PauseAudioDevice(dev, 0);
    }
        
}

PiperBridge::~PiperBridge() {
    this->isRunning = false;

    if (writePipe[1] >= 0) {
        close(writePipe[1]);
        writePipe[1] = -1;
    }

    if (piperProcessId > 0) {
        kill(piperProcessId, SIGTERM);

        int status;
        waitpid(piperProcessId, &status, 0);
    }

    if (readPipe >= 0) {
        close(readPipe);
        readPipe = -1;
    }
}

bool CopyToRam(const std::string& source, const std::string& dest) {
    std::ifstream src(source, std::ios::binary);
    std::ofstream dst(dest, std::ios::binary);
    if (!src || !dst) return false;
    dst << src.rdbuf();
    return true;
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
    std::string model_path_in_disk = get_base_dir() + "/bin/piper/es_AR-daniela-quant.onnx";
    std::string config_model_path_in_disk = get_base_dir() + "/bin/piper/es_AR-daniela-quant.onnx.json";
    std::string ram_path = "/dev/shm/es_AR-daniela-quant.onnx";
    std::string config_ram_path = "/dev/shm/es_AR-daniela-quant.onnx.json";

    if(CopyToRam(model_path_in_disk, ram_path) && CopyToRam(config_model_path_in_disk, config_ram_path)){
        std::cout << "Model copied to RAM successfully." << std::endl;
        piper = std::make_unique<PiperBridge>(ram_path, dev);
    } else {
        std::cerr << "Failed to copy model to RAM." << std::endl;
        piper = std::make_unique<PiperBridge>(model_path_in_disk, dev);
    }
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
    std::string ram_path = "/dev/shm/es_AR-daniela-quant.onnx";
    std::string config_ram_path = "/dev/shm/es_AR-daniela-quant.onnx.json";

    std::remove(ram_path.c_str());
    std::remove(config_ram_path.c_str());
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
    std::string clean_text = CleanTextForTalk(message);
    
    if(piper && !clean_text.empty()){
        piper->Speak(clean_text);
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
    std::string text = std::regex_replace(message, commands, "");

    std::regex gestures("\\([^\\)]*\\)");
    text = std::regex_replace(text, gestures, "");

    std::regex markdown("[\\*_#`\\-]");
    text = std::regex_replace(text, markdown, "");

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