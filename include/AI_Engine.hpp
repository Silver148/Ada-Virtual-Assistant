#ifndef AI_ENGINE_HPP
#define AI_ENGINE_HPP
#include "json.hpp"
#include <curl/curl.h>
#include <filesystem>
#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <thread>
#include <atomic>
#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
#else
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/types.h>
#endif
#if defined(__linux__) || (__unix__)
#include "main.hpp"
#endif

using json = nlohmann::json;

class AI_ENGINE{

private:
    std::string system_prompt = ""; //By default
    std::string online_system_prompt = "";
    std::string offline_system_prompt = "";
    std::string API_Key = "";
    std::string defaultModel = "";
    json AI_config;
    json memories = json::array();;

    bool offline_ready = false;
    std::string offline_model_path = "AdaOffline.Q4_K_M.gguf";

    bool StartLLamaServer();
    void StopLLamaServer();
    bool is_running() const;

    struct Server;
    std::unique_ptr<Server> server;

public:

    AI_ENGINE(const std::string& model);
    ~AI_ENGINE();

    void SetSystemPrompt(const std::string &sys_prompt);

    void SetOfflineSystemPrompt(const std::string &sys_prompt);

    void SetAPI_Key(const std::string &api_key);

    std::string SendPrompt(const std::string &Prompt, bool useOffline = false, bool saveMemory = true);

    void LoadMemories(const std::string &file);

    void SaveMemories(const std::string &file);

    bool IsOfflineModelDownloaded(const std::string &model_path);
    bool IsOfflineReady() const;
    bool InitOfflineMode();
    void DeinitOfflineMode();

};

#endif