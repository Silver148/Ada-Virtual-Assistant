#ifndef GUI_HPP
#define GUI_HPP
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <md4c.h>
#include <string>
#include <iostream>
#include <thread>
#include <md4c.h>
#include "Reminders.hpp"
#include "Talk.hpp"
#include "AI_Engine.hpp"
#if defined(__linux__) || defined(__unix__)
#include "Audio_Engine.hpp"
#include "main.hpp"
#endif

enum LineType {
    LINE_NORMAL,
    LINE_TITLE,
    LINE_SUBTITLE,
    LINE_BULLET
};

struct TextStyleToken {
    std::string text;
    bool isBold;
};

struct RenderLine {
    LineType type;
    std::vector<TextStyleToken> tokens;
};

class GUI{

private:
    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;

    SDL_Surface* Ada_SpriteSheet_surface = nullptr;
    SDL_Texture* Ada_SpriteSheet_texture = nullptr;

    SDL_Rect Ada_dest_rect;
    SDL_Rect Ada_src_rect;

    std::string UserText;
    std::string ResponseText;
    std::string LastResponse;

    TTF_Font* UserTextFont = nullptr;
    SDL_Surface* UserTextSurface = nullptr;
    SDL_Texture* UserTextTexture = nullptr;
    SDL_Rect UserTextRect;

    TTF_Font* AdaTextFont = nullptr;
    SDL_Surface* AdaTextSurface = nullptr;
    std::vector<RenderLine> parsedLines;
    std::vector<SDL_Texture*> lineTextures;
    std::vector<SDL_Rect> lineRects;
    SDL_Rect AdaTextRect;

    TTF_Font* CopyFont = nullptr;
    SDL_Surface* CopySurface = nullptr;
    SDL_Texture* CopyTexture = nullptr;
    SDL_Rect CopyRect;
    SDL_Rect CopyButton;

    TTF_Font* TextVoiceButton_font = nullptr;
    SDL_Surface* TextVoiceButton_surf = nullptr;
    SDL_Texture* TextVoiceButton_tex = nullptr;
    SDL_Rect TextVoiceButton;
    SDL_Rect VoiceButton;

    SDL_Texture* thinking_texture = nullptr;

    int scrollY = 0;
    int maxScrollY = 0;

    int userScrollY = 0;
    int maxUserScrollY = 0;

    Reminders r;
#if defined(__linux__ ) || defined(__unix___)
    AudioEngine audioEngine;
#endif
    AdaVoice voice;

    bool IsThinking = false;
    bool VoiceIsActive = false;

    size_t cursorIndex = 0;
    
public:

    GUI();
    ~GUI();

    void RenderGui(AI_ENGINE &AI);
    void AdaGestures(int id);

    void ProcessAIResponse(const std::string& rawText);
    void MakeResponseTexture(SDL_Rect ResponseArea);

};

#endif