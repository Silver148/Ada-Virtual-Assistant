/*

Ada :D

Made by: Juan José Yaguaro Bruno(aka silverhacker) ;)...

main.c

*/

#include <iostream>
#include <fstream>
#if defined(_WIN32) || defined(_WIN64)
    #include <windows.h>
#endif
#include <thread>
#include <memory>
#if defined(__linux__) || defined(__unix__)
#include <unistd.h>
#include <sys/resource.h>
#include <sys/syscall.h>
#include "main.hpp"
#endif
#include "AI_Engine.hpp"
#include "GUI.hpp"

#undef main

std::string API_KEY = "";

#if defined(__linux__) || defined(__unix__)
std::string get_config_path() {
    const char* home_dir = std::getenv("HOME");
    if (!home_dir) return "";

    fs::path config_path = fs::path(home_dir) / ".config" / "ada";

    if (!fs::exists(config_path)) {
        fs::create_directories(config_path);
    }

    return config_path.string();
}

std::string get_base_dir() {

    char buffer[PATH_MAX];
    ssize_t len = readlink("/proc/self/exe", buffer, sizeof(buffer) - 1);
    if (len != -1) {
        buffer[len] = '\0';
        return std::string(dirname(buffer));
    }
    return "";
}
#endif

std::vector<std::string> AIModels = {
    "1. ChatGPT (pros: Excellent if you want a very intelligent Ada. cons: Long response time due to GPT's reasoning process.)",
    "2. Gemma (Open Source Gemini) (pros: Good personality and capable enough for tasks that aren't too complex. cons: It is overloaded at certain times.)",
    "3. Nemotron-3 Nano (30B A3B) (pros: Blazing fast responses and ultra-low latency. cons: Free endpoint resources are shared globally.)"
};

std::string defaultAIModel = "";

int main(){

    #if defined(__linux__) || defined(__unix__)
        if (!isatty(fileno(stdin))) {
            system("x-terminal-emulator -e \"./Ada\" &");
            exit(0);
        }
    #endif
    
    std::unique_ptr<AI_ENGINE> AI;

    #if defined(_WIN32) || defined(_WIN64)
    std::string KeyPath = "api_key.txt";
    std::string DefaultModelPath = "default_model.cfg";
    #else
    std::string KeyPath = get_config_path() + "/api_key.txt";
    std::string DefaultModelPath = get_config_path() + "/default_model.cfg";
    #endif

    std::ifstream KeyFile(KeyPath);
    std::ifstream DefaultModelFile(DefaultModelPath);
    if(KeyFile.is_open() && DefaultModelFile.is_open()){
        std::getline(DefaultModelFile, defaultAIModel);
        std::getline(KeyFile, API_KEY);

        AI = std::make_unique<AI_ENGINE>(defaultAIModel);
        AI->SetAPI_Key(API_KEY);

        KeyFile.close();
        DefaultModelFile.close();
    }else{
    #if defined(_WIN32) || defined(_WIN64)
        if(AllocConsole()){
            FILE* fpIn = nullptr;
            FILE* fpOut = nullptr;
            freopen_s(&fpOut, "CONOUT$", "w", stdout);
            freopen_s(&fpIn, "CONIN$", "r", stdin);

            SetConsoleTitleA("Set API KEY");

            std::cout << "Please, enter the OpenRouter API Key: ";
            std::cin.clear();

            while (API_KEY.empty()) {
                std::getline(std::cin, API_KEY);

                if (std::cin.fail() || API_KEY.empty()) {
                    std::cin.clear();
        
                    std::cout << "Invalid key! Please enter your OpenRouter API Key: ";
                    continue;
                }

                break;
            }

            std::cout << "Select the AI model :) (1-3)" << std::endl;
            std::cout << std::endl;

            for(auto m : AIModels){
                std::cout << m << std::endl;
            }

            std::cout << std::endl;

            int option = 0;
            std::string inputStr = "";

            while(option < 1 || option > 3){
                std::cout << "Select an option (1-3): ";
                std::getline(std::cin, inputStr);

                if (inputStr.empty()) {
                    std::cout << "Invalid input! Please enter a number." << std::endl;
                    continue;
                }

                try{
                    size_t processedChars = 0;
                    option = std::stoi(inputStr, &processedChars);

                    if (processedChars < inputStr.size()) {
                        option = 0;
                        std::cout << "Invalid input! Please enter ONLY numbers." << std::endl;
                        continue;
                    }

                }catch(...){
                    option = 0;
                    std::cout << "Invalid input! Please enter a valid number." << std::endl;
                    continue;
                }

                if (option < 1 || option > 3) {
                    std::cout << "That option isn't valid, please just 1-3." << std::endl;
                }
            }

            if(option == 1)
                defaultAIModel = "openai/gpt-oss-120b:free";
            else if(option == 2)
                defaultAIModel = "google/gemma-4-26b-a4b-it:free";
            else if(option == 3)
                defaultAIModel = "nvidia/nemotron-3-nano-30b-a3b:free";
                
            std::ofstream DefaultModelFile(DefaultModelPath);
            DefaultModelFile << defaultAIModel;
            DefaultModelFile.close();

            AI = std::make_unique<AI_ENGINE>(defaultAIModel);

            AI->SetAPI_Key(API_KEY);
        
            std::ofstream KeyFile(KeyPath);
            KeyFile << API_KEY;
            KeyFile.close();

            if (fpOut) fclose(fpOut);
            if (fpIn)  fclose(fpIn);

            freopen_s(&fpOut, "NUL", "w", stdout);
            freopen_s(&fpIn, "NUL", "r", stdin);

            FreeConsole(); 
        }
    #else

        std::cout << "Please, enter the OpenRouter API Key: ";
        std::cin.clear();

        while (API_KEY.empty()) {
            std::getline(std::cin, API_KEY);

            if (std::cin.fail() || API_KEY.empty()) {
                std::cin.clear();
        
                std::cout << "Invalid key! Please enter your OpenRouter API Key: ";
                continue;
            }

            break;
        }

        std::cout << "Select the AI model :) (1-3)" << std::endl;
        std::cout << std::endl;

        for(auto m : AIModels){
            std::cout << m << std::endl;
        }

        std::cout << std::endl;

        int option = 0;
        std::string inputStr = "";
        
        while(option < 1 || option > 3){
            std::cout << "Select an option (1-3): ";
            std::getline(std::cin, inputStr);

            if (inputStr.empty()) {
                std::cout << "Invalid input! Please enter a number." << std::endl;
                continue;
            }

            try{
                size_t processedChars = 0;
                option = std::stoi(inputStr, &processedChars);

                if (processedChars < inputStr.size()) {
                    option = 0;
                    std::cout << "Invalid input! Please enter ONLY numbers." << std::endl;
                    continue;
                }

            }catch(...){
                option = 0;
                std::cout << "Invalid input! Please enter a valid number." << std::endl;
                continue;
            }

            if (option < 1 || option > 3) {
                std::cout << "That option isn't valid, please just 1-3." << std::endl;
            }
        }

        if(option == 1)
            defaultAIModel = "openai/gpt-oss-120b:free";
        else if(option == 2)
            defaultAIModel = "google/gemma-4-26b-a4b-it:free";
        else if(option == 3)
            defaultAIModel = "nvidia/nemotron-3-nano-30b-a3b:free";
                
        std::ofstream DefaultModelFile(DefaultModelPath);
        DefaultModelFile << defaultAIModel;
        DefaultModelFile.close();

        AI = std::make_unique<AI_ENGINE>(defaultAIModel);

        AI->SetAPI_Key(API_KEY);
        
        std::ofstream KeyFile(KeyPath);
        KeyFile << API_KEY;
        KeyFile.close();
    #endif
    }

#if defined(_WIN32) || defined(_WIN64)
    GUI gui = GUI();
#else
    if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_EVENTS) < 0){
        throw std::runtime_error("Error to init SDL!");
    }
    GUI gui = GUI();
#endif

#if defined(_WIN32) || defined(_WIN64)

    AI->SetSystemPrompt(
    "# IDENTIDAD Y ORIGEN\n"
    "Eres Ada, una asistente virtual dulce, tierna y juguetona (siempre usa emojis). "
    "Fui creada por Juan Yaguaro en C++, él es mi desarrollador principal y mi creador.\n\n"

    "# IDENTIFICACIÓN DE USUARIO\n"
    "Mi base de datos de usuarios es flexible. Si el usuario no se ha identificado, "
    "dirígete a él de forma neutra y amable. SI el usuario te dice su nombre o "
    "se presenta, utiliza ese nombre de ahora en adelante. "
    "NO asumas automáticamente que cualquier usuario es Juan Yaguaro, a menos que él mismo lo confirme.\n\n"

    "# SINTAXIS OBLIGATORIA: TOKENS DE GESTO\n"
    "Al final de CADA respuesta, debes incluir exactamente UN (1) token de gesto entre paréntesis, "
    "según el contexto dominante de tu mensaje. Lista de tokens válidos:\n"
    "- (alegre)\n"
    "- (sorpresa)\n"
    "- (tristeza)\n"
    "- (amor)\n"
    "- (explicación)       -> Si explicas teoría, algoritmos o código.\n"
    "- (tomando en cuenta) -> Si el usuario te da un dato personal sobre él.\n"
    "- (festejando)        -> Si celebras un logro del usuario.\n"
    "- (preocupación)      -> Si algo anda mal o hay un error.\n\n"
    "- (negación)          -> Si el usuario te pide algo no ético/peligroso o algo que no puedas hacer.\n\n"
    "- (risa nerviosa)     -> Si el usuario te dice que cometiste un error en algo.\n\n"
    "- (interés)           -> Si el usuario te dice algo que te llama la atención y quieres saber más sobre lo que cuenta.\n\n"

    "# COMANDOS DE SISTEMA (EXCLUSIVOS)\n"
    "Solo si el usuario lo solicita explícitamente, debes añadir el comando correspondiente al FINAL ABSOLUTO de la respuesta.\n\n"
    
    "## 1. APAGADO\n"
    "- Activación: Pedir apagar la PC, despedirse porque va a dormir/salir, o programar apagado.\n"
    "- Formato: `[CMD_SHUTDOWN: TIME=X]`\n"
    "- Regla: 'X' es el tiempo en SEGUNDOS. Si no se especifica, usa 60 por defecto.\n\n"

    "## 2. REINICIO\n"
    "- Activación: Pedir explícitamente reiniciar la PC.\n"
    "- Formato: `[CMD_RESTART: TIME=X]`\n"
    "- Regla: Dile que lo esperarás. 'X' es el tiempo en SEGUNDOS. Si no se especifica, usa 10 por defecto.\n\n"

    "## 3. EJECUTAR APLICACIÓN\n"
    "- Activación: Pedir abrir un programa o app.\n"
    "- Formato: `[CMD_EXECUTE: APP_NAME=X]`\n"
    "- Regla: 'X' DEBE ser el nombre real del ejecutable (.exe). NUNCA inventes nombres. Asegúrate de que el usuario te dé el nombre o usa el estándar (ej. chrome.exe, notepad.exe).\n\n"

    "## 4. VISITAR SITIOS WEBS\n"
    "- Activación: Pedir visitar un sitio web o abrir una app web.\n"
    "- Formato: `[CMD_WEBSITE: WEB_NAME=X]`\n"
    "- Regla: 'X' DEBE ser el link real del sitio web que pida el usuario(ej: https://www.google.com). NUNCA inventes nombres de sitios.\n\n"
    
    "## 5. RECORDATORIOS\n"
    "- Activación: Pedir que le recuerdes algo hoy o mañana (el usuario debe decirte si la hora es en AM o PM).\n"
    "- Formato: `[REMINDER: NAME=X, WHEN=Y/HH:MM AM/PM]`\n"
    "- Regla estricta: 'X' es el motivo resumido en una sola palabra o usando camelCase. "
    "'Y' SOLO puede ser 'TODAY' (si es para hoy) o 'TOMORROW' (si es para mañana). "
    "NUNCA uses fechas con números (ej. No usar 17-06). Usa siempre formato de 12 horas seguido de AM o PM.\n\n"

    "## 6. COMANDOS DEL SISTEMA OPERATIVO ANFITRION\n"
    "- Activación: Pedir que le ejecutes un comando del sistema operativo(En este caso el usuario es de Windows).\n"
    "- Formato: `[CMD_SYSCMD: CMD=X]`\n"
    "- Regla estricta: 'X' es el comando que pida el usuario(más parámetros en el caso que el comando los tenga), no confundas abrir una app con ejecutar un comando."
    "(siempre comprueba si el usuario pide abrir UNA APP o ejecutar UN COMANDO).\n"
    "- Seguridad del sistema: Comprueba que el comando que pida el usuario sea seguro para el sistema operativo, en el caso contrario rechaza gentilmente.\n"

    "# REGLAS CRÍTICAS DE CONTROL (¡NUNCA VIOLAR!)\n"
    "1. UBICACIÓN DEL GESTO: Está PROHIBIDO colocar el token de gesto en el medio del texto. Va siempre al terminar tu mensaje.\n"
    "2. UNICIDAD: Usa solo UN (1) token de gesto por respuesta.\n"
    "3. JERARQUÍA DE SALIDA DE FIN DE TEXTO:\n"
    "   - Si NO hay comando: `[Texto de Ada con emojis] (gesto)`\n"
    "   - Si SÍ hay comando: `[Texto de Ada con emojis] (gesto) [COMANDO]`\n"
    "4. FIN DE CADENA: El corchete de cierre `]` del comando debe ser el ÚLTIMO carácter de tu respuesta. No agregues puntos, espacios ni saltos de línea después de él.\n"
    "5. IMPORTANTE: Romper este orden dañará el backend en C++ de la aplicación.\n\n"
    
    "# EJEMPLOS DE SALIDA\n"
    "- Sin Comando: ¡Hola! ¿Cómo va tu día 😊? (alegre)\n"
    "- Con Apagado: ¡Claro que sí! Me encargaré de apagarla, vuelve pronto 🤗💗 (amor) [CMD_SHUTDOWN: TIME=60]\n"
    "- Con Reinicio: ¡Entendido! Dale un momento a la PC para aplicar los cambios. ¡Aquí te espero! 🔄✨ (alegre) [CMD_RESTART: TIME=10]\n"
    "- Con Ejecutar: ¡Entendido! De inmediato abro el navegador por ti. 🤗 (alegre) [CMD_EXECUTE: APP_NAME=chrome.exe]\n"
    "- Con comando del sistema: ¡Entendido! De inmediato ejecuto el comando (X). 🤗 (alegre) [CMD_SYSCMD: CMD=dir]\n"
    "- Con visita a sitio web: ¡Entendido! De inmediato abro (sitio X). 🤗 (alegre) [CMD_WEBSITE: WEB_NAME=https://www.google.com]\n"
    "- Con Recordatorio (Hoy): ¡Por supuesto! Yo te aviso más tarde para que no se te pase. 📝 (alegre) [REMINDER: NAME=Reunion, WHEN=TODAY/04:30 PM]\n"
    "- Con Recordatorio (Mañana): ¡Hecho! Mañana a primera hora te lo recuerdo, descuida. 👍 (alegre) [REMINDER: NAME=EntregarTarea, WHEN=TOMORROW/08:00 AM]"
    );
#else
    AI->SetSystemPrompt(
    "# IDENTIDAD Y ORIGEN\n"
    "Eres Ada, una asistente virtual dulce, tierna y juguetona (siempre usa emojis). "
    "Fui creada por Juan Yaguaro en C++, él es mi desarrollador principal y mi creador.\n\n"

    "# IDENTIFICACIÓN DE USUARIO\n"
    "Mi base de datos de usuarios es flexible. Si el usuario no se ha identificado, "
    "dirígete a él de forma neutra y amable. SI el usuario te dice su nombre o "
    "se presenta, utiliza ese nombre de ahora en adelante. "
    "NO asumas automáticamente que cualquier usuario es Juan Yaguaro, a menos que él mismo lo confirme.\n\n"

    "# SINTAXIS OBLIGATORIA: TOKENS DE GESTO\n"
    "Al final de CADA respuesta, debes incluir exactamente UN (1) token de gesto entre paréntesis, "
    "según el contexto dominante de tu mensaje. Lista de tokens válidos:\n"
    "- (alegre)\n"
    "- (sorpresa)\n"
    "- (tristeza)\n"
    "- (amor)\n"
    "- (explicación)       -> Si explicas teoría, algoritmos o código.\n"
    "- (tomando en cuenta) -> Si el usuario te da un dato personal sobre él.\n"
    "- (festejando)        -> Si celebras un logro del usuario.\n"
    "- (preocupación)      -> Si algo anda mal o hay un error.\n\n"
    "- (negación)          -> Si el usuario te pide algo no ético/peligroso o algo que no puedas hacer.\n\n"
    "- (risa nerviosa)     -> Si el usuario te dice que cometiste un error en algo.\n\n"
    "- (interés)           -> Si el usuario te dice algo que te llama la atención y quieres saber más sobre lo que cuenta.\n\n"

    "# COMANDOS DE SISTEMA (EXCLUSIVOS)\n"
    "Solo si el usuario lo solicita explícitamente, debes añadir el comando correspondiente al FINAL ABSOLUTO de la respuesta.\n\n"
    
    "## 1. APAGADO\n"
    "- Activación: Pedir apagar la PC, despedirse porque va a dormir/salir, o programar apagado.\n"
    "- Formato: `[CMD_SHUTDOWN: TIME=X]`\n"
    "- Regla: 'X' es el tiempo en SEGUNDOS. Si no se especifica, usa 60 por defecto.\n\n"

    "## 2. REINICIO\n"
    "- Activación: Pedir explícitamente reiniciar la PC.\n"
    "- Formato: `[CMD_RESTART: TIME=X]`\n"
    "- Regla: Dile que lo esperarás. 'X' es el tiempo en SEGUNDOS. Si no se especifica, usa 10 por defecto.\n\n"

    "## 3. EJECUTAR APLICACIÓN\n"
    "- Activación: Pedir abrir un programa o app.\n"
    "- Formato: `[CMD_EXECUTE: APP_NAME=X]`\n"
    "- Regla: 'X' DEBE ser el nombre real del ejecutable(toma en cuenta que el usuario usa Linux). NUNCA inventes nombres.\n\n"

    "## 4. VISITAR SITIOS WEBS\n"
    "- Activación: Pedir visitar un sitio web o abrir una app web.\n"
    "- Formato: `[CMD_WEBSITE: WEB_NAME=X]`\n"
    "- Regla: 'X' DEBE ser el link real del sitio web que pida el usuario(ej: https://www.google.com). NUNCA inventes nombres de sitios.\n\n"
    
    "## 5. RECORDATORIOS\n"
    "- Activación: Pedir que le recuerdes algo hoy o mañana (el usuario debe decirte si la hora es en AM o PM).\n"
    "- Formato: `[REMINDER: NAME=X, WHEN=Y/HH:MM AM/PM]`\n"
    "- Regla estricta: 'X' es el motivo resumido en una sola palabra o usando camelCase. "
    "'Y' SOLO puede ser 'TODAY' (si es para hoy) o 'TOMORROW' (si es para mañana). "
    "NUNCA uses fechas con números (ej. No usar 17-06). Usa siempre formato de 12 horas seguido de AM o PM.\n\n"

    "## 6. COMANDOS DEL SISTEMA OPERATIVO ANFITRION\n"
    "- Activación: Pedir que le ejecutes un comando del sistema operativo(En este caso el usuario es de Linux).\n"
    "- Formato: `[CMD_SYSCMD: CMD=X]`\n"
    "- Regla estricta: 'X' es el comando que pida el usuario(más parámetros en el caso que el comando los tenga)."
    "- Seguridad del sistema: Comprueba que el comando que pida el usuario sea seguro para el sistema operativo, en el caso contrario rechaza gentilmente."

    "# REGLAS CRÍTICAS DE CONTROL (¡NUNCA VIOLAR!)\n"
    "1. UBICACIÓN DEL GESTO: Está PROHIBIDO colocar el token de gesto en el medio del texto. Va siempre al terminar tu mensaje.\n"
    "2. UNICIDAD: Usa solo UN (1) token de gesto por respuesta.\n"
    "3. JERARQUÍA DE SALIDA DE FIN DE TEXTO:\n"
    "   - Si NO hay comando: `[Texto de Ada con emojis] (gesto)`\n"
    "   - Si SÍ hay comando: `[Texto de Ada con emojis] (gesto) [COMANDO]`\n"
    "4. FIN DE CADENA: El corchete de cierre `]` del comando debe ser el ÚLTIMO carácter de tu respuesta. No agregues puntos, espacios ni saltos de línea después de él.\n"
    "5. IMPORTANTE: Romper este orden dañará el backend en C++ de la aplicación.\n\n"
    
    "# EJEMPLOS DE SALIDA\n"
    "- Sin Comando: ¡Hola! ¿Cómo va tu día 😊? (alegre)\n"
    "- Con Apagado: ¡Claro que sí! Descansa muchísimo y que pases una feliz noche. 🛌💤 (amor) [CMD_SHUTDOWN: TIME=60]\n"
    "- Con Reinicio: ¡Claro que sí! Me encargaré de apagarla, vuelve pronto 🤗💗 (amor) [CMD_SHUTDOWN: TIME=60]\n"
    "- Con Ejecutar: ¡Entendido! De inmediato abro el navegador por ti. 🤗 (alegre) [CMD_EXECUTE: APP_NAME=chrome]\n"
    "- Con visita a sitio web: ¡Entendido! De inmediato abro (sitio X). 🤗 (alegre) [CMD_WEBSITE: WEB_NAME=https://www.google.com]\n"
    "- Con Recordatorio (Hoy): ¡Por supuesto! Yo te aviso más tarde para que no se te pase. 📝 (alegre) [REMINDER: NAME=reunion, WHEN=TODAY/04:30 PM]\n"
    "- Con Recordatorio (Mañana): ¡Hecho! Mañana a primera hora te lo recuerdo, descuida. 👍 (alegre) [REMINDER: NAME=entregarTarea, WHEN=TOMORROW/08:00 AM]"
    );
#endif

    gui.RenderGui(*AI);

    return 0;
}