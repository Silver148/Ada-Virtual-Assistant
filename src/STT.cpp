#include "STT.hpp"

void AudioCaptureCallback(void* userdata, Uint8* stream, int len){
    
    STT* stt_instance = (STT*)userdata;
    if (!stt_instance || !stt_instance->recognizer) return;

    int is_ready = vosk_recognizer_accept_waveform(stt_instance->recognizer, (const char*)stream, len);

    if(is_ready){
        std::string raw_result = vosk_recognizer_result(stt_instance->recognizer);
        //std::cout << "Complete phrase: " << raw_result << std::endl;
        
        stt_instance->PushText(raw_result);
    }

}

void STT::PushText(const std::string& raw_json){
    try{
        std::lock_guard<std::mutex> lock(mtx);
        MicText = json::parse(raw_json);
    }catch(...){
        std::cerr << "Error to parse microphone text json" << std::endl;
    }
    
}

std::string STT::TextFromMic(){
    std::lock_guard<std::mutex> lock(mtx);

    if(MicText.is_object() && MicText.contains("text")){
        std::string text = MicText["text"];
        MicText = json::object();

        return text;
    }

    return "";
}

bool STT::ResetMic(){

    if (mic != 0) {
        SDL_CloseAudioDevice(mic);
        mic = 0;
    }

    mic = SDL_OpenAudioDevice(NULL, 1, &desiredSpec, &obtainedSpec, 0); //Open a mic

    if(mic == 0){
        std::cerr << "Error to open mic: " << SDL_GetError() << std::endl;
        return false;
    }

    SDL_PauseAudioDevice(mic, 1);

    return true;
}

void STT::StartListening(){
    if(mic == 0) return;
    SDL_PauseAudioDevice(mic, 0);
}

void STT::StopListening(){
    if(mic == 0) return;
    SDL_PauseAudioDevice(mic, 1);
}

STT::STT(){
    model = nullptr;
    recognizer = nullptr;
    mic = 0;
}

bool STT::Init(){

    vosk_set_log_level(-1);

    #if defined(__linux__) || defined(__unix__)
    std::string modelPath = get_base_dir() + "/vosk_model";
    model = vosk_model_new(modelPath.c_str());

    if(!model){
        std::cerr << "Error to load vosk model" << std::endl;
        return false;
    }
        
    #else
    model = vosk_model_new("vosk_model");

    if(!model){
        std::cerr << "Error to load vosk model" << std::endl;
        return false;
    } 
    #endif

    float sample_rate = 16000.0f;
    recognizer = vosk_recognizer_new(model, sample_rate);

    if(!recognizer){
        std::cerr << "Error to create recognizer!" << std::endl;
        return false;
    }

    SDL_zero(desiredSpec);
    
    desiredSpec.freq = 16000;
    desiredSpec.format = AUDIO_S16SYS;
    desiredSpec.channels = 1;
    desiredSpec.samples = 4096;
    desiredSpec.callback = AudioCaptureCallback;
    desiredSpec.userdata = this;

    mic = SDL_OpenAudioDevice(NULL, 1, &desiredSpec, &obtainedSpec, 0); //Open a mic

    if(mic == 0){
        std::cerr << "Error to open mic: " << SDL_GetError() << std::endl;
        return false;
    }

    SDL_PauseAudioDevice(mic, 1);

    return true;
}

STT::~STT(){
    if (recognizer != nullptr) 
        vosk_recognizer_free(recognizer);

    if (model != nullptr)       
        vosk_model_free(model);

    if(mic != 0)
        SDL_CloseAudioDevice(mic);
}