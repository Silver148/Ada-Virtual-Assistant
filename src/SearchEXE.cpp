#include "SearchEXE.hpp"
#include <iostream>
#include <unordered_set>

using json = nlohmann::json;

namespace fs = std::filesystem;

const std::unordered_map<std::string, std::string> KNOWN_APPS = {
    {"cmd.exe", "C:\\Windows\\System32\\cmd.exe"},
    {"cmd", "C:\\Windows\\System32\\cmd.exe"},
    {"notepad.exe", "C:\\Windows\\System32\\notepad.exe"},
    {"notepad", "C:\\Windows\\System32\\notepad.exe"},
    {"calc.exe", "C:\\Windows\\System32\\calc.exe"},
    {"calc", "C:\\Windows\\System32\\calc.exe"},
    {"taskmgr.exe", "C:\\Windows\\System32\\taskmgr.exe"},
    {"taskmgr", "C:\\Windows\\System32\\taskmgr.exe"},
    {"explorer.exe", "C:\\Windows\\explorer.exe"},
    {"explorer", "C:\\Windows\\explorer.exe"}
};

const std::unordered_set<std::string> BLACK_LIST = {
    "common files", "windowsapps", "cache", "locales", "resources", 
    "assets", "node_modules", "include", "libs", "translations", 
    "images", "fonts", "tmp", "temp", "microsoft help", "internet explorer",
    "steamapps", "steam", "epic games", "git", "system32", "syswow64", 
    "winsxs", "assembly", "documents", "downloads", "desktop"
};

static std::unordered_map<std::string, std::string> discoveredAppsCache;

void SaveCache(const std::string& filename){
    
    try{
        json discoveredAppsJSON = discoveredAppsCache;

        std::ofstream file(filename);

        if(file.is_open()){
            file << discoveredAppsJSON.dump(4);
            file.close();

        }else{
            std::cout << "No se puedo escribir el archivo: " << filename << std::endl;
        }
    } catch(const std::exception& e){
        std::cerr << "[EXCEPCIÓN]: Fallo al guardar la caché de apps: " << e.what() << std::endl;
    }
}

void LoadCache(const std::string& filename){

    try {
        std::ifstream file(filename);
        if (!file.is_open()) {
            std::cout << "[INFO]: No hay caché previa de aplicaciones. Se creará una nueva al cerrar." << std::endl;
            return;
        }

        json apps_cache;
        file >> apps_cache;
        file.close();

        // Reconstruimos el unordered_map de forma segura
        discoveredAppsCache = apps_cache.get<std::unordered_map<std::string, std::string>>();
        std::cout << "[ÉXITO]: " << discoveredAppsCache.size() << " aplicaciones cargadas desde la caché JSON." << std::endl;

    } catch (const json::exception& e) {
        std::cerr << "[ERROR]: La caché JSON está corrupta o mal estructurada: " << e.what() << std::endl;
    }
}

void RecursiveSearch(const fs::path& currentPath, const std::string& target, const std::unordered_set<std::string>& blackList, std::string& result) {
    if (!result.empty()) return;

    const auto options = fs::directory_options::skip_permission_denied;
    try {
        // Usamos el iterador simple (no recursivo), que es mil veces más estable ante errores
        for (const auto& entry : fs::directory_iterator(currentPath, options)) {
            try {
                if (entry.is_directory()) {
                    std::string dirName = entry.path().filename().string();
                    std::transform(dirName.begin(), dirName.end(), dirName.begin(), ::tolower);

                    
                    if (blackList.find(dirName) != blackList.end()) {
                        continue; 
                    }

                    RecursiveSearch(entry.path(), target, blackList, result);
                } 
                else if (entry.is_regular_file()) {
                    std::string filename = entry.path().filename().string();
                    std::transform(filename.begin(), filename.end(), filename.begin(), ::tolower);

                    if (filename == target) {
                        result = entry.path().string();
                        return;
                    }
                }
            }
            catch (...) {
                continue; 
            }
        }
    }
    catch (...) {
        return;
    }
}

std::string SearchExe::FindExe(const std::string& exeName) {

    std::string target = exeName;
    std::transform(target.begin(), target.end(), target.begin(), ::tolower);

    if (discoveredAppsCache.empty()) {
        LoadCache("cache.json");
    }

    const char* userEnv = std::getenv("USERNAME");
    
    if (target.find(".exe") == std::string::npos) {
        target += ".exe";
    }

    //Search in the KNOW_APPS
    if (KNOWN_APPS.find(target) != KNOWN_APPS.end()) {
        return KNOWN_APPS.at(target);
    }

    //Search in the cache
    if (discoveredAppsCache.find(target) != discoveredAppsCache.end()) {
        return discoveredAppsCache[target];
    }

    //"If" for Discord, XD
    if (target == "discord.exe" && userEnv) {
        std::string username(userEnv);
        fs::path discordLocalPath = "C:\\Users\\" + username + "\\AppData\\Local\\Discord";
        
        if (fs::exists(discordLocalPath)) {
            for (const auto& entry : fs::directory_iterator(discordLocalPath)) {
                if (entry.is_directory()) {
                    std::string dirName = entry.path().filename().string();
                    if (dirName.find("app-") == 0) {
                        fs::path fullDiscordExe = entry.path() / "Discord.exe";
                        if (fs::exists(fullDiscordExe)) {
                            discoveredAppsCache[target] = fullDiscordExe.string();
                            return fullDiscordExe.string();
                        }
                    }
                }
            }
        }
    }

    const char* progFiles = std::getenv("ProgramFiles");
    const char* progFilesX86 = std::getenv("ProgramFiles(x86)");
    const char* localAppData = std::getenv("LOCALAPPDATA");

    std::vector<std::string> directAppPaths;
    
    if (progFiles) {
        directAppPaths.push_back(std::string(progFiles) + "\\VideoLAN\\VLC"); 
        directAppPaths.push_back(std::string(progFiles) + "\\Google\\Chrome\\Application");
        directAppPaths.push_back(std::string(progFiles) + "\\obs-studio\\bin\\64bit"); 
        directAppPaths.push_back(std::string(progFiles) + "\\Microsoft Visual Studio\\Installer"); 
    }
    if (progFilesX86) {
        directAppPaths.push_back(std::string(progFilesX86) + "\\Steam"); 
    }
    if (localAppData) {
        directAppPaths.push_back(std::string(localAppData) + "\\Programs\\Lazarus");
        directAppPaths.push_back(std::string(localAppData) + "\\Programs\\Microsoft VS Code"); 
        directAppPaths.push_back(std::string(localAppData) + "\\Programs\\Opera GX");          
    }

    for (const auto& path : directAppPaths) {
        fs::path fullPath = fs::path(path) / target;
        if (fs::exists(fullPath)) {
            discoveredAppsCache[target] = fullPath.string(); 
            SaveCache("cache.json");
            
            return fullPath.string();
        }
    }

    //Scan paths
    std::vector<std::string> searchPaths;
    if (localAppData)  searchPaths.push_back(std::string(localAppData));
    if (progFiles)     searchPaths.push_back(std::string(progFiles));
    if (progFilesX86)  searchPaths.push_back(std::string(progFilesX86));

    std::string foundResult = "";

    for (const auto& basePath : searchPaths) {
        if (!fs::exists(basePath)) continue;

        RecursiveSearch(basePath, target, BLACK_LIST, foundResult);

        if (!foundResult.empty()) {
            discoveredAppsCache[target] = foundResult;
            SaveCache("cache.json");

            return foundResult;
        }
    }

    return ""; 
}