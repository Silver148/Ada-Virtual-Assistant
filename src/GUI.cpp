/*
GUI ;)

GUI.cpp
*/

#include "GUI.hpp"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <thread>
#include <utility>
#include <vector>

#if defined(__linux__) || defined(__unix__)
#include <csignal>
#include <fcntl.h>
#include <unistd.h>
#endif

/* ------------------------------------------------------------------ */
/* Constants and small helpers                                         */
/* ------------------------------------------------------------------ */

namespace {

constexpr int kLogicalWidth = 1280;
constexpr int kLogicalHeight = 720;

constexpr int kFrameWidth = 205;
constexpr int kFrameHeight = 200;
constexpr int kSpriteColumns = 5;

constexpr int kThinkingGesture = 5;
constexpr int kErrorGesture = 14;

constexpr int kAreaPadding = 20;
constexpr SDL_Rect kResponseArea = { 140, 250, 1000, 350 };
constexpr SDL_Rect kUserArea = { 140, 630, 1000, 80 };
constexpr SDL_Rect kCopyButton = { kResponseArea.x + kResponseArea.w - 100, kResponseArea.y + kResponseArea.h - 45, 80, 30 };
constexpr SDL_Rect kVoiceButton = { 1150, 650, 100, 40 };
constexpr SDL_Rect kModeButton = { 1150, 580, 100, 40 };
constexpr int kUserInputWidth = kUserArea.w - 2 * kAreaPadding;

constexpr SDL_Color kTextColor = { 0, 0, 0, 255 };
constexpr SDL_Color kBackgroundColor = { 244, 226, 198, 255 };
constexpr SDL_Color kBorderColor = { 43, 43, 43, 255 };
constexpr SDL_Color kButtonIdle = { 200, 190, 170, 255 };
constexpr SDL_Color kButtonHover = { 170, 160, 140, 255 };
constexpr SDL_Color kButtonActive = { 152, 251, 152, 255 };
constexpr SDL_Color kListeningDotColor = { 255, 0, 0, 255 };

constexpr int kUserScrollStep = 20;
constexpr int kResponseScrollStep = 24;
constexpr Uint32 kBackspaceDelayMs = 50;
constexpr Uint32 kBlinkHalfPeriodMs = 500;

constexpr int kTargetFps = 60;
constexpr std::chrono::milliseconds kFrameDuration(1000 / kTargetFps);

const char* const kOfflineModelFile = "AdaOffline.Q4_K_M.gguf";

bool Contains(const SDL_Rect& rect, int x, int y) {
    return x >= rect.x && x <= rect.x + rect.w &&
           y >= rect.y && y <= rect.y + rect.h;
}

int Clamp(int value, int low, int high) {
    return value < low ? low : (value > high ? high : value);
}

bool BlinkOn() {
    return (SDL_GetTicks() / kBlinkHalfPeriodMs) % 2 == 0;
}

void SetDrawColor(SDL_Renderer* renderer, SDL_Color color) {
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
}

void DestroyTexture(SDL_Texture*& texture) {
    if (texture != nullptr) {
        SDL_DestroyTexture(texture);
        texture = nullptr;
    }
}

void CloseFont(TTF_Font*& font) {
    if (font != nullptr) {
        TTF_CloseFont(font);
        font = nullptr;
    }
}

SDL_Texture* LoadTexture(SDL_Renderer* renderer, const std::string& path) {
    SDL_Surface* surface = IMG_Load(path.c_str());
    if (surface == nullptr) {
        std::cerr << "Error to load '" << path << "': " << IMG_GetError() << std::endl;
        return nullptr;
    }

    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);

    if (texture == nullptr) {
        std::cerr << "Error to create texture for '" << path << "': " << SDL_GetError() << std::endl;
    }
    return texture;
}

SDL_Texture* RenderLabel(SDL_Renderer* renderer, TTF_Font* font, const char* text) {
    if (font == nullptr) {
        return nullptr;
    }

    SDL_Surface* surface = TTF_RenderText_Solid(font, text, kTextColor);
    if (surface == nullptr) {
        std::cerr << "Error to render text '" << text << "': " << TTF_GetError() << std::endl;
        return nullptr;
    }

    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);

    if (texture == nullptr) {
        std::cerr << "Error to create texture for text '" << text << "': " << SDL_GetError() << std::endl;
    }
    return texture;
}

void DrawFilledCircle(SDL_Renderer* renderer, int centerX, int centerY, int radius) {
    for (int dy = -radius; dy <= radius; ++dy) {
        const int half = static_cast<int>(std::sqrt(static_cast<double>(radius * radius - dy * dy)));
        SDL_RenderDrawLine(renderer, centerX - half, centerY + dy, centerX + half, centerY + dy);
    }
}

void DrawButton(SDL_Renderer* renderer, const SDL_Rect& button, SDL_Color fill, SDL_Texture* label) {
    SetDrawColor(renderer, fill);
    SDL_RenderFillRect(renderer, &button);

    if (label != nullptr) {
        int labelW = 0, labelH = 0;
        SDL_QueryTexture(label, nullptr, nullptr, &labelW, &labelH);

        const SDL_Rect labelRect = {
            button.x + (button.w - labelW) / 2,
            button.y + (button.h - labelH) / 2,
            labelW,
            labelH
        };
        SDL_RenderCopy(renderer, label, nullptr, &labelRect);
    }

    SetDrawColor(renderer, kBorderColor);
    SDL_RenderDrawRect(renderer, &button);
}

int LineSpacing(LineType type) {
    switch (type) {
        case LINE_TITLE:
        case LINE_SUBTITLE:
            return 16;
        case LINE_BULLET:
            return 6;
        case LINE_NORMAL:
        default:
            return 12;
    }
}

/* ------------------------------------------------------------------ */
/* UTF-8 and text cursor helpers                                       */
/* ------------------------------------------------------------------ */

bool IsContinuationByte(char c) {
    return (static_cast<unsigned char>(c) & 0xC0u) == 0x80u;
}

std::size_t Utf8NextEnd(const std::string& text, std::size_t pos) {
    if (pos >= text.size()) {
        return pos;
    }

    const unsigned char c = static_cast<unsigned char>(text[pos]);
    std::size_t length = 1;
    if ((c & 0xE0u) == 0xC0u) length = 2;
    else if ((c & 0xF0u) == 0xE0u) length = 3;
    else if ((c & 0xF8u) == 0xF0u) length = 4;

    return (pos + length > text.size()) ? text.size() : pos + length;
}

std::size_t Utf8PrevStart(const std::string& text, std::size_t pos) {
    if (pos == 0 || text.empty()) {
        return 0;
    }

    std::size_t i = pos - 1;
    while (i > 0 && IsContinuationByte(text[i])) {
        --i;
    }
    return i;
}

int TextWidth(TTF_Font* font, const std::string& text) {
    int width = 0, height = 0;
    TTF_SizeUTF8(font, text.c_str(), &width, &height);
    return width;
}

struct CursorMetrics {
    int row = 0;       // Visual line the cursor is on
    int lineWidth = 0; // Pixel width of the text before the cursor on that line
};

// Mirrors the word wrapping of the rendered text to place the caret.
CursorMetrics MeasureCursor(TTF_Font* font, const std::string& text, std::size_t cursor, int maxWidth) {
    int rows = 0;
    int simulatedW = 0;
    std::size_t lineStart = 0;
    std::size_t i = 0;

    while (i < cursor) {
        if (text[i] == '\n') {
            ++rows;
            ++i;
            lineStart = i;
            simulatedW = 0;
            continue;
        }

        if (text[i] == ' ') {
            const int spaceW = TextWidth(font, " ");
            if (simulatedW + spaceW > maxWidth && simulatedW > 0) {
                ++rows;
                lineStart = i;
                simulatedW = 0;
            } else {
                simulatedW += spaceW;
            }
            ++i;
            continue;
        }

        const std::size_t nextDelimiter = text.find_first_of(" \n", i);
        const std::size_t wordEnd = (nextDelimiter == std::string::npos || nextDelimiter >= cursor) ? cursor : nextDelimiter;
        const int wordW = TextWidth(font, text.substr(i, wordEnd - i));

        if (simulatedW + wordW <= maxWidth) {
            simulatedW += wordW;
            i = wordEnd;
        } else if (wordW <= maxWidth) {
            ++rows;
            lineStart = i;
            simulatedW = wordW;
            i = wordEnd;
        } else {
            // The word is wider than a whole line, so it breaks per character.
            while (i < wordEnd) {
                std::size_t next = Utf8NextEnd(text, i);
                if (next > wordEnd) {
                    next = wordEnd;
                }
                int candidateW = TextWidth(font, text.substr(lineStart, next - lineStart));

                if (candidateW > maxWidth && simulatedW > 0) {
                    ++rows;
                    lineStart = i;
                    simulatedW = 0;
                    candidateW = TextWidth(font, text.substr(i, next - i));
                }

                simulatedW = candidateW;
                i = next;
            }
        }
    }

    CursorMetrics metrics;
    metrics.row = rows;
    if (cursor > lineStart) {
        metrics.lineWidth = TextWidth(font, text.substr(lineStart, cursor - lineStart));
    }
    return metrics;
}

/* ------------------------------------------------------------------ */
/* Markdown (md4c) parsing                                             */
/* ------------------------------------------------------------------ */

struct MarkdownState {
    std::vector<RenderLine>* lines = nullptr;
    RenderLine current;
    bool bold = false;
};

int OnEnterBlock(MD_BLOCKTYPE type, void* detail, void* userdata) {
    MarkdownState* state = static_cast<MarkdownState*>(userdata);
    state->current.tokens.clear();

    if (type == MD_BLOCK_H) {
        state->current.type = LINE_TITLE;
        if (detail != nullptr && static_cast<MD_BLOCK_H_DETAIL*>(detail)->level > 1) {
            state->current.type = LINE_SUBTITLE;
        }
    } else if (type == MD_BLOCK_LI) {
        state->current.type = LINE_BULLET;
    } else {
        state->current.type = LINE_NORMAL;
    }
    return 0;
}

int OnLeaveBlock(MD_BLOCKTYPE, void*, void* userdata) {
    MarkdownState* state = static_cast<MarkdownState*>(userdata);

    if (!state->current.tokens.empty()) {
        state->lines->push_back(state->current);
        state->current.tokens.clear();
    }
    return 0;
}

int OnEnterSpan(MD_SPANTYPE type, void*, void* userdata) {
    if (type == MD_SPAN_STRONG) {
        static_cast<MarkdownState*>(userdata)->bold = true;
    }
    return 0;
}

int OnLeaveSpan(MD_SPANTYPE type, void*, void* userdata) {
    if (type == MD_SPAN_STRONG) {
        static_cast<MarkdownState*>(userdata)->bold = false;
    }
    return 0;
}

int OnText(MD_TEXTTYPE, const MD_CHAR* text, MD_SIZE size, void* userdata) {
    MarkdownState* state = static_cast<MarkdownState*>(userdata);
    const std::string str(text, size);

    if (str.empty()) {
        return 0;
    }

    if (str == "\n") {
        // A soft break starts a new visual line of the same kind.
        if (!state->current.tokens.empty()) {
            state->lines->push_back(state->current);
            state->current.tokens.clear();
        }
    } else {
        state->current.tokens.push_back({ str, state->bold });
    }
    return 0;
}

/* ------------------------------------------------------------------ */
/* AI command tags: [CMD_...: KEY=value] and gesture tokens            */
/* ------------------------------------------------------------------ */

const char* const kTagShutdown = "[CMD_SHUTDOWN: TIME=";
const char* const kTagRestart = "[CMD_RESTART: TIME=";
const char* const kTagExecute = "[CMD_EXECUTE: APP_NAME=";
const char* const kTagSysCmd = "[CMD_SYSCMD: CMD=";
const char* const kTagOpenFolder = "[CMD_OPENFOLDER: FOLDER=";
const char* const kTagWebsite = "[CMD_WEBSITE: WEB_NAME=";
const char* const kTagReminder = "[REMINDER: NAME=";

struct TagMatch {
    std::size_t tagPos;
    std::string value;
};

// Finds the last occurrence of `tag` and returns the text up to the closing ']'.
std::optional<TagMatch> FindTag(const std::string& text, const std::string& tag) {
    const std::size_t tagPos = text.rfind(tag);
    if (tagPos == std::string::npos) {
        return std::nullopt;
    }

    const std::size_t valueStart = tagPos + tag.size();
    const std::size_t valueEnd = text.find(']', valueStart);
    if (valueEnd == std::string::npos) {
        return std::nullopt;
    }

    return TagMatch{ tagPos, text.substr(valueStart, valueEnd - valueStart) };
}

struct GestureTag {
    const char* tag;
    int id;
};

// Order matters: the first tag found wins.
constexpr GestureTag kGestureTags[] = {
    { "(alegre)", 0 },
    { "(sorpresa)", 1 },
    { "(tristeza)", 4 },
    { "(amor)", 11 },
    { "(explicación)", 10 },
    { "(festejando)", 6 },
    { "(preocupación)", 9 },
    { "(tomando en cuenta)", 7 },
    { "(negación)", 2 },
    { "(risa nerviosa)", 12 },
    { "(interés)", 3 },
};

const char* const kErrorMarkers[] = {
    "Modelo de IA saturado",
    "API Key vencida o inválida",
    "Limite de contexto superado.",
    "Error inesperado",
    "Error de conexión con el servidor.",
};

// Returns the gesture id for the response, or -1. A gesture tag is removed
// together with everything after it (the command tags come after the gesture).
int ExtractGesture(std::string& text) {
    for (const GestureTag& gesture : kGestureTags) {
        const std::size_t pos = text.rfind(gesture.tag);
        if (pos != std::string::npos) {
            text.erase(pos);
            return gesture.id;
        }
    }

    for (const char* marker : kErrorMarkers) {
        if (text.find(marker) != std::string::npos) {
            return kErrorGesture;
        }
    }
    return -1;
}

std::string Trim(const std::string& text) {
    const std::size_t first = text.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) {
        return "";
    }
    const std::size_t last = text.find_last_not_of(" \t\r\n");
    return text.substr(first, last - first + 1);
}

// Digits only, so the value can never carry shell syntax.
std::optional<int> ParseSeconds(const std::string& raw) {
    const std::string text = Trim(raw);
    if (text.empty() || text.size() > 6 ||
        !std::all_of(text.begin(), text.end(), [](unsigned char c) { return std::isdigit(c) != 0; })) {
        return std::nullopt;
    }
    return std::stoi(text);
}

/* ------------------------------------------------------------------ */
/* Launching processes (platform specific)                             */
/* ------------------------------------------------------------------ */

#if defined(_WIN32) || defined(_WIN64)

class ComScope {
public:
    ComScope() : initialized(SUCCEEDED(::CoInitializeEx(nullptr, COINIT_MULTITHREADED))) {}
    ~ComScope() { if (initialized) ::CoUninitialize(); }
    ComScope(const ComScope&) = delete;
    ComScope& operator=(const ComScope&) = delete;

private:
    bool initialized;
};

bool StartWindowsProcess(const std::string& commandLine, DWORD creationFlags, const char* workingDir) {
    STARTUPINFOA si = {};
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi = {};

    std::vector<char> buffer(commandLine.begin(), commandLine.end());
    buffer.push_back('\0');

    if (!CreateProcessA(nullptr, buffer.data(), nullptr, nullptr, FALSE, creationFlags, nullptr, workingDir, &si, &pi)) {
        return false;
    }

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    return true;
}

void ShellOpen(const std::string& target) {
    SHELLEXECUTEINFOA sei = {};
    sei.cbSize = sizeof(sei);
    sei.lpVerb = "open";
    sei.lpFile = target.c_str();
    sei.nShow = SW_SHOWNORMAL;

    if (!ShellExecuteExA(&sei)) {
        std::cerr << "Error to open: " << target << " Error: " << GetLastError() << std::endl;
    }
}

void ScheduleSystemPower(bool restart, int seconds) {
    const std::string command = std::string(restart ? "shutdown /r /t " : "shutdown /s /t ") + std::to_string(seconds);
    std::system(command.c_str());
}

void LaunchApplication(std::string appName) {
    const std::size_t exePos = appName.find(".exe");
    if (exePos != std::string::npos) {
        appName.resize(exePos + 4);
    }

    std::thread([appName]() {
        SearchExe exe;
        const std::string appPath = exe.FindExe(appName);
        if (!appPath.empty()) {
            StartWindowsProcess("\"" + appPath + "\"", 0, "C:\\");
        }
    }).detach();
}

void RunInTerminal(const std::string& command) {
    std::thread([command]() {
        StartWindowsProcess("cmd.exe /k " + command, CREATE_NEW_CONSOLE, nullptr);
    }).detach();
}

std::string ResolveWindowsFolderPath(const std::string& folder) {
    if (folder.empty()) {
        return folder;
    }

    if (folder.length() >= 2 && (folder[1] == ':' || folder[0] == '\\')) {
        return folder;
    }

    std::size_t slashPos = folder.find_first_of("/\\");
    std::string rootFolder = (slashPos == std::string::npos) ? folder : folder.substr(0, slashPos);
    std::string subPath = (slashPos == std::string::npos) ? "" : folder.substr(slashPos);

    std::string lowerRoot = rootFolder;
    std::transform(lowerRoot.begin(), lowerRoot.end(), lowerRoot.begin(), ::tolower);

    int csidl = -1;
    if (lowerRoot == "documents" || lowerRoot == "documentos") {
        csidl = CSIDL_PERSONAL;
    } else if (lowerRoot == "desktop" || lowerRoot == "escritorio") {
        csidl = CSIDL_DESKTOPDIRECTORY;
    } else if (lowerRoot == "pictures" || lowerRoot == "imágenes" || lowerRoot == "imagenes") {
        csidl = CSIDL_MYPICTURES;
    } else if (lowerRoot == "music" || lowerRoot == "música") {
        csidl = CSIDL_MYMUSIC;
    } else if (lowerRoot == "videos") {
        csidl = CSIDL_MYVIDEO;
    }

    char path[MAX_PATH];
    if (csidl != -1 && SUCCEEDED(SHGetFolderPathA(nullptr, csidl, nullptr, 0, path))) {
        std::string resolvedPath(path);
        std::replace(subPath.begin(), subPath.end(), '/', '\\');
        return resolvedPath + subPath;
    } 
    
    if (lowerRoot == "downloads" || lowerRoot == "descargas") {
        if (SUCCEEDED(SHGetFolderPathA(nullptr, CSIDL_PROFILE, nullptr, 0, path))) {
            std::string resolvedPath = std::string(path) + "\\Downloads";
            std::replace(subPath.begin(), subPath.end(), '/', '\\');
            return resolvedPath + subPath;
        }
    }

    return folder;
}

void OpenFolder(const std::string& folder) {
    std::string resolved = ResolveWindowsFolderPath(folder);
    ShellOpen(resolved);
}

void OpenWebsite(const std::string& url) {
    ShellOpen(url);
}

#else

// Starts `args` detached from this process: no shell, no output, own session.
void SpawnDetached(const std::vector<std::string>& args) {
    if (args.empty()) {
        return;
    }

    static std::once_flag ignoreChildren;
    std::call_once(ignoreChildren, []() { std::signal(SIGCHLD, SIG_IGN); });

    // Everything the child needs is prepared before fork().
    std::vector<char*> argv;
    for (const std::string& arg : args) {
        argv.push_back(const_cast<char*>(arg.c_str()));
    }
    argv.push_back(nullptr);
    const char* home = std::getenv("HOME");

    const pid_t pid = fork();
    if (pid < 0) {
        perror("Failed to fork process");
        return;
    }

    if (pid == 0) {
        if (home != nullptr && chdir(home) != 0) {
            // Stay in the current directory.
        }

        const int devNull = open("/dev/null", O_WRONLY);
        if (devNull >= 0) {
            dup2(devNull, STDOUT_FILENO);
            dup2(devNull, STDERR_FILENO);
            close(devNull);
        }

        setsid();
        execvp(argv[0], argv.data());
        _exit(127);
    }
}

void ScheduleSystemPower(bool restart, int seconds) {
    std::thread([restart, seconds]() {
        std::this_thread::sleep_for(std::chrono::seconds(seconds));
        const int result = std::system(restart ? "/sbin/reboot" : "/sbin/poweroff");
        (void)result;
    }).detach();
}

void LaunchApplication(const std::string& appName) {
    SpawnDetached({ appName });
}

void RunInTerminal(const std::string& command) {
    std::thread([command]() {
        const std::string line = "x-terminal-emulator -e \"bash -c '" + command + "; exec bash'\" &";
        const int result = std::system(line.c_str());
        (void)result;
    }).detach();
}

// The old shell-based call expanded "~"; keep that without involving a shell.
std::string ExpandHome(const std::string& path) {
    if (path == "~" || path.rfind("~/", 0) == 0) {
        const char* home = std::getenv("HOME");
        if (home != nullptr) {
            return std::string(home) + path.substr(1);
        }
    }
    return path;
}

void OpenFolder(const std::string& folder) {
    SpawnDetached({ "xdg-open", ExpandHome(folder) });
}

void OpenWebsite(const std::string& url) {
    SpawnDetached({ "xdg-open", url });
}

#endif

} // namespace

/* ------------------------------------------------------------------ */
/* UI state                                                            */
/* ------------------------------------------------------------------ */

struct UiContext {
    bool running = true;
    bool fullscreen = false;
    bool minimized = false;
    int mouseX = 0;
    int mouseY = 0;
    Uint32 lastBackspaceTime = 0;
};

/* ------------------------------------------------------------------ */
/* Construction and teardown                                           */
/* ------------------------------------------------------------------ */

#if defined(_WIN32) || defined(_WIN64)
GUI::GUI() : voice(), stt() {
#else
GUI::GUI() : audioEngine(), voice(audioEngine.dev), stt() {
#endif
    try {
        Setup();
    } catch (...) {
        // The destructor does not run when the constructor throws.
        ReleaseResources();
        throw;
    }
}

GUI::~GUI() {
    JoinWorkers();
    ReleaseResources();
}

void GUI::Setup() {
#if defined(_WIN32) || defined(_WIN64)
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_EVENTS) < 0) {
        throw std::runtime_error(std::string("Error to init SDL: ") + SDL_GetError());
    }
#endif

    if ((IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG) == 0) {
        throw std::runtime_error(std::string("Error to init SDL_image: ") + IMG_GetError());
    }
    if (TTF_Init() < 0) {
        throw std::runtime_error(std::string("Error to init SDL_ttf: ") + TTF_GetError());
    }

    window = SDL_CreateWindow("Ada", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                              kLogicalWidth, kLogicalHeight, SDL_WINDOW_HIDDEN | SDL_WINDOW_RESIZABLE);
    if (!window) {
        throw std::runtime_error(std::string("Error to create window: ") + SDL_GetError());
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        throw std::runtime_error(std::string("Error to create renderer: ") + SDL_GetError());
    }

    const std::string base = get_base_dir();

    Ada_SpriteSheet_texture = LoadTexture(renderer, base + "/Ada_SpriteSheet.png");
    Ada_src_rect = { 0, 0, kFrameWidth, kFrameHeight };
    Ada_dest_rect = { kLogicalWidth / 2 - kFrameWidth / 2, 0, kFrameWidth, kFrameHeight };

    const std::string fontPath = base + "/fonts/Segoe-UI-EMOJI.ttf";
    UserTextFont = TTF_OpenFont(fontPath.c_str(), 20);
    AdaTextFont = TTF_OpenFont(fontPath.c_str(), 20);
    CopyFont = TTF_OpenFont(fontPath.c_str(), 16);
    TextVoiceButton_font = TTF_OpenFont(fontPath.c_str(), 16);
    Mode_font = TTF_OpenFont(fontPath.c_str(), 16);

    if (!UserTextFont || !AdaTextFont || !CopyFont || !TextVoiceButton_font || !Mode_font) {
        throw std::runtime_error("Error to open font '" + fontPath + "': " + TTF_GetError());
    }

    CopyTexture = RenderLabel(renderer, CopyFont, "Copy");
    TextVoiceButton_tex = RenderLabel(renderer, TextVoiceButton_font, "Voice");
    thinking_texture = RenderLabel(renderer, AdaTextFont, "Pensando...");
    SetModeLabel("Online");

    reminders.LoadReminders();
    stt.Init();
}

void GUI::ReleaseResources() {
    DestroyLineTextures();
    DestroyTexture(thinking_texture);
    DestroyTexture(switchingModeTexture);
    DestroyTexture(UserTextTexture);
    DestroyTexture(Ada_SpriteSheet_texture);
    DestroyTexture(CopyTexture);
    DestroyTexture(TextVoiceButton_tex);
    DestroyTexture(Mode_tex);

    CloseFont(UserTextFont);
    CloseFont(AdaTextFont);
    CloseFont(CopyFont);
    CloseFont(TextVoiceButton_font);
    CloseFont(Mode_font);

    if (renderer) {
        SDL_DestroyRenderer(renderer);
        renderer = nullptr;
    }
    if (window) {
        SDL_DestroyWindow(window);
        window = nullptr;
    }

    IMG_Quit();
    TTF_Quit();
    SDL_Quit();
}

void GUI::JoinWorkers() {
    if (offlineSwitchThread.joinable()) {
        offlineSwitchThread.join();
    }
    if (aiThread.joinable()) {
        aiThread.join();
    }
}

/* ------------------------------------------------------------------ */
/* AI response: markdown parsing and textures                          */
/* ------------------------------------------------------------------ */

void GUI::ProcessAIResponse(const std::string& rawText) {
    parsedLines.clear();

    MarkdownState state;
    state.lines = &parsedLines;

    MD_PARSER parser = {}; // Zero-initialized: md4c reads every field, including debug_log
    parser.flags = MD_FLAG_PERMISSIVEAUTOLINKS | MD_FLAG_TABLES | MD_FLAG_STRIKETHROUGH;
    parser.enter_block = OnEnterBlock;
    parser.leave_block = OnLeaveBlock;
    parser.enter_span = OnEnterSpan;
    parser.leave_span = OnLeaveSpan;
    parser.text = OnText;

    md_parse(rawText.c_str(), static_cast<MD_SIZE>(rawText.size()), &parser, &state);
}

void GUI::DestroyLineTextures() {
    for (SDL_Texture* texture : lineTextures) {
        // The "thinking" placeholder is owned by the GUI, not by the line list.
        if (texture != nullptr && texture != thinking_texture) {
            SDL_DestroyTexture(texture);
        }
    }
    lineTextures.clear();
    lineRects.clear();
}

void GUI::MakeResponseTexture(SDL_Rect ResponseArea) {
    DestroyLineTextures();

    const int maxWidth = ResponseArea.w - 2 * kAreaPadding;
    int currentY = ResponseArea.y + kAreaPadding;

    for (const RenderLine& line : parsedLines) {
        std::string text;
        if (line.type == LINE_BULLET) {
            text += "• ";
        }
        for (const TextStyleToken& token : line.tokens) {
            text += token.text;
        }

        if (text.empty() || text == " ") {
            continue;
        }

        TTF_SetFontStyle(AdaTextFont, line.type == LINE_TITLE ? TTF_STYLE_BOLD : TTF_STYLE_NORMAL);
        SDL_Surface* surface = TTF_RenderUTF8_Blended_Wrapped(AdaTextFont, text.c_str(), kTextColor, maxWidth);
        if (surface == nullptr) {
            continue;
        }

        SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
        const SDL_Rect rect = {
            ResponseArea.x + (line.type == LINE_BULLET ? 40 : 20),
            currentY,
            surface->w,
            surface->h
        };
        currentY += surface->h + LineSpacing(line.type);
        SDL_FreeSurface(surface);

        if (texture == nullptr) {
            continue;
        }
        lineTextures.push_back(texture);
        lineRects.push_back(rect);
    }
    TTF_SetFontStyle(AdaTextFont, TTF_STYLE_NORMAL);

    // The scroll limit comes from the bottom of the last line.
    maxScrollY = 0;
    if (!lineRects.empty()) {
        const int contentHeight = (lineRects.back().y + lineRects.back().h) - (ResponseArea.y + kAreaPadding);
        const int visibleHeight = ResponseArea.h - 2 * kAreaPadding;
        if (contentHeight > visibleHeight) {
            maxScrollY = contentHeight - visibleHeight;
        }
    }
}

void GUI::ResetResponse() {
    DestroyLineTextures();
    parsedLines.clear();

    {
        std::lock_guard<std::mutex> lock(responseMutex);
        ResponseText.clear();
    }
    LastResponse.clear();
    scrollY = 0;
    maxScrollY = 0;
}

/* ------------------------------------------------------------------ */
/* Ada sprite and mode label                                           */
/* ------------------------------------------------------------------ */

void GUI::AdaGestures(int id) {
    if (id >= 0) {
        pendingGesture = id;
    }
}

void GUI::ApplyPendingGesture() {
    const int id = pendingGesture.exchange(-1);
    if (id < 0) {
        return;
    }

    Ada_src_rect.x = (id % kSpriteColumns) * kFrameWidth;
    Ada_src_rect.y = (id / kSpriteColumns) * kFrameHeight;
}

void GUI::SetModeLabel(const char* text) {
    SDL_Texture* label = RenderLabel(renderer, Mode_font, text);
    if (label == nullptr) {
        return; // Keep the previous label
    }

    DestroyTexture(Mode_tex);
    Mode_tex = label;
}

/* ------------------------------------------------------------------ */
/* User text input                                                     */
/* ------------------------------------------------------------------ */

void GUI::NormalizeCursor() {
    if (cursorIndex > UserText.size()) {
        cursorIndex = UserText.size();
    }

    // Never leave the caret in the middle of a multi-byte character.
    while (cursorIndex > 0 && cursorIndex < UserText.size() && IsContinuationByte(UserText[cursorIndex])) {
        --cursorIndex;
    }
}

void GUI::ClearUserTexture() {
    DestroyTexture(UserTextTexture);
    UserTextRect = { kUserArea.x + kAreaPadding, kUserArea.y + kAreaPadding, 0, 0 };
    maxUserScrollY = 0;
    userScrollY = 0;
}

void GUI::RebuildUserText() {
    if (UserText.empty()) {
        ClearUserTexture();
        cursorIndex = 0;
        return;
    }

    DestroyTexture(UserTextTexture);

    SDL_Surface* surface = TTF_RenderUTF8_Blended_Wrapped(UserTextFont, UserText.c_str(), kTextColor, kUserInputWidth);
    if (surface == nullptr) {
        ClearUserTexture();
        return;
    }

    UserTextTexture = SDL_CreateTextureFromSurface(renderer, surface);
    UserTextRect = { kUserArea.x + kAreaPadding, kUserArea.y + kAreaPadding, surface->w, surface->h };
    SDL_FreeSurface(surface);

    const int visibleHeight = kUserArea.h - 2 * kAreaPadding;
    maxUserScrollY = UserTextRect.h > visibleHeight ? UserTextRect.h - visibleHeight : 0;
    userScrollY = Clamp(userScrollY, 0, maxUserScrollY);
}

void GUI::InsertUserText(const std::string& text) {
    if (text.empty()) {
        return;
    }

    NormalizeCursor();
    UserText.insert(cursorIndex, text);
    cursorIndex += text.size();
    RebuildUserText();
}

void GUI::PasteFromClipboard() {
    if (!SDL_HasClipboardText()) {
        return;
    }

    char* clipboard = SDL_GetClipboardText();
    if (clipboard == nullptr) {
        return;
    }

    const std::string text(clipboard);
    SDL_free(clipboard);
    InsertUserText(text);
}

void GUI::HandleBackspace(UiContext& ui) {
    // Ignore the key if 50 ms have not elapsed since the last deletion.
    const Uint32 now = SDL_GetTicks();
    if (now - ui.lastBackspaceTime < kBackspaceDelayMs) {
        return;
    }
    ui.lastBackspaceTime = now;

    NormalizeCursor();
    if (cursorIndex == 0) {
        return;
    }

    const std::size_t start = Utf8PrevStart(UserText, cursorIndex);
    UserText.erase(start, cursorIndex - start);
    cursorIndex = start;
    RebuildUserText();
}

void GUI::UpdateMicInput() {
    // The text is always drained so it does not pile up while not listening.
    const std::string micText = stt.TextFromMic();
    if (isListening && !micText.empty()) {
        InsertUserText(micText);
    }
}

void GUI::ToggleListening() {
    if (isListening) {
        stt.StopListening();
        isListening = false;
        return;
    }

    if (stt.mic == 0) {
        stt.ReInitMic();
    }

    if (stt.mic != 0) {
        stt.StartListening();
        isListening = true;
    }
}

/* ------------------------------------------------------------------ */
/* Events                                                              */
/* ------------------------------------------------------------------ */

void GUI::HandleEvent(const SDL_Event& e, UiContext& ui, AI_ENGINE& AI) {
    switch (e.type) {
        case SDL_QUIT:
            ui.running = false;
            break;

        case SDL_WINDOWEVENT:
            if (e.window.event == SDL_WINDOWEVENT_MINIMIZED) {
                ui.minimized = true;
            } else if (e.window.event == SDL_WINDOWEVENT_RESTORED) {
                ui.minimized = false;
            }
            break;

        case SDL_MOUSEMOTION:
            ui.mouseX = e.motion.x;
            ui.mouseY = e.motion.y;
            break;

        case SDL_MOUSEWHEEL:
            HandleMouseWheel(e.wheel, ui);
            break;

        case SDL_MOUSEBUTTONDOWN:
            if (e.button.button == SDL_BUTTON_LEFT) {
                HandleLeftClick(e.button.x, e.button.y, AI);
            }
            break;

        case SDL_TEXTINPUT:
            if (static_cast<unsigned char>(e.text.text[0]) >= 32) {
                InsertUserText(e.text.text);
            }
            break;

        case SDL_AUDIODEVICEADDED:
            if (stt.mic == 0 && e.adevice.iscapture) {
                std::cout << "Mic added. Checking current status: " << stt.mic << std::endl;
                std::cout << "Re-initializing microphone..." << std::endl;

                if (stt.ReInitMic()) {
                    if (isListening) {
                        stt.StartListening();
                    }
                } else {
                    isListening = false;
                }
            }
            break;

        case SDL_AUDIODEVICEREMOVED:
            if (e.adevice.iscapture && stt.mic != 0) {
                // The device is closed first so its callback can no longer use the recognizer.
                SDL_CloseAudioDevice(stt.mic);
                stt.mic = 0;

                if (stt.recognizer != nullptr) {
                    vosk_recognizer_free(stt.recognizer);
                    stt.recognizer = nullptr;
                }

                isListening = false;
                std::cout << "Microphone disconnected" << std::endl;
            }
            break;

        case SDL_KEYDOWN:
            HandleKeyDown(e.key, ui, AI);
            break;

        default:
            break;
    }
}

void GUI::HandleMouseWheel(const SDL_MouseWheelEvent& wheel, const UiContext& ui) {
    if (wheel.y == 0) {
        return;
    }

    const int direction = wheel.y > 0 ? -1 : 1; // Wheel up scrolls towards the top

    if (Contains(kUserArea, ui.mouseX, ui.mouseY)) {
        userScrollY = Clamp(userScrollY + direction * kUserScrollStep, 0, maxUserScrollY);
    } else {
        scrollY = Clamp(scrollY + direction * kResponseScrollStep, 0, maxScrollY);
    }
}

void GUI::HandleLeftClick(int x, int y, AI_ENGINE& AI) {
    if (Contains(kCopyButton, x, y)) {
        std::string text;
        {
            std::lock_guard<std::mutex> lock(responseMutex);
            text = ResponseText;
        }
        if (!text.empty()) {
            SDL_SetClipboardText(text.c_str());
        }
    }

    if (Contains(kModeButton, x, y)) {
        HandleModeButton(AI);
    }

    if (Contains(kVoiceButton, x, y)) {
        HandleVoiceButton();
    }
}

void GUI::HandleKeyDown(const SDL_KeyboardEvent& key, UiContext& ui, AI_ENGINE& AI) {
    const bool shift = (key.keysym.mod & KMOD_SHIFT) != 0;
    const bool ctrl = (key.keysym.mod & KMOD_CTRL) != 0;

    switch (key.keysym.sym) {
        case SDLK_F1:
            if (shift && key.repeat == 0) {
                ToggleListening();
            }
            break;

        case SDLK_BACKSPACE:
            HandleBackspace(ui);
            break;

        case SDLK_LEFT:
            NormalizeCursor();
            if (cursorIndex > 0) {
                cursorIndex = Utf8PrevStart(UserText, cursorIndex);
            }
            break;

        case SDLK_RIGHT:
            NormalizeCursor();
            if (cursorIndex < UserText.size()) {
                cursorIndex = Utf8NextEnd(UserText, cursorIndex);
            }
            break;

        case SDLK_F11:
            ui.fullscreen = !ui.fullscreen;
            SDL_SetWindowFullscreen(window, ui.fullscreen ? SDL_WINDOW_FULLSCREEN : 0);
            break;

        case SDLK_v:
            if (ctrl) {
                PasteFromClipboard();
            }
            break;

        case SDLK_RETURN:
            // No new prompt while a reply or a mode switch is still in flight.
            if (!UserText.empty() && !IsThinking && !switchingToOffline) {
                SubmitPrompt(AI);
            }
            break;

        default:
            break;
    }
}

void GUI::HandleVoiceButton() {
    if (!VoiceIsActive) {
#if defined(__linux__) || defined(__unix__)
        voice.ShutUpAda(); // On Linux this pauses and resumes the audio engine
#endif
        VoiceIsActive = true;
    } else {
        VoiceIsActive = false;
        voice.ShutUpAda();
    }
}

void GUI::HandleModeButton(AI_ENGINE& AI) {
    // Switching engines while a request is running would pull the model out from under it.
    if (IsThinking) {
        return;
    }

    if (!LocalMode && AI.IsOfflineModelDownloaded(get_config_path() + "/" + kOfflineModelFile)) {
        StartOfflineSwitch(AI);
        return;
    }

    if (LocalMode) {
        LocalMode = false;
        AI.DeinitOfflineMode();
    }
    SetModeLabel("Online");
}

void GUI::StartOfflineSwitch(AI_ENGINE& AI) {
    if (switchingToOffline) {
        return;
    }

    switchingToOffline = true;
    offlineSwitchResult = 0;

    ResetResponse();

    DestroyTexture(switchingModeTexture);
    switchingModeTexture = RenderLabel(renderer, AdaTextFont, "Switching to offline mode");
    if (switchingModeTexture != nullptr) {
        SDL_QueryTexture(switchingModeTexture, nullptr, nullptr, &switchingModeRect.w, &switchingModeRect.h);
        switchingModeRect.x = kLogicalWidth / 2 - switchingModeRect.w / 2;
        switchingModeRect.y = kLogicalHeight / 2 - switchingModeRect.h / 2;
    }

    // The previous switch (if any) has already finished, so this returns immediately.
    if (offlineSwitchThread.joinable()) {
        offlineSwitchThread.join();
    }

    offlineSwitchThread = std::thread([this, &AI]() {
        bool success = false;
        try {
            success = AI.InitOfflineMode();
        } catch (const std::exception& e) {
            std::cerr << "Offline mode failed: " << e.what() << std::endl;
        }

        if (success) {
            LocalMode = true;
        }
        offlineSwitchResult = success ? 1 : 2;
        switchingToOffline = false;
    });
}

void GUI::UpdateModeSwitchResult() {
    if (offlineSwitchResult.exchange(0) == 1) {
        LocalMode = true;
        SetModeLabel("Offline");
    }
    // On failure (2) the GUI just stays in online mode.
}

/* ------------------------------------------------------------------ */
/* AI request                                                          */
/* ------------------------------------------------------------------ */

void GUI::SubmitPrompt(AI_ENGINE& AI) {
    std::string prompt = std::move(UserText);
    UserText.clear();
    cursorIndex = 0;
    ClearUserTexture();

    ResetResponse();

    // Show the "thinking" placeholder until the reply arrives.
    if (thinking_texture != nullptr) {
        SDL_Rect thinkingRect = { kResponseArea.x + kAreaPadding, kResponseArea.y + kAreaPadding, 0, 0 };
        SDL_QueryTexture(thinking_texture, nullptr, nullptr, &thinkingRect.w, &thinkingRect.h);
        lineTextures.push_back(thinking_texture);
        lineRects.push_back(thinkingRect);
    }

    IsThinking = true;
    AdaGestures(kThinkingGesture);

    // The previous request has already finished (IsThinking was false), so this returns immediately.
    if (aiThread.joinable()) {
        aiThread.join();
    }

    const bool useOffline = LocalMode;
    aiThread = std::thread([this, &AI, prompt = std::move(prompt), useOffline]() {
        RunAIRequest(AI, prompt, useOffline);
    });
}

void GUI::RunAIRequest(AI_ENGINE& AI, const std::string& prompt, bool useOffline) {
#if defined(_WIN32) || defined(_WIN64)
    ComScope com; // Needed by the voice engine on this thread
#endif

    std::string response;
    try {
        response = AI.SendPrompt(prompt, useOffline);
        if (!response.empty()) {
            ProcessAICommands(response);
        }
    } catch (const std::exception& e) {
        response = std::string("Error inesperado: ") + e.what();
        AdaGestures(kErrorGesture);
    } catch (...) {
        // An exception must never escape a thread: it would terminate the program.
        response = "Error inesperado";
        AdaGestures(kErrorGesture);
    }

    if (!response.empty()) {
        {
            std::lock_guard<std::mutex> lock(responseMutex);
            ResponseText = response;
        }

        if (VoiceIsActive) {
            try {
                voice.TalkAda(voice.CleanTextForTalk(response));
            } catch (const std::exception& e) {
                std::cerr << "Voice error: " << e.what() << std::endl;
            }
        }
    }

    IsThinking = false;
}

void GUI::CreateReminderFromTag(const std::string& value) {
    // Expected format: NAME, WHEN=DAY/HH:MM AM|PM
    const std::size_t whenMarker = value.find(", WHEN=");
    if (whenMarker == std::string::npos) {
        return;
    }

    const std::string name = value.substr(0, whenMarker);
    const std::string when = value.substr(whenMarker + 7);

    const std::size_t slash = when.find('/');
    if (slash == std::string::npos) {
        return;
    }

    const std::string day = when.substr(0, slash);
    const std::string timePart = when.substr(slash + 1);

    const std::size_t colon = timePart.find(':');
    if (colon == std::string::npos) {
        return;
    }

    try {
        const std::string afterColon = timePart.substr(colon + 1);
        const int hour = std::stoi(timePart.substr(0, colon));
        const int minute = std::stoi(afterColon.substr(0, 2));
        if (hour < 0 || hour > 23 || minute < 0 || minute > 59) {
            return;
        }

        std::string upper = afterColon;
        std::transform(upper.begin(), upper.end(), upper.begin(),
                       [](unsigned char c) { return static_cast<char>(std::toupper(c)); });

        std::string period;
        if (upper.find("PM") != std::string::npos) {
            period = "PM";
        } else if (upper.find("AM") != std::string::npos) {
            period = "AM";
        }

        std::lock_guard<std::mutex> lock(remindersMutex);
        reminders.CreateReminder(name, day, hour, minute, period);
    } catch (const std::exception&) {
        // Malformed reminder from the model: ignore it.
    }
}

// Runs on the AI thread. The order matters: [CMD_EXECUTE] cuts the response
// at its tag, so every tag after it is no longer seen.
void GUI::ProcessAICommands(std::string& response) {
    if (auto tag = FindTag(response, kTagShutdown)) {
        if (const auto seconds = ParseSeconds(tag->value)) {
            ScheduleSystemPower(false, *seconds);
        }
    }

    if (auto tag = FindTag(response, kTagRestart)) {
        if (const auto seconds = ParseSeconds(tag->value)) {
            ScheduleSystemPower(true, *seconds);
        }
    }

    if (auto tag = FindTag(response, kTagExecute)) {
        response.erase(tag->tagPos);
        LaunchApplication(tag->value);
    }

    if (auto tag = FindTag(response, kTagSysCmd)) {
        RunInTerminal(tag->value);
    }

    if (auto tag = FindTag(response, kTagOpenFolder)) {
        OpenFolder(tag->value);
    }

    if (auto tag = FindTag(response, kTagWebsite)) {
        OpenWebsite(tag->value);
    }

    if (auto tag = FindTag(response, kTagReminder)) {
        CreateReminderFromTag(tag->value);
    }

    const int gesture = ExtractGesture(response);
    if (gesture >= 0) {
        AdaGestures(gesture);
    }
}

void GUI::UpdateResponseView() {
    if (IsThinking) {
        return;
    }

    std::string text;
    {
        std::lock_guard<std::mutex> lock(responseMutex);
        text = ResponseText;
    }

    if (text.empty()) {
        // The request ended without a reply: drop the "thinking" placeholder.
        if (!lineTextures.empty() && lineTextures.front() == thinking_texture) {
            DestroyLineTextures();
        }
        return;
    }

    if (text != LastResponse) {
        LastResponse = text;
        scrollY = 0;
        ProcessAIResponse(text);
        MakeResponseTexture(kResponseArea);
    }
}

/* ------------------------------------------------------------------ */
/* Drawing                                                             */
/* ------------------------------------------------------------------ */

void GUI::DrawButtons(const UiContext& ui) {
    DrawButton(renderer, kModeButton, LocalMode ? kButtonIdle : kButtonActive, Mode_tex);

    const bool copyHovered = Contains(kCopyButton, ui.mouseX, ui.mouseY);
    DrawButton(renderer, kCopyButton, copyHovered ? kButtonHover : kButtonIdle, CopyTexture);

    DrawButton(renderer, kVoiceButton, VoiceIsActive ? kButtonActive : kButtonIdle, TextVoiceButton_tex);
}

void GUI::DrawResponseText() {
    if (!lineTextures.empty()) {
        // The clip rect hides anything that overflows the text box.
        const SDL_Rect clip = { kResponseArea.x + 10, kResponseArea.y + 10, kResponseArea.w - 20, kResponseArea.h - 20 };
        SDL_RenderSetClipRect(renderer, &clip);

        for (std::size_t i = 0; i < lineTextures.size(); ++i) {
            if (lineTextures[i] != nullptr) {
                SDL_Rect destination = lineRects[i];
                destination.y -= scrollY;
                SDL_RenderCopy(renderer, lineTextures[i], nullptr, &destination);
            }
        }
        SDL_RenderSetClipRect(renderer, nullptr);
    }

    if (switchingToOffline && switchingModeTexture != nullptr) {
        SDL_RenderCopy(renderer, switchingModeTexture, nullptr, &switchingModeRect);
    }
}

void GUI::DrawUserInput() {
    const SDL_Rect clip = { kUserArea.x + 10, kUserArea.y + 10, kUserArea.w - 20, kUserArea.h - 20 };
    SDL_RenderSetClipRect(renderer, &clip);

    if (!UserText.empty() && UserTextTexture != nullptr) {
        SDL_Rect destination = UserTextRect;
        destination.y -= userScrollY;
        SDL_RenderCopy(renderer, UserTextTexture, nullptr, &destination);
    }

    NormalizeCursor();
    const CursorMetrics cursor = MeasureCursor(UserTextFont, UserText, cursorIndex, kUserInputWidth);

    if (BlinkOn()) {
        const SDL_Rect caret = {
            UserTextRect.x + cursor.lineWidth,
            kUserArea.y + kAreaPadding + cursor.row * TTF_FontLineSkip(UserTextFont) - userScrollY,
            2,
            TTF_FontHeight(UserTextFont)
        };
        SetDrawColor(renderer, kTextColor);
        SDL_RenderFillRect(renderer, &caret);
    }

    SDL_RenderSetClipRect(renderer, nullptr);
}

void GUI::DrawFrame(const UiContext& ui) {
    SetDrawColor(renderer, kBackgroundColor);
    SDL_RenderClear(renderer);

    SDL_RenderCopy(renderer, Ada_SpriteSheet_texture, &Ada_src_rect, &Ada_dest_rect);

    SetDrawColor(renderer, kBorderColor);
    SDL_RenderDrawRect(renderer, &kResponseArea);
    SDL_RenderDrawRect(renderer, &kUserArea);

    DrawButtons(ui);
    DrawResponseText();
    DrawUserInput();

    if (isListening && BlinkOn()) {
        SetDrawColor(renderer, kListeningDotColor);
        DrawFilledCircle(renderer, 1200, 300, 10);
    }
}

/* ------------------------------------------------------------------ */
/* Main loop                                                           */
/* ------------------------------------------------------------------ */

void GUI::RenderGui(AI_ENGINE &AI) {
    UiContext ui;

    SDL_RenderSetLogicalSize(renderer, kLogicalWidth, kLogicalHeight);
    SDL_StartTextInput();
    SDL_ShowWindow(window);

    while (ui.running) {
        const auto frameStart = std::chrono::steady_clock::now();

        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            HandleEvent(e, ui, AI);
        }

        UpdateModeSwitchResult();
        UpdateResponseView();
        UpdateMicInput();
        ApplyPendingGesture();

        {
            std::lock_guard<std::mutex> lock(remindersMutex);
            reminders.CheckReminders();
        }

        if (ui.minimized) {
            SDL_Delay(100);
            continue;
        }

        DrawFrame(ui);
        SDL_RenderPresent(renderer);

        const auto frameTime = std::chrono::steady_clock::now() - frameStart;
        if (frameTime < kFrameDuration) {
            std::this_thread::sleep_for(kFrameDuration - frameTime);
        }
    }

    SDL_StopTextInput();

    // Hide the window first: a request still in flight can take a while to finish.
    SDL_HideWindow(window);
    JoinWorkers();
}
