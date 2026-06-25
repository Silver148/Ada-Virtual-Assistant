#ifndef AI_ENGINE_HPP
#define AI_ENGINE_HPP
#include <string>
#include "json.hpp"
#if defined(__linux__) || (__unix__)
#include "main.hpp"
#endif

using json = nlohmann::json;

class AI_ENGINE{

private:
    std::string system_prompt = ""; //By default
    std::string API_Key = "";
    json AI_config;
    json memories = json::array();;
    json response;

public:

    AI_ENGINE();

    void SetSystemPrompt(const std::string &Personalidad);

    void SetAPI_Key(const std::string &API_Key);

    std::string SendPrompt(const std::string &Prompt);

    void LoadMemories(const std::string &file);

    void SaveMemories(const std::string &file);

};

#endif