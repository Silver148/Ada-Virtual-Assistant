#ifndef GUI_HPP
#define GUI_HPP
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <md4c.h>
#include <string>
#include <iostream>
#include <thread>
#include <atomic>
#include <mutex>
#include <vector>
#include "Reminders.hpp"
#include "Talk.hpp"
#include "AI_Engine.hpp"
#include "STT.hpp"
#if defined(_WIN32) || defined(_WIN64)
    #include "SearchEXE.hpp"
    #include <windows.h>
    #include <shlobj.h>
#endif
#if defined(__linux__) || defined(__unix__)
#include "Audio_Engine.hpp"
#include "main.hpp"
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#endif

enum LineType {
    LINE_NORMAL,
    LINE_TITLE,
    LINE_SUBTITLE,
    LINE_BULLET
};

struct TextStyleToken {
    std::string text;
    bool isBold = false;
};

struct RenderLine {
    LineType type = LINE_NORMAL;
    std::vector<TextStyleToken> tokens;
};

struct UiContext; // Per-frame UI state, defined in GUI.cpp

class GUI{

private:
    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;

    SDL_Texture* Ada_SpriteSheet_texture = nullptr;
    SDL_Rect Ada_dest_rect = { 0, 0, 0, 0 };
    SDL_Rect Ada_src_rect = { 0, 0, 0, 0 };

    std::string UserText;
    std::string ResponseText;   // Written by the AI thread, always guarded by responseMutex
    std::string LastResponse;

    TTF_Font* UserTextFont = nullptr;
    SDL_Texture* UserTextTexture = nullptr;
    SDL_Rect UserTextRect = { 0, 0, 0, 0 };

    TTF_Font* AdaTextFont = nullptr;
    std::vector<RenderLine> parsedLines;
    std::vector<SDL_Texture*> lineTextures;
    std::vector<SDL_Rect> lineRects;

    TTF_Font* CopyFont = nullptr;
    SDL_Texture* CopyTexture = nullptr;

    TTF_Font* TextVoiceButton_font = nullptr;
    SDL_Texture* TextVoiceButton_tex = nullptr;

    TTF_Font* Mode_font = nullptr;
    SDL_Texture* Mode_tex = nullptr;

    SDL_Texture* thinking_texture = nullptr;

    int scrollY = 0;
    int maxScrollY = 0;

    int userScrollY = 0;
    int maxUserScrollY = 0;

    Reminders reminders;
    std::mutex remindersMutex;  // CreateReminder (AI thread) vs CheckReminders (main thread)

#if defined(__linux__ ) || defined(__unix__)
    AudioEngine audioEngine;
#endif

    AdaVoice voice;

    std::atomic<bool> IsThinking{false};
    std::atomic<bool> VoiceIsActive{false};
    std::atomic<bool> LocalMode{false};

    std::atomic<bool> switchingToOffline{false};
    std::atomic<int> offlineSwitchResult{0}; // 0 = none, 1 = success, 2 = failure
    std::atomic<int> pendingGesture{-1};     // -1 = none, applied by the main thread
    std::thread offlineSwitchThread;
    std::thread aiThread;
    std::mutex responseMutex;

    SDL_Texture* switchingModeTexture = nullptr;
    SDL_Rect switchingModeRect = { 0, 0, 0, 0 };

    size_t cursorIndex = 0;

    STT stt;
    bool isListening = false;

    /* Setup / teardown */
    void Setup();
    void ReleaseResources();
    void JoinWorkers();

    /* Helpers shared by the event handlers */
    void SetModeLabel(const char* text);
    void DestroyLineTextures();
    void ResetResponse();
    void ClearUserTexture();
    void RebuildUserText();
    void InsertUserText(const std::string& text);
    void PasteFromClipboard();
    void NormalizeCursor();
    void ApplyPendingGesture();

    /* Input */
    void HandleEvent(const SDL_Event& e, UiContext& ui, AI_ENGINE& AI);
    void HandleMouseWheel(const SDL_MouseWheelEvent& wheel, const UiContext& ui);
    void HandleLeftClick(int x, int y, AI_ENGINE& AI);
    void HandleKeyDown(const SDL_KeyboardEvent& key, UiContext& ui, AI_ENGINE& AI);
    void HandleBackspace(UiContext& ui);
    void HandleModeButton(AI_ENGINE& AI);
    void HandleVoiceButton();
    void ToggleListening();
    void StartOfflineSwitch(AI_ENGINE& AI);

    /* AI request */
    void SubmitPrompt(AI_ENGINE& AI);
    void RunAIRequest(AI_ENGINE& AI, const std::string& prompt, bool useOffline);
    void ProcessAICommands(std::string& response);
    void CreateReminderFromTag(const std::string& value);

    /* Per-frame updates and drawing */
    void UpdateModeSwitchResult();
    void UpdateResponseView();
    void UpdateMicInput();
    void DrawFrame(const UiContext& ui);
    void DrawButtons(const UiContext& ui);
    void DrawResponseText();
    void DrawUserInput();

    void ProcessAIResponse(const std::string& rawText);
    void MakeResponseTexture(SDL_Rect ResponseArea);

public:

    GUI();
    ~GUI();

    GUI(const GUI&) = delete;
    GUI& operator=(const GUI&) = delete;

    void RenderGui(AI_ENGINE &AI);
    void AdaGestures(int id); // Thread-safe: the frame is applied by the render loop

};

#endif
