/*
AI Engine :D

AI_Engine.cpp
*/

#include "AI_Engine.hpp"

struct AI_ENGINE::Server{
    #if defined(_WIN32) || defined(_WIN64)
    HANDLE hProcess = nullptr;
    #else
    pid_t pid = -1;
    #endif
};

static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

void AI_ENGINE::LoadMemories(const std::string &file){
    std::ifstream f(file);

    if (!f.is_open()) {
        memories = json::array();
        return;
    }

    // Load the file content first to avoid partial stream state issues.
    std::string content((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    if (content.empty()) {
        memories = json::array();
        return;
    }

    try {
        memories = json::parse(content);
        if (!memories.is_array()) {
            memories = json::array();
        }
    } catch (const json::exception &e) {
        std::cerr << "Warning: failed to parse memories file '" << file << "': " << e.what() << std::endl;
        memories = json::array();
    }
}

void AI_ENGINE::SaveMemories(const std::string &file){
    std::ofstream f(file);

    if(f.is_open()){
        f << memories.dump(4);
    }else{
        std::cerr << "Error to save memories" << std::endl;
    }
}

AI_ENGINE::AI_ENGINE(const std::string& model){
    AI_config["model"] = model;
    defaultModel = model;
    AI_config["max_tokens"] = 2048;
    AI_config["include_reasoning"] = false;
    AI_config["stream"] = false;

    LoadMemories(get_config_path() + "/memories.json");
}

AI_ENGINE::~AI_ENGINE() {
    #if defined(__linux__) || defined(__unix__)
    if(server && server->pid != -1)
        DeinitOfflineMode();

    #else
    if(server && server->hProcess)
        DeinitOfflineMode();
    #endif
}

void AI_ENGINE::SetAPI_Key(const std::string &api_key){
    this->API_Key = api_key;
}

void AI_ENGINE::SetSystemPrompt(const std::string &sys_prompt){
    this->system_prompt = sys_prompt;
    this->online_system_prompt = sys_prompt;
}

void AI_ENGINE::SetOfflineSystemPrompt(const std::string &sys_prompt){
    this->offline_system_prompt = sys_prompt;
}

bool AI_ENGINE::IsOfflineModelDownloaded(const std::string &model_path) {
    std::filesystem::path p(model_path);
    return std::filesystem::exists(p);
}

bool AI_ENGINE::StartLLamaServer() {
    server = std::make_unique<Server>();

    std::filesystem::path model_path(get_config_path() + "/AdaOffline.Q4_K_M.gguf");
    const unsigned int hardware_threads = std::thread::hardware_concurrency();
    const unsigned int server_threads = hardware_threads > 0 ? hardware_threads : 1;
    
    #if defined(_WIN32) || defined(_WIN64)

        std::filesystem::path server_path = "llama-server.exe";

        std::string cmd = "\"" + server_path.string() + "\"" + 
                " -m \"" + model_path.string() + "\"" + 
                " --port 8080" + 
                " -t " + std::to_string(server_threads) +
                " -tb " + std::to_string(server_threads) +
                " --temp 0.2";

        std::vector<char> cmdBuffer(cmd.begin(), cmd.end());
        cmdBuffer.push_back('\0');

        STARTUPINFO si;
        PROCESS_INFORMATION pi;
        ZeroMemory(&si, sizeof(si));
        si.cb = sizeof(si);
        si.dwFlags = STARTF_USESHOWWINDOW;
        si.wShowWindow = SW_HIDE;
        ZeroMemory(&pi, sizeof(pi));

        if (!CreateProcess(NULL, cmdBuffer.data(), NULL, NULL, FALSE,
                       CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
            std::cerr << "Failed to start llama-server: " << GetLastError() << std::endl;
            return false;
        }

        CloseHandle(pi.hThread);
        server->hProcess = pi.hProcess;
    #else
        std::filesystem::path server_path = "llama-server";

        pid_t pid = fork();

        if(pid  < 0){
            perror("Failed to fork process for executing application");
            return false;
        }else if(pid == 0){

            int devNull = open("/dev/null", O_WRONLY);

            dup2(devNull, STDOUT_FILENO);
            dup2(devNull, STDERR_FILENO);
            close(devNull);

            execl(server_path.c_str(), "llama-server", 
                "-m", model_path.c_str(), 
                "--port", "8080", 
                "-t", std::to_string(server_threads).c_str(),
                "-tb", std::to_string(server_threads).c_str(),
                "--temp", "0.2", 
                (char*)NULL);

            exit(1);
        }else{
            server->pid = pid;
        }

    #endif

    bool ready = false;
    for (int i = 0; i < 60; ++i) {
        if (IsOfflineReady()) {
            ready = true;
            break;
        }
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    if (!ready) {
        std::cerr << "llama-server ran but did not respond in time." << std::endl;
        StopLLamaServer();
        return false;
    }

    return true;
}

void AI_ENGINE::StopLLamaServer() {
    #if defined(_WIN32) || defined(_WIN64)

        if (server && server->hProcess) {
            TerminateProcess(server->hProcess, 0);
            CloseHandle(server->hProcess);
            server->hProcess = nullptr;
        }
    #else
        kill(server->pid, SIGTERM);

        int status;
        waitpid(server->pid, &status, 0);

        server->pid = -1;
    #endif
}

bool AI_ENGINE::IsOfflineReady() const {
    CURL* curl = curl_easy_init();
    if (!curl) return false;

    curl_easy_setopt(curl, CURLOPT_URL, "http://127.0.0.1:8080/health");
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 2L);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 2L);
    curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);
    curl_easy_setopt(curl, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_1);

    CURLcode res = curl_easy_perform(curl);
    if (res == CURLE_OK) {
        long http_code = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
        if (http_code == 200) {
            curl_easy_cleanup(curl);
            return true;
        }
    }
    curl_easy_cleanup(curl);
    return false;
}

bool AI_ENGINE::InitOfflineMode() {
    if (offline_ready) return true;

    if (!IsOfflineModelDownloaded(get_config_path() + "/AdaOffline.Q4_K_M.gguf")) {
        std::cerr << "Offline model not found: " << get_config_path() + "/AdaOffline.Q4_K_M.gguf" << std::endl;
        return false;
    }

    if (!StartLLamaServer()) {
        std::cerr << "Failed to start llama-server." << std::endl;
        return false;
    }

    if (IsOfflineReady()) {

        offline_ready = true;
        system_prompt = offline_system_prompt;
        SendPrompt("Calentando motores", true, false);
    }

    return true;
}

void AI_ENGINE::DeinitOfflineMode() {
    StopLLamaServer();
    offline_ready = false;
    system_prompt = online_system_prompt;
}

std::string AI_ENGINE::SendPrompt(const std::string &Prompt, bool useOffline, bool saveMemory) {
    LoadMemories(get_config_path() + "/memories.json");

    if (useOffline && !offline_ready) {
        return "Error local: el servidor offline no está listo.";
    }

    while (memories.size() > 30) {
        memories.erase(memories.begin());
    }

    AI_config["messages"] = json::array();
       
    AI_config["messages"].push_back({{"role", "system"}, {"content", this->system_prompt}});

    for (const auto& msg : memories) {
        if (msg["role"] != "system") {
            AI_config["messages"].push_back(msg);
        }
    }

    AI_config["messages"].push_back({{"role", "user"}, {"content", Prompt}});

    if(useOffline){
        AI_config["model"] = "local";
    }else{
        AI_config["model"] = defaultModel;
    }

    //std::cout << "JSON ready to send:\n" << AI_config.dump(4) << std::endl;

    CURL* curl = curl_easy_init();
    std::string json_str = AI_config.dump();
    std::string result;
    std::string final_reply = "Error interno al inicializar red.";

    if (curl) {
        std::string url;
        struct curl_slist* headers = NULL;

        headers = curl_slist_append(headers, "Content-Type: application/json");
        headers = curl_slist_append(headers, "Expect:");

        if (useOffline) {
            url = "http://127.0.0.1:8080/v1/chat/completions";
        } else {
            url = "https://openrouter.ai/api/v1/chat/completions";
            std::string auth = "Authorization: Bearer " + API_Key;
            headers = curl_slist_append(headers, auth.c_str());
            headers = curl_slist_append(headers, "HTTP-Referer: http://localhost:3000");
            headers = curl_slist_append(headers, "X-Title: Ada_Virtual_Assistant");
        }

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_str.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, static_cast<long>(json_str.size()));
        curl_easy_setopt(curl, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_1);
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);
        
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, useOffline ? 300L : 120L);
        
        curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
        curl_easy_setopt(curl, CURLOPT_TCP_KEEPALIVE, 1L);
        curl_easy_setopt(curl, CURLOPT_TCP_KEEPIDLE, 120L);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &result);

        CURLcode res = curl_easy_perform(curl);

        if (res != CURLE_OK) {
            final_reply = useOffline ?
                "Error de conexión con el servidor local. ¿Está corriendo llama-server?" :
                "Error de conexión con el servidor. Revisa tu internet.";
        } else {
            try {
                json response = json::parse(result);
                if (response.contains("choices") && !response["choices"].empty()) {
                    final_reply = response["choices"][0]["message"]["content"];
                    
                    if (saveMemory) {
                        memories.push_back({{"role", "user"}, {"content", Prompt}});
                        memories.push_back({{"role", "assistant"}, {"content", final_reply}});
                        
                        SaveMemories(get_config_path() + "/memories.json");
                    }
                } else if (response.contains("error")) {
                    std::string error_msg = "Desconocido";
                    std::string error_code_str = "0";
                    
                    if (response["error"].is_object()) {
                        if (response["error"].contains("code")) {
                            if (response["error"]["code"].is_number()) {
                                error_code_str = std::to_string(response["error"]["code"].get<int>());
                            } else {
                                error_code_str = response["error"]["code"].get<std::string>();
                            }
                        }
                        if (response["error"].contains("message")) {
                            error_msg = response["error"]["message"].get<std::string>();
                        }
                    } else if (response["error"].is_string()) {
                        error_msg = response["error"].get<std::string>();
                    }

                    if (useOffline) {
                        final_reply = "Error del servidor local: " + error_msg;
                    } else {
                        if (error_code_str == "429")
                            final_reply = "Modelo de IA saturado, por favor espere un momento...";
                        else if (error_code_str == "401")
                            final_reply = "API Key vencida o inválida. Verifique el archivo api_key.txt(si está en Linux vaya a .config)";
                        else if (error_code_str == "413")
                            final_reply = "Limite de contexto superado.";
                        else
                            final_reply = "Error inesperado: (" + error_code_str + " " + error_msg + ")";
                    }
                } else {
                    final_reply = "Respuesta del servidor vacía o inesperada.";
                }
            } catch (const json::exception&) {
                final_reply = useOffline ?
                    "Error al procesar respuesta del servidor local." :
                    "Al parecer ocurrió un error en el servidor, intente más tarde.";
            }
        }
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
    }
    return final_reply;
}
