/*
AI Engine :D

AI_Engine.cpp
*/

#include "AI_Engine.hpp"
#include "json.hpp"
#include <curl/curl.h>
#include <string>
#include <iostream>
#include <fstream>

static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

void AI_ENGINE::LoadMemories(const std::string &file){
    std::ifstream f(file);

    if(f.peek() == std::ifstream::traits_type::eof()){
        //std::cout << "Memoria vacía, creando base..." << std::endl;
        memories = json::array();
        return;
    }

    if(f.is_open()){
        f >> memories;
        //std::cout << "Recuerdos cargados ;)..." << std::endl;
    }else{
        std::cerr << "Error al cargar recuerdos" << std::endl;
    }
}

void AI_ENGINE::SaveMemories(const std::string &file){
    std::ofstream f(file);

    if(f.is_open()){
        f << memories.dump(4);
    }else{
        std::cerr << "Error al guardar recuerdos" << std::endl;
    }
}

AI_ENGINE::AI_ENGINE(){
    AI_config["model"] = "openai/gpt-oss-120b:free";
    AI_config["max_tokens"] = 2048;
    AI_config["stream"] = false;

    LoadMemories("memories.json");
}

void AI_ENGINE::SetAPI_Key(const std::string &API_Key){
    this->API_Key = API_Key;
}

void AI_ENGINE::SetSystemPrompt(const std::string &system_prompt){
        this->system_prompt = system_prompt;
}

std::string AI_ENGINE::SendPrompt(const std::string &Prompt){

        while (memories.size() > 30) {
            memories.erase(memories.begin());
        }

        json response;

        AI_config["messages"] = json::array();
       
        AI_config["messages"].push_back({{"role", "system"}, {"content", this->system_prompt}});

        for (const auto& msg : memories) {
            if (msg["role"] != "system") {
                AI_config["messages"].push_back(msg);
            }
        }

        AI_config["messages"].push_back({{"role", "user"}, {"content", Prompt}});

        //std::cout << "JSON Listo para enviar:\n" << AI_config.dump(4) << std::endl;

        CURL* curl = curl_easy_init();

        std::string json_str = AI_config.dump();
        std::string result;
        std::string final_reply = "";

        if(curl){

            curl_easy_setopt(curl, CURLOPT_URL, "https://openrouter.ai/api/v1/chat/completions");

            struct curl_slist* headers = NULL;

            headers = curl_slist_append(headers, "Content-Type: application/json");

            std::string auth_header = "Authorization: Bearer " + API_Key;
            headers = curl_slist_append(headers, auth_header.c_str());

            headers = curl_slist_append(headers, "HTTP-Referer: http://localhost:3000"); 
            headers = curl_slist_append(headers, "X-Title: Ada_Virtual_Assistant");

            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
            
            //configure to send
            curl_easy_setopt(curl, CURLOPT_POST, 1L);
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_str.c_str());

            //configure to recieve
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &result);
            
            //Send prompt to API
            CURLcode res = curl_easy_perform(curl);

            if (res != CURLE_OK) {
                //std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
                final_reply = "Error de conexión con el servidor. Revisa tu internet.";
            } else {
                try {
                    response = json::parse(result);
                    std::cout << response << std::endl;

                    if (response.contains("choices") && !response["choices"].empty()) {
            
                        std::string ada_reply = response["choices"][0]["message"]["content"];

                        memories.push_back({{"role", "user"}, {"content", Prompt}});
                        memories.push_back({{"role", "assistant"}, {"content", ada_reply}});

                        SaveMemories("memories.json");

                        final_reply = ada_reply;

                        return ada_reply;

                    } else if (response.contains("error")) {

                        std::string error_msg = response["error"]["message"];
                        //std::cerr << "API Error: " << error_msg << std::endl;
                        final_reply = "Error de la API: " + error_msg;
                    }

                    return "Al parecer ocurrió un error inesperado en la respuesta del servidor.";

                } catch (const json::exception& e) {
                    //std::cerr << "JSON Exception caught: " << e.what() << std::endl;
                    final_reply = "Al parecer ocurrió un error en el servidor, intente más tarde. ";
                }
            }

            curl_slist_free_all(headers);
            curl_easy_cleanup(curl);

        }

        return final_reply;
}