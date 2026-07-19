/*
GUI ;)

GUI.cpp
*/

#include "GUI.hpp"

#define FRAME_WIDTH 205
#define FRAME_HEIGHT 200

#if defined(_WIN32) || defined(_WIN64)
GUI::GUI() : voice(), stt() {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_EVENTS) < 0 || IMG_Init(IMG_INIT_PNG) < 0 || TTF_Init() < 0) {
        throw std::runtime_error("Error to init SDL, IMG and TTF");
    }

    window = SDL_CreateWindow("Ada", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 1280, 720, SDL_WINDOW_HIDDEN | SDL_WINDOW_RESIZABLE);

    if (!window) {
        throw std::runtime_error("Error to create window");
    }
    
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    if (!renderer) {
        throw std::runtime_error("Error to create renderer");
    }

    Ada_SpriteSheet_surface = IMG_Load("Ada_SpriteSheet.png");

    if (Ada_SpriteSheet_surface) {
        Ada_SpriteSheet_texture = SDL_CreateTextureFromSurface(renderer, Ada_SpriteSheet_surface);
        SDL_FreeSurface(Ada_SpriteSheet_surface);
        Ada_SpriteSheet_surface = nullptr;
    } else {
        fprintf(stderr, "Error to render Ada texture");
    }
        
    Ada_src_rect.x = 0;
    Ada_src_rect.y = 0;
    Ada_src_rect.w = FRAME_WIDTH;
    Ada_src_rect.h = FRAME_HEIGHT;

    Ada_dest_rect.x = 1280 / 2 - FRAME_WIDTH / 2;
    Ada_dest_rect.y = 0;
    Ada_dest_rect.w = FRAME_WIDTH;
    Ada_dest_rect.h = FRAME_HEIGHT;

    std::string TextFontPath = "fonts/Segoe-UI-EMOJI.ttf";

    UserTextFont = TTF_OpenFont(TextFontPath.c_str(), 20);
    AdaTextFont = TTF_OpenFont(TextFontPath.c_str(), 20);
    CopyFont = TTF_OpenFont(TextFontPath.c_str(), 16);
    TextVoiceButton_font = TTF_OpenFont(TextFontPath.c_str(), 16); 
    Mode_font = TTF_OpenFont(TextFontPath.c_str(), 16);

    if (CopyFont) {
        CopySurface = TTF_RenderText_Solid(CopyFont, "Copy", {0, 0, 0, 255});
        if (CopySurface) {
            CopyTexture = SDL_CreateTextureFromSurface(renderer, CopySurface);
            SDL_FreeSurface(CopySurface);
            CopySurface = nullptr;
        } else {
            fprintf(stderr, "Error to render 'copy' text\n");
        }
    }

    if (TextVoiceButton_font) {
        TextVoiceButton_surf = TTF_RenderText_Solid(TextVoiceButton_font, "Voice", {0, 0, 0, 255});
        
        if (TextVoiceButton_surf) {
            TextVoiceButton_tex = SDL_CreateTextureFromSurface(renderer, TextVoiceButton_surf);
            SDL_FreeSurface(TextVoiceButton_surf);
            TextVoiceButton_surf = nullptr;
        } else {
            fprintf(stderr, "Error to render text 'voice' button\n");
        }    
    }

    if(Mode_font){
        Mode_surf = TTF_RenderText_Solid(Mode_font, "Online", {0, 0, 0, 255});

        if(Mode_surf){
            Mode_tex = SDL_CreateTextureFromSurface(renderer, Mode_surf);
            SDL_FreeSurface(Mode_surf);
            Mode_surf = nullptr;
        }else{
            fprintf(stderr, "Error to render text 'Mode:' button\n");
        }
    }

    reminders = Reminders();
    reminders.LoadReminders();

    stt.Init();
}
#else
GUI::GUI() : audioEngine(), voice(audioEngine.dev), stt() {

    if (IMG_Init(IMG_INIT_PNG) < 0 || TTF_Init() < 0) {
        throw std::runtime_error("Error to init IMG and TTF!");
    }

    std::string base = get_base_dir();

    window = SDL_CreateWindow("Ada", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 1280, 720, SDL_WINDOW_HIDDEN | SDL_WINDOW_RESIZABLE);

    if (!window) {
        throw std::runtime_error("Error to create window");
    }
    
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    if (!renderer) {
        throw std::runtime_error("Error to create renderer");
    }

    std::string SpriteSheetPath = base + "/Ada_SpriteSheet.png";
    Ada_SpriteSheet_surface = IMG_Load(SpriteSheetPath.c_str());

    if (Ada_SpriteSheet_surface) {
        Ada_SpriteSheet_texture = SDL_CreateTextureFromSurface(renderer, Ada_SpriteSheet_surface);
        SDL_FreeSurface(Ada_SpriteSheet_surface);
        Ada_SpriteSheet_surface = nullptr;
    } else {
        fprintf(stderr, "Error to render Ada texture");
    }
        
    Ada_src_rect.x = 0;
    Ada_src_rect.y = 0;
    Ada_src_rect.w = FRAME_WIDTH;
    Ada_src_rect.h = FRAME_HEIGHT;

    Ada_dest_rect.x = 1280 / 2 - FRAME_WIDTH / 2;
    Ada_dest_rect.y = 0;
    Ada_dest_rect.w = FRAME_WIDTH;
    Ada_dest_rect.h = FRAME_HEIGHT;


    std::string TextFontPath = base + "/fonts/Segoe-UI-EMOJI.ttf";

    UserTextFont = TTF_OpenFont(TextFontPath.c_str(), 20);
    AdaTextFont = TTF_OpenFont(TextFontPath.c_str(), 20);
    CopyFont = TTF_OpenFont(TextFontPath.c_str(), 16);
    TextVoiceButton_font = TTF_OpenFont(TextFontPath.c_str(), 16); 
    Mode_font = TTF_OpenFont(TextFontPath.c_str(), 16);

    if (CopyFont) {
        CopySurface = TTF_RenderText_Solid(CopyFont, "Copy", {0, 0, 0});
        if (CopySurface) {
            CopyTexture = SDL_CreateTextureFromSurface(renderer, CopySurface);
            SDL_FreeSurface(CopySurface);
            CopySurface = nullptr;
        } else {
            fprintf(stderr, "Error to render 'copy' text\n");
        }
    }

    if (TextVoiceButton_font) {
        TextVoiceButton_surf = TTF_RenderText_Solid(TextVoiceButton_font, "Voice", {0, 0, 0});
        
        if (TextVoiceButton_surf) {
            TextVoiceButton_tex = SDL_CreateTextureFromSurface(renderer, TextVoiceButton_surf);
            SDL_FreeSurface(TextVoiceButton_surf);
            TextVoiceButton_surf = nullptr;
        } else {
            fprintf(stderr, "Error to render text 'voice' button\n");
        }    
    }

    if(Mode_font){
        Mode_surf = TTF_RenderText_Solid(Mode_font, "Online", {0, 0, 0, 255});

        if(Mode_surf){
            Mode_tex = SDL_CreateTextureFromSurface(renderer, Mode_surf);
            SDL_FreeSurface(Mode_surf);
            Mode_surf = nullptr;
        }else{
            fprintf(stderr, "Error to render text 'Mode:' button\n");
        }
    }

    reminders = Reminders();
    reminders.LoadReminders();

    stt.Init();
}
#endif

GUI::~GUI() {
    if (Ada_SpriteSheet_texture) {
        SDL_DestroyTexture(Ada_SpriteSheet_texture);
        Ada_SpriteSheet_texture = nullptr;
    }

    if (thinking_texture) {
        SDL_DestroyTexture(thinking_texture);
        thinking_texture = nullptr;
    }

    if (switchingModeTexture) {
        SDL_DestroyTexture(switchingModeTexture);
        switchingModeTexture = nullptr;
    }

    if (TextVoiceButton_tex) {
        SDL_DestroyTexture(TextVoiceButton_tex);
        TextVoiceButton_tex = nullptr;
    }

    if (CopyTexture) {
        SDL_DestroyTexture(CopyTexture);
        CopyTexture = nullptr;
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);

    IMG_Quit();

    TTF_CloseFont(UserTextFont);
    TTF_CloseFont(AdaTextFont);
    TTF_CloseFont(TextVoiceButton_font);
    TTF_CloseFont(Mode_font);

    TTF_Quit();
    SDL_Quit();
}

/* PARSER MD4C */

static std::vector<RenderLine>* g_lines = nullptr;
static RenderLine currentLine;
static bool nextTextIsBold = false;

int cb_enter_block(MD_BLOCKTYPE type, void* detail, void* userdata) {
    (void)userdata;

    currentLine.tokens.clear();
    
    if (type == MD_BLOCK_H) {
        currentLine.type = LINE_TITLE;
        if (detail != nullptr) {
            MD_BLOCK_H_DETAIL* h_detail = static_cast<MD_BLOCK_H_DETAIL*>(detail);
            if (h_detail->level > 1) currentLine.type = LINE_SUBTITLE;
        }
    } else if (type == MD_BLOCK_LI) {
        currentLine.type = LINE_BULLET;
    } else {
        currentLine.type = LINE_NORMAL;
    }
    return 0;
}

int cb_leave_block(MD_BLOCKTYPE type, void* detail, void* userdata) {
    (void)type;
    (void)detail;
    (void)userdata;

    if (!currentLine.tokens.empty()) {
        g_lines->push_back(currentLine);
        currentLine.tokens.clear();
    }
    return 0;
}

int cb_enter_span(MD_SPANTYPE type, void* detail, void* userdata) {

    (void)detail;
    (void)userdata;

    if (type == MD_SPAN_STRONG) {
        nextTextIsBold = true;
    }
    return 0;
}

int cb_leave_span(MD_SPANTYPE type, void* detail, void* userdata) {

    (void)detail;
    (void)userdata;

    if (type == MD_SPAN_STRONG) {
        nextTextIsBold = false;
    }
    return 0;
}

int cb_text(MD_TEXTTYPE type, const MD_CHAR* text, MD_SIZE size, void* userdata) {

    (void)type;
    (void)userdata;

    std::string str(text, size);
    
    if (!str.empty()) {
        if (str == "\n") {
            if (!currentLine.tokens.empty()) {
                g_lines->push_back(currentLine);
                LineType prevType = currentLine.type;
                currentLine.tokens.clear();
                currentLine.type = prevType;
            }
        } else {
            TextStyleToken token = { str, nextTextIsBold };
            currentLine.tokens.push_back(token);
        }
    }
    return 0;
}

//FUNCS TO PROCESS AI RESPONSE
void GUI::ProcessAIResponse(const std::string& rawText) {
    parsedLines.clear();
    g_lines = &parsedLines;
    nextTextIsBold = false;

    MD_PARSER parser;

    parser.flags = MD_FLAG_PERMISSIVEAUTOLINKS | MD_FLAG_TABLES | MD_FLAG_STRIKETHROUGH;
    parser.abi_version = 0;
    parser.enter_block = cb_enter_block;
    parser.leave_block = cb_leave_block;
    parser.enter_span  = cb_enter_span;
    parser.leave_span  = cb_leave_span;
    parser.text        = cb_text;

    md_parse(rawText.c_str(), (MD_SIZE)rawText.size(), &parser, nullptr);
}

void GUI::MakeResponseTexture(SDL_Rect ResponseArea) {
    for (auto* tex : lineTextures) {
        if (tex && tex != thinking_texture) SDL_DestroyTexture(tex); //Only destroy if tex isn't "thinking_texture" :)
    }

    lineTextures.clear();
    lineRects.clear();

    int currentY = ResponseArea.y + 20;
    int maxW = ResponseArea.w - 40;

    SDL_Color textColor = {0, 0, 0, 255};

    for (const auto& line : parsedLines) {
        //Configure the size font depending on the line
        if (line.type == LINE_TITLE) TTF_SetFontStyle(AdaTextFont, TTF_STYLE_BOLD);
        else TTF_SetFontStyle(AdaTextFont, TTF_STYLE_NORMAL);
        
        std::string lineString = "";
        if (line.type == LINE_BULLET) {
            lineString += "• ";
        }

        for (const auto& token : line.tokens) {
            lineString += token.text;
        }

        if (lineString.empty() || lineString == " ") continue;

        //if it's a title, visually increase the size font
        if (line.type == LINE_TITLE) {
            TTF_SetFontStyle(AdaTextFont, TTF_STYLE_BOLD);
        }

        //Render the line with wrapped
        SDL_Surface* surf = TTF_RenderUTF8_Blended_Wrapped(AdaTextFont, lineString.c_str(), textColor, maxW);
        
        //Reset style by default
        TTF_SetFontStyle(AdaTextFont, TTF_STYLE_NORMAL);

        if (surf != nullptr) {
            SDL_Texture* tex = SDL_CreateTextureFromSurface(renderer, surf);
    
            SDL_Rect r;
            r.x = ResponseArea.x + (line.type == LINE_BULLET ? 40 : 20);
            r.y = currentY;
            r.w = surf->w;
            r.h = surf->h;

            lineTextures.push_back(tex);
            lineRects.push_back(r);

            if (line.type == LINE_TITLE || line.type == LINE_SUBTITLE) {
                currentY += surf->h + 16;
            } else if (line.type == LINE_BULLET) {
                currentY += surf->h + 6; 
            } else {
                currentY += surf->h + 12;
            }

            SDL_FreeSurface(surf);
        }
    }

    //Recalculate the scroll limit based on the height of the last line
    if (!lineRects.empty()) {
        int totalHeight = (lineRects.back().y + lineRects.back().h) - (ResponseArea.y + 20);
        if (totalHeight > (ResponseArea.h - 40)) {
            maxScrollY = totalHeight - (ResponseArea.h - 40);
        } else {
            maxScrollY = 0;
        }
    }
}

/* FUNCS GUI */
void GUI::AdaGestures(int id) {
    int column = id % 5;
    int row = id / 5;

    Ada_src_rect.x = column * FRAME_WIDTH;
    Ada_src_rect.y = row * FRAME_HEIGHT;
}

void DrawFilledCircle(SDL_Renderer* renderer, int x, int y, int radius) {
    for (int w = 0; w < radius * 2; w++) {
        for (int h = 0; h < radius * 2; h++) {
            int dx = radius - w;
            int dy = radius - h;
            if ((dx*dx + dy*dy) <= (radius * radius)) {
                SDL_RenderDrawPoint(renderer, x + dx, y + dy);
            }
        }
    }
}

static Uint32 lastBackspaceTime = 0;
const Uint32 BACKSPACE_DELAY = 50;

const int TARGET_FPS = 60;
const std::chrono::milliseconds FRAME_DURATION(1000 / TARGET_FPS);

void GUI::RenderGui(AI_ENGINE &AI) {
    bool running = true;
    bool fullscreen = false;
    bool isMinimized = false;

    SDL_Rect ResponseArea = { 140, 250, 1000, 350 };
    SDL_Rect UserArea = { 140, 630, 1000, 80 };

    CopyButton = { 
        ResponseArea.x + ResponseArea.w - 100,
        ResponseArea.y + ResponseArea.h - 45,
        80,
        30
    };

    VoiceButton = { 1150, 650, 100, 40 };
    ModeButton = {1150, 580, 100, 40};

    int maxInputWidth = UserArea.w - 40;

    SDL_RenderSetLogicalSize(renderer, 1280, 720);

    SDL_StartTextInput();

    SDL_Surface* thinking = TTF_RenderText_Solid(AdaTextFont, "Pensando...", {0, 0, 0, 255}); //Text for thinking state
    thinking_texture = SDL_CreateTextureFromSurface(renderer, thinking);

    SDL_FreeSurface(thinking);

    int mx = 0, my = 0;

    SDL_ShowWindow(window);

    while (running) {
        auto frameStart = std::chrono::high_resolution_clock::now();

        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                running = false;
            } else if (e.type == SDL_WINDOWEVENT) {
                switch (e.window.event) {
                    case SDL_WINDOWEVENT_MINIMIZED:
                        isMinimized = true;
                        break;
                    case SDL_WINDOWEVENT_RESTORED:
                        isMinimized = false;
                        break;
                }
            } else if (e.type == SDL_MOUSEMOTION) {
                mx = e.motion.x;
                my = e.motion.y;
            } else if (e.type == SDL_MOUSEWHEEL) { //Scroll
                if (mx >= UserArea.x && mx <= (UserArea.x + UserArea.w) &&
                    my >= UserArea.y && my <= (UserArea.y + UserArea.h)) {
                    if (e.wheel.y > 0) { // Up
                        userScrollY -= 20;
                        if (userScrollY < 0) userScrollY = 0;
                    } else if (e.wheel.y < 0) { // Down
                        userScrollY += 20;
                        if (userScrollY > maxUserScrollY) userScrollY = maxUserScrollY;
                    }
                } else {
                    if (e.wheel.y > 0) { // Up
                        scrollY -= 24;
                        if (scrollY < 0) scrollY = 0;
                    } else if (e.wheel.y < 0) { // Down
                        scrollY += 24;
                        if (scrollY > maxScrollY) scrollY = maxScrollY;
                    }
                }
            } else if (e.type == SDL_MOUSEBUTTONDOWN) {
                if (e.button.button == SDL_BUTTON_LEFT) { // Clic izquierdo
                    int mouseX = e.button.x;
                    int mouseY = e.button.y;

                    if (mouseX >= CopyButton.x && mouseX <= (CopyButton.x + CopyButton.w) &&
                        mouseY >= CopyButton.y && mouseY <= (CopyButton.y + CopyButton.h)) {
                        if (!this->ResponseText.empty()) {
                            SDL_SetClipboardText(this->ResponseText.c_str());
                        }
                    }

                    if(mouseX >= ModeButton.x && mouseX <= (ModeButton.x + ModeButton.w) &&
                        mouseY >= ModeButton.y && mouseY <= (ModeButton.y + ModeButton.h)) {

                        if(!LocalMode && AI.IsOfflineModelDownloaded("AdaOffline.Q4_K_M.gguf")){
                            if (!switchingToOffline) {
                                switchingToOffline = true;
                                offlineSwitchResult = 0;

                                if (switchingModeTexture) {
                                    SDL_DestroyTexture(switchingModeTexture);
                                    switchingModeTexture = nullptr;
                                }

                                /*DESTROY IF LINE TEXTURE HAS TEXT*/
                                for (SDL_Texture* tex : lineTextures) {
                                    if (tex && tex != thinking_texture) {
                                        SDL_DestroyTexture(tex);
                                    }
                                }

                                lineTextures.clear();
                                lineRects.clear();
                                parsedLines.clear();

                                this->ResponseText = "";
                                this->LastResponse = "";

                                if (UserTextTexture != nullptr) {
                                    SDL_DestroyTexture(UserTextTexture);
                                    UserTextTexture = nullptr;
                                }
                                if (UserTextSurface != nullptr) {
                                    SDL_FreeSurface(UserTextSurface);
                                    UserTextSurface = nullptr;
                                }
                                userScrollY = 0;
                                maxUserScrollY = 0;
                                scrollY = 0;

                                SDL_Surface* switchingSurface = TTF_RenderText_Solid(AdaTextFont, "Switching to offline mode", {0, 0, 0, 255});
                                if (switchingSurface) {
                                    switchingModeTexture = SDL_CreateTextureFromSurface(renderer, switchingSurface);
                                    switchingModeRect.w = switchingSurface->w;
                                    switchingModeRect.h = switchingSurface->h;
                                    switchingModeRect.x = 1280 / 2 - switchingModeRect.w / 2;
                                    switchingModeRect.y = 720 / 2 - switchingModeRect.h / 2;
                                    SDL_FreeSurface(switchingSurface);
                                }

                                GUI* self = this;
                                offlineSwitchThread = std::thread([&AI, self]() {
                                    bool success = AI.InitOfflineMode();
                                    if (success) {
                                        self->LocalMode = true;
                                        self->offlineSwitchResult = 1;
                                    } else {
                                        self->offlineSwitchResult = 2;
                                    }
                                    self->switchingToOffline = false;
                                });
                                offlineSwitchThread.detach();
                            }
                        } else {
                            if (LocalMode) {
                                LocalMode = false;
                                AI.DeinitOfflineMode();
                            }
                            if(Mode_tex) SDL_DestroyTexture(Mode_tex);
                            Mode_surf = TTF_RenderText_Solid(Mode_font, "Online", {0, 0, 0, 255});
                            if(Mode_surf){
                                Mode_tex = SDL_CreateTextureFromSurface(renderer, Mode_surf);
                                SDL_FreeSurface(Mode_surf);
                                Mode_surf = nullptr;
                            }else{
                                fprintf(stderr, "Error to render text 'Online' button\n");
                            }
                        }
                    }

                    if (mouseX >= VoiceButton.x && mouseX <= (VoiceButton.x + VoiceButton.w) &&
                        mouseY >= VoiceButton.y && mouseY <= (VoiceButton.y + VoiceButton.h)) {
                        if (!VoiceIsActive) {
                            #if defined(__linux__) || defined(__unix__)
                                voice.ShutUpAda(); //In Linux this pause and resume the audio engine
                            #endif
                            VoiceIsActive = true;
                        } else {
                            VoiceIsActive = false;
                            voice.ShutUpAda();
                        }
                    }
                }
            } else if (e.type == SDL_TEXTINPUT) {
                if (static_cast<unsigned char>(e.text.text[0]) >= 32) {
                    UserText.insert(cursorIndex, e.text.text);
                    cursorIndex += strlen(e.text.text);
                
                    if (UserTextSurface != nullptr) SDL_FreeSurface(UserTextSurface);
                    if (UserTextTexture != nullptr) SDL_DestroyTexture(UserTextTexture);

                    UserTextSurface = TTF_RenderUTF8_Blended_Wrapped(UserTextFont, UserText.c_str(), {0, 0, 0, 255}, maxInputWidth);
                    UserTextTexture = SDL_CreateTextureFromSurface(renderer, UserTextSurface);

                    UserTextRect.x = UserArea.x + 20;
                    UserTextRect.y = UserArea.y + 20;
                    UserTextRect.w = UserTextSurface->w;
                    UserTextRect.h = UserTextSurface->h;

                    if (UserTextRect.h > (UserArea.h - 40)) {
                        maxUserScrollY = UserTextRect.h - (UserArea.h - 40);
                        userScrollY = maxUserScrollY; // Auto-scroll
                    } else {
                        maxUserScrollY = 0;
                        userScrollY = 0;
                    }
                }
            } else if (e.type == SDL_AUDIODEVICEADDED) {
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
            } else if (e.type == SDL_AUDIODEVICEREMOVED) {
                if (e.adevice.iscapture && stt.mic != 0) {
                    SDL_CloseAudioDevice(stt.mic);
                    stt.mic = 0;

                    if (stt.recognizer != nullptr) {
                        vosk_recognizer_free(stt.recognizer);
                        stt.recognizer = nullptr; 
                    }

                    isListening = false;
                    std::cout << "Microphone disconected" << std::endl;
                }
            } else if (e.type == SDL_KEYDOWN) {
                bool shift = (e.key.keysym.mod & KMOD_SHIFT);

                if (e.key.keysym.sym == SDLK_F1 && shift) {
                    if (e.key.repeat == 0) {
                        if (!isListening) {
                            if (stt.mic == 0) {
                                stt.ReInitMic();
                            }

                            if (stt.mic != 0) {
                                stt.StartListening();
                                isListening = true;
                            }
                        } else {
                            stt.StopListening();
                            isListening = false;
                        }
                    }
                } else if (e.key.keysym.sym == SDLK_BACKSPACE) {
                    Uint32 currentTime = SDL_GetTicks();
                    if (currentTime - lastBackspaceTime < BACKSPACE_DELAY) { //Ignore if 50 ms have not elapsed
                        continue;
                    }
                    lastBackspaceTime = currentTime;

                    if (cursorIndex > 0 && !UserText.empty() && cursorIndex <= UserText.length()) {
                        int start_del = cursorIndex - 1;

                        if (start_del >= static_cast<int>(UserText.length())) {
                            start_del = UserText.length() - 1;
                        }

                        while (start_del > 0 && 
                               static_cast<unsigned char>(UserText[start_del]) >= 128 && 
                               static_cast<unsigned char>(UserText[start_del]) <= 191) {
                            start_del--;
                        }

                        UserText.erase(start_del, cursorIndex - start_del);
                        cursorIndex = start_del;

                        if (!UserText.empty()) {
                            if (UserTextSurface != nullptr) SDL_FreeSurface(UserTextSurface);
                            if (UserTextTexture != nullptr) SDL_DestroyTexture(UserTextTexture);

                            UserTextSurface = TTF_RenderUTF8_Blended_Wrapped(UserTextFont, UserText.c_str(), {0, 0, 0, 255}, maxInputWidth);
                            UserTextTexture = SDL_CreateTextureFromSurface(renderer, UserTextSurface);

                            UserTextRect.w = UserTextSurface->w;
                            UserTextRect.h = UserTextSurface->h;

                            if (UserTextRect.h > (UserArea.h - 40)) {
                                maxUserScrollY = UserTextRect.h - (UserArea.h - 40);
                                if (userScrollY > maxUserScrollY) userScrollY = maxUserScrollY;
                            } else {
                                maxUserScrollY = 0;
                                userScrollY = 0;
                            }
                        } else {
                            if (UserTextTexture != nullptr) {
                                SDL_DestroyTexture(UserTextTexture);
                                UserTextTexture = nullptr;
                            }
                            maxUserScrollY = 0;
                            userScrollY = 0;
                            cursorIndex = 0;
                        }   
                    } else if (UserText.empty() || cursorIndex <= 0) {
                        if (UserText.empty()) {
                            cursorIndex = 0;
                            if (UserTextTexture != nullptr) {
                                SDL_DestroyTexture(UserTextTexture);
                                UserTextTexture = nullptr;
                            }
                            maxUserScrollY = 0;
                            userScrollY = 0;
                        }
                        if (cursorIndex <= 0) {
                            cursorIndex = 0;
                        }
                    }
                } else if (e.key.keysym.sym == SDLK_LEFT) {
                    if (cursorIndex > 0) {
                        cursorIndex--;

                        while (cursorIndex > 0 && 
                              (static_cast<unsigned char>(UserText[cursorIndex]) >= 128 && 
                               static_cast<unsigned char>(UserText[cursorIndex]) <= 191)) {
                            cursorIndex--;
                        }   
                    }
                } else if (e.key.keysym.sym == SDLK_RIGHT) {
                    if (cursorIndex < UserText.length()) {
                        cursorIndex++;
                        
                        while (cursorIndex < UserText.length() && 
                              (static_cast<unsigned char>(UserText[cursorIndex]) >= 128 && 
                               static_cast<unsigned char>(UserText[cursorIndex]) <= 191)) {
                            cursorIndex++;
                        }   
                    }
                } else if (e.key.keysym.sym == SDLK_F11) {
                    if (!fullscreen) {
                        SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN);
                        fullscreen = true;
                    } else {
                        SDL_SetWindowFullscreen(window, 0);
                        fullscreen = false;
                    }
                } else if (e.key.keysym.sym == SDLK_RETURN && !UserText.empty() && !IsThinking) {
                    std::string prompt = UserText;
                    UserText = "";

                    /*DESTROY IF LINE TEXTURE HAS TEXT*/
                    for (SDL_Texture* tex : lineTextures) {
                        if (tex && tex != thinking_texture) {
                            SDL_DestroyTexture(tex);
                        }
                    }

                    lineTextures.clear();
                    lineRects.clear();
                    parsedLines.clear();

                    lineTextures.push_back(thinking_texture); //Overwrite

                    SDL_Rect thinkingRect;
                    thinkingRect.x = ResponseArea.x + 20;
                    thinkingRect.y = ResponseArea.y + 20;

                    SDL_QueryTexture(thinking_texture, NULL, NULL, &thinkingRect.w, &thinkingRect.h);

                    lineRects.push_back(thinkingRect);

                    IsThinking = true;
                    
                    this->ResponseText = "";
                    this->LastResponse = "";

                    if (UserTextTexture != nullptr) {
                        SDL_DestroyTexture(UserTextTexture);
                        UserTextTexture = nullptr;
                    }
                    if (UserTextSurface != nullptr) {
                        SDL_FreeSurface(UserTextSurface);
                        UserTextSurface = nullptr;
                    }
                    userScrollY = 0;
                    maxUserScrollY = 0;
                    scrollY = 0;

                    std::thread AIThread([&AI, prompt, this]() {
                        #if defined(_WIN32) || defined(_WIN64)
                        ::CoInitializeEx(NULL, COINIT_MULTITHREADED); //For voice thread
                        #endif

                        std::string remoteResponse = AI.SendPrompt(prompt, LocalMode);

                        if (!remoteResponse.empty()) {
                            if (remoteResponse.rfind("[CMD_SHUTDOWN: TIME=") != std::string::npos) {
                                size_t start_lenght = remoteResponse.rfind("[CMD_SHUTDOWN: TIME=") + 20;
                                size_t end = remoteResponse.find("]", start_lenght);

                                if (end != std::string::npos) {
                                    size_t length = end - start_lenght;
                                    std::string time = remoteResponse.substr(start_lenght, length);
                                    #if defined(_WIN32) || defined(_WIN64)
                                        std::string cmd = "shutdown /s /t " + time;
                                        system(cmd.c_str());
                                    #else
                                        std::thread([time]() {
                                            std::this_thread::sleep_for(std::chrono::seconds(std::stoi(time)));
                                            system("/sbin/poweroff");
                                        }).detach();
                                    #endif
                                }
                            }

                            if (remoteResponse.rfind("[CMD_RESTART: TIME=") != std::string::npos) {
                                size_t start_lenght = remoteResponse.rfind("[CMD_RESTART: TIME=") + 19;
                                size_t end = remoteResponse.find("]", start_lenght);

                                if (end != std::string::npos) {
                                    size_t length = end - start_lenght;
                                    std::string time = remoteResponse.substr(start_lenght, length);
                                    #if defined(_WIN32) || defined(_WIN64)
                                        std::string cmd = "shutdown /r /t " + time;
                                        system(cmd.c_str());
                                    #else
                                        std::thread([time]() {
                                            std::this_thread::sleep_for(std::chrono::seconds(std::stoi(time)));
                                            system("/sbin/reboot");
                                        }).detach();
                                    #endif
                                }
                            }

                            size_t cmd_pos = remoteResponse.rfind("[CMD_EXECUTE: APP_NAME=");
                            if (cmd_pos != std::string::npos) {
                                size_t start_lenght = cmd_pos + 23;
                                size_t end = remoteResponse.find("]", start_lenght);

                                if (end != std::string::npos) {
                                    size_t length = end - start_lenght;
                                    std::string app_name = remoteResponse.substr(start_lenght, length);
                                    #if defined(_WIN32) || defined(_WIN64)
                                        if (app_name.find(".exe") != std::string::npos) {
                                            app_name = app_name.substr(0, app_name.find(".exe") + 4);
                                        }
                                        remoteResponse = remoteResponse.substr(0, cmd_pos);

                                        std::thread SearchExeThread([app_name]() {
                                            SearchExe exe;
                                            std::string app_path = exe.FindExe(app_name);
                                            std::string formatted_path = "\"" + app_path + "\"";
                                            std::vector<char> cmdBuffer(formatted_path.begin(), formatted_path.end());
                                            cmdBuffer.push_back('\0');
                                            char* lpCommandLine = cmdBuffer.data();

                                            STARTUPINFO si;
                                            PROCESS_INFORMATION pi;

                                            ZeroMemory(&si, sizeof(si));
                                            si.cb = sizeof(si);
                                            ZeroMemory(&pi, sizeof(pi));

                                            if (CreateProcessA(NULL, lpCommandLine, NULL, NULL, FALSE, 0, NULL, "C:\\", &si, &pi)) {
                                                CloseHandle(pi.hProcess);
                                                CloseHandle(pi.hThread);
                                            }
                                        });

                                        SearchExeThread.detach();
                                    #else
                                        remoteResponse = remoteResponse.substr(0, cmd_pos);
                                        signal(SIGCHLD, SIG_IGN);
                                        pid_t pid = fork();

                                        if (pid < 0) {
                                            perror("Failed to fork process for executing application");
                                        } else if (pid == 0) {
                                            const char* home_dir = getenv("HOME");
                                            
                                            if (home_dir != nullptr) {
                                                if (chdir(home_dir) != 0) {
                                                    perror("Failed to change directory");
                                                }
                                            }

                                            int devNull = open("/dev/null", O_WRONLY);

                                            dup2(devNull, STDOUT_FILENO);
                                            dup2(devNull, STDERR_FILENO);
                                            close(devNull);

                                            setsid();
                                            char* args[] = {const_cast<char*>(app_name.c_str()), NULL};
                                            execvp(args[0], args);
                                            exit(1);
                                        }
                                    #endif
                                }
                            }

                            if (remoteResponse.rfind("[CMD_SYSCMD: CMD=") != std::string::npos) {
                                size_t start_lenght = remoteResponse.rfind("[CMD_SYSCMD: CMD=") + 17;
                                size_t end = remoteResponse.find("]", start_lenght);

                                if (end != std::string::npos) {
                                    size_t length = end - start_lenght;
                                    std::string cmd = remoteResponse.substr(start_lenght, length);

                                    #if defined(_WIN32) || defined(_WIN64)
                                        std::thread SYSCMD([cmd, this]() {

                                            STARTUPINFO si;
                                            PROCESS_INFORMATION pi;

                                            ZeroMemory(&si, sizeof(si));
                                            si.cb = sizeof(si);
                                            ZeroMemory(&pi, sizeof(pi));
                                            
                                            std::string finalCommand = "cmd.exe /k " + cmd;
                                            std::vector<char> cmdBuffer(finalCommand.begin(), finalCommand.end());
                                            cmdBuffer.push_back('\0');
                                            char* lpCommandLine = cmdBuffer.data();

                                            if(CreateProcessA(NULL, lpCommandLine, NULL, NULL, FALSE, CREATE_NEW_CONSOLE, NULL, NULL, &si, &pi)){
                                                CloseHandle(pi.hProcess);
                                                CloseHandle(pi.hThread);
                                            }
                                        });

                                        SYSCMD.detach();
                                    #else
                                        std::string backgroundCmd = cmd + " &";
                                        system(backgroundCmd.c_str());
                                    #endif
                                }
                            }
       
                            if (remoteResponse.rfind("[CMD_WEBSITE: WEB_NAME=") != std::string::npos) {
                                size_t start_lenght = remoteResponse.rfind("[CMD_WEBSITE: WEB_NAME=") + 23;
                                size_t end = remoteResponse.find("]", start_lenght);

                                if (end != std::string::npos) {
                                    size_t length = end - start_lenght;
                                    std::string web_name = remoteResponse.substr(start_lenght, length);

                                    #if defined(_WIN32) || defined(_WIN64)
                                        SHELLEXECUTEINFO sei;
                                        ZeroMemory(&sei, sizeof(sei));

                                        sei.cbSize = sizeof(SHELLEXECUTEINFO);
                                        sei.fMask = SEE_MASK_NOCLOSEPROCESS;
                                        sei.hwnd = NULL;
                                        sei.lpVerb = "open";
                                        sei.lpFile = web_name.c_str();
                                        sei.nShow = SW_SHOWNORMAL;

                                        if (!ShellExecuteEx(&sei)) {
                                            DWORD error = GetLastError();
                                            std::cout << "Error to open url: " << web_name << " Error: " << error << std::endl;
                                        }
                                    #else
                                        signal(SIGCHLD, SIG_IGN);
                                        pid_t pid = fork();

                                        if (pid < 0) {
                                            perror("Failed to fork process for executing application");
                                        } else if (pid == 0) {
                                            const char* home_dir = getenv("HOME");
                                            
                                            if (home_dir != nullptr) {
                                                if (chdir(home_dir) != 0) {
                                                    perror("Failed to change directory");
                                                }
                                            }

                                            int devNull = open("/dev/null", O_WRONLY);

                                            dup2(devNull, STDOUT_FILENO);
                                            dup2(devNull, STDERR_FILENO);
                                            close(devNull);

                                            setsid();
                                            execlp("xdg-open", "xdg-open", web_name.c_str(), NULL);
                                            exit(1);
                                        }
                                    #endif
                                }
                            }

                            if (remoteResponse.rfind("[REMINDER: NAME=") != std::string::npos) {
                                size_t start_lenght = remoteResponse.rfind("[REMINDER: NAME=") + 16;
                                size_t end = remoteResponse.find("]", start_lenght);

                                if (end != std::string::npos) {
                                    size_t when_marker = remoteResponse.find(", WHEN=", start_lenght);
                                    if (when_marker != std::string::npos && when_marker < end) {
                                        size_t name_lenght = when_marker - start_lenght;
                                        std::string name = remoteResponse.substr(start_lenght, name_lenght);

                                        size_t start_when = when_marker + 7; 
                                        size_t when_lenght = end - start_when;

                                        std::string when = remoteResponse.substr(start_when, when_lenght);
                                        size_t slash_pos = when.find("/");

                                        if (slash_pos != std::string::npos) {
                                            std::string when_day = when.substr(0, slash_pos);
                                            std::string time_part = when.substr(slash_pos + 1);
                                            size_t colon_pos = time_part.find(":");

                                            if (colon_pos != std::string::npos) {
                                                try {
                                                    std::string str_hour = time_part.substr(0, colon_pos);
                                                    std::string string_end = time_part.substr(colon_pos + 1);

                                                    int hour = std::stoi(str_hour);
                                                    int minute = std::stoi(string_end.substr(0, 2));

                                                    std::string am_or_pm = "";
                                                    if (string_end.find("PM") != std::string::npos || string_end.find("pm") != std::string::npos) {
                                                        am_or_pm = "PM";
                                                    } else if (string_end.find("AM") != std::string::npos || string_end.find("am") != std::string::npos) {
                                                        am_or_pm = "AM";
                                                    }
                                                    this->reminders.CreateReminder(name, when_day, hour, minute, am_or_pm);
                                                } catch(...) {}
                                            }
                                        }
                                    }
                                }
                            }

                            //Ada Gestures ;)
                            if (remoteResponse.rfind("(alegre)") != std::string::npos) {
                                this->AdaGestures(0);
                                size_t pos = remoteResponse.find("(alegre)");
                                remoteResponse.erase(pos, remoteResponse.size() - pos);
                            } else if (remoteResponse.rfind("(sorpresa)") != std::string::npos) {
                                this->AdaGestures(1);
                                size_t pos = remoteResponse.rfind("(sorpresa)");
                                remoteResponse.erase(pos, remoteResponse.size() - pos);
                            } else if (remoteResponse.rfind("(tristeza)") != std::string::npos) {
                                this->AdaGestures(4);
                                size_t pos = remoteResponse.rfind("(tristeza)");
                                remoteResponse.erase(pos, remoteResponse.size() - pos);
                            } else if (remoteResponse.rfind("(amor)") != std::string::npos) {
                                this->AdaGestures(11);
                                size_t pos = remoteResponse.rfind("(amor)");
                                remoteResponse.erase(pos, remoteResponse.size() - pos);
                            } else if (remoteResponse.rfind("(explicación)") != std::string::npos) {
                                this->AdaGestures(10);
                                size_t pos = remoteResponse.rfind("(explicación)");
                                remoteResponse.erase(pos, remoteResponse.size() - pos);
                            } else if (remoteResponse.rfind("(festejando)") != std::string::npos) {
                                this->AdaGestures(6);
                                size_t pos = remoteResponse.rfind("(festejando)");
                                remoteResponse.erase(pos, remoteResponse.size() - pos);
                            } else if (remoteResponse.rfind("(preocupación)") != std::string::npos) {
                                this->AdaGestures(9);
                                size_t pos = remoteResponse.rfind("(preocupación)");
                                remoteResponse.erase(pos, remoteResponse.size() - pos);
                            } else if (remoteResponse.rfind("(tomando en cuenta)") != std::string::npos) {
                                this->AdaGestures(7);
                                size_t pos = remoteResponse.rfind("(tomando en cuenta)");
                                remoteResponse.erase(pos, remoteResponse.size() - pos);
                            } else if (remoteResponse.rfind("(negación)") != std::string::npos) {
                                this->AdaGestures(2);
                                size_t pos = remoteResponse.rfind("(negación)");
                                remoteResponse.erase(pos, remoteResponse.size() - pos);
                            } else if (remoteResponse.rfind("(risa nerviosa)") != std::string::npos) {
                                this->AdaGestures(12);
                                size_t pos = remoteResponse.rfind("(risa nerviosa)");
                                remoteResponse.erase(pos, remoteResponse.size() - pos);
                            } else if (remoteResponse.rfind("(interés)") != std::string::npos) {
                                this->AdaGestures(3);
                                size_t pos = remoteResponse.rfind("(interés)");
                                remoteResponse.erase(pos, remoteResponse.size() - pos);
                            } else if (remoteResponse.find("Modelo de IA saturado") != std::string::npos || 
                                       remoteResponse.find("API Key vencida o inválida") != std::string::npos ||
                                       remoteResponse.find("Limite de contexto superado.") != std::string::npos ||
                                       remoteResponse.find("Error inesperado") != std::string::npos ||
                                       remoteResponse.find("Error de conexión con el servidor.") != std::string::npos) {
                                this->AdaGestures(14);  
                            }

                            this->ResponseText = remoteResponse;

                            if (this->VoiceIsActive) {
                                std::string TalkString = voice.CleanTextForTalk(this->ResponseText);
                                voice.TalkAda(TalkString);
                            }
                        }
                        
                        this->IsThinking = false;

                        #if defined(_WIN32) || defined(_WIN64)
                        ::CoUninitialize();
                        #endif
                    });

                    AIThread.detach();
                }
            }
        }

        if (offlineSwitchResult == 1) {
            LocalMode = true;
            if(Mode_tex) SDL_DestroyTexture(Mode_tex);
            Mode_surf = TTF_RenderText_Solid(Mode_font, "Offline", {0, 0, 0, 255});
            if(Mode_surf){
                Mode_tex = SDL_CreateTextureFromSurface(renderer, Mode_surf);
                SDL_FreeSurface(Mode_surf);
                Mode_surf = nullptr;
            }
            offlineSwitchResult = 0;
        } else if (offlineSwitchResult == 2) {
            // Offline mode failed; just reset state and keep online text.
            offlineSwitchResult = 0;
        }

        if (!IsThinking && !this->ResponseText.empty() && LastResponse != this->ResponseText) {
            LastResponse = this->ResponseText;
            scrollY = 0; // Reset scroll

            // Process AI Response
            ProcessAIResponse(this->ResponseText);

            // Make response texture
            MakeResponseTexture(ResponseArea);
        }

        if (IsThinking && this->ResponseText.empty()) {
            AdaGestures(5);
        }

        if (!isMinimized) {
            SDL_SetRenderDrawColor(renderer, 244, 226, 198, 255);
            SDL_RenderClear(renderer);

            SDL_RenderCopy(renderer, Ada_SpriteSheet_texture, &Ada_src_rect, &Ada_dest_rect);

            //Reponse area and user area
            SDL_SetRenderDrawColor(renderer, 43, 43, 43, 50);
            SDL_RenderDrawRect(renderer, &ResponseArea);
            SDL_SetRenderDrawColor(renderer, 43, 43, 43, 50);
            SDL_RenderDrawRect(renderer, &UserArea);

            //Mode button
            if(LocalMode){
                SDL_SetRenderDrawColor(renderer, 200, 190, 170, 255);
            } else {
                SDL_SetRenderDrawColor(renderer, 152, 251, 152, 255);
            }
            SDL_RenderFillRect(renderer, &ModeButton);

            int TextModeW = 0, TextModeH = 0;
            SDL_QueryTexture(Mode_tex, NULL, NULL, &TextModeW, &TextModeH);

            TextMode.x = ModeButton.x + (ModeButton.w - TextModeW)  / 2;
            TextMode.y = ModeButton.y + (ModeButton.h - TextModeH) / 2;
            TextMode.w = TextModeW;
            TextMode.h = TextModeH;

            SDL_RenderCopy(renderer, Mode_tex, NULL, &TextMode);

            SDL_SetRenderDrawColor(renderer, 43, 43, 43, 50);
            SDL_RenderDrawRect(renderer, &ModeButton);

            //Copy button
            if (mx >= CopyButton.x && mx <= (CopyButton.x + CopyButton.w) &&
                my >= CopyButton.y && my <= (CopyButton.y + CopyButton.h)) {
                SDL_SetRenderDrawColor(renderer, 170, 160, 140, 255);
            } else {
                SDL_SetRenderDrawColor(renderer, 200, 190, 170, 255);
            }
            SDL_RenderFillRect(renderer, &CopyButton);  

            int CopyTextW = 0, CopyTextH = 0;
            SDL_QueryTexture(CopyTexture, NULL, NULL, &CopyTextW, &CopyTextH);

            CopyRect.x = CopyButton.x + (CopyButton.w - CopyTextW) / 2;
            CopyRect.y = CopyButton.y + (CopyButton.h - CopyTextH) / 2;
            CopyRect.w = CopyTextW;
            CopyRect.h = CopyTextH;

            SDL_RenderCopy(renderer, CopyTexture, NULL, &CopyRect);

            SDL_SetRenderDrawColor(renderer, 43, 43, 43, 255);
            SDL_RenderDrawRect(renderer, &CopyButton);

            //Voice button
            if (VoiceIsActive) {
                SDL_SetRenderDrawColor(renderer, 152, 251, 152, 255);
            } else {
                SDL_SetRenderDrawColor(renderer, 200, 190, 170, 255);
            }
            SDL_RenderFillRect(renderer, &VoiceButton);

            int TextVoiceButtonW = 0, TextVoiceButtonH = 0;
            SDL_QueryTexture(TextVoiceButton_tex, NULL, NULL, &TextVoiceButtonW, &TextVoiceButtonH);

            TextVoiceButton.x = VoiceButton.x + (VoiceButton.w - TextVoiceButtonW) / 2;
            TextVoiceButton.y = VoiceButton.y + (VoiceButton.h - TextVoiceButtonH) / 2;
            TextVoiceButton.w = TextVoiceButtonW;
            TextVoiceButton.h = TextVoiceButtonH;

            SDL_RenderCopy(renderer, TextVoiceButton_tex, NULL, &TextVoiceButton);

            SDL_SetRenderDrawColor(renderer, 43, 43, 43, 255);
            SDL_RenderDrawRect(renderer, &VoiceButton);

            // Cut with clip rect (Hides anything that overflows the text box)
            if (!lineTextures.empty()) {
                SDL_Rect clipRegion = { ResponseArea.x + 10, ResponseArea.y + 10, ResponseArea.w - 20, ResponseArea.h - 20 };
                SDL_RenderSetClipRect(renderer, &clipRegion);

                //Draw lines
                for (size_t i = 0; i < lineTextures.size(); ++i) {
                    if (lineTextures[i] != nullptr) {
                        SDL_Rect currentRenderRect = lineRects[i];
                        currentRenderRect.y -= scrollY; //Global scroll

                        SDL_RenderCopy(renderer, lineTextures[i], NULL, &currentRenderRect);
                    }
                }
                SDL_RenderSetClipRect(renderer, NULL);
            }

            if (switchingToOffline && switchingModeTexture) {
                SDL_RenderCopy(renderer, switchingModeTexture, NULL, &switchingModeRect);
            }

            /*CODE FOR USER AREA AND TEXT CURSOR ;)*/
            SDL_Rect userClipRegion = { UserArea.x + 10, UserArea.y + 10, UserArea.w - 20, UserArea.h - 20 };
            SDL_RenderSetClipRect(renderer, &userClipRegion);
        
            if (!UserText.empty() && UserTextTexture != nullptr) {
                SDL_Rect currentMinInputRect = UserTextRect;
                currentMinInputRect.y -= userScrollY; //Dynamic scroll
                SDL_RenderCopy(renderer, UserTextTexture, NULL, &currentMinInputRect);
            }

            if (cursorIndex > UserText.length()) {
                cursorIndex = UserText.length();
            }

            if (cursorIndex > 0) {
                while (cursorIndex > 0 && (static_cast<unsigned char>(UserText[cursorIndex]) & 0xC0) == 0x80) {
                    cursorIndex--;
                }
            }

            /* AUTOMATIC WRAP FOR TEXT CURSOR*/
            int padding = 20;
            int fontHeight = 0;
            bool fontValid = (UserTextFont != nullptr);
            
            if (fontValid) {
                fontHeight = TTF_FontHeight(UserTextFont);
            } else {
                std::cerr << "UserTextFont is null, cursor not drawn." << std::endl;
            }

            int lineHeight = fontValid ? TTF_FontLineSkip(UserTextFont) : 0;
            int maxWidth = UserArea.w - 2 * padding;
            int currentLineW = 0;
            int rows = 0;
            size_t lineStartIndex = 0;
            int simulatedW = 0;

            if (fontValid) {
                size_t i = 0;
                while (i < cursorIndex) {
                    if (UserText[i] == '\n') {
                        rows++;
                        i++;
                        lineStartIndex = i; 
                        simulatedW = 0;
                        continue;
                    }

                    if (UserText[i] == ' ') {
                        int spaceW, spaceH;
                        TTF_SizeUTF8(UserTextFont, " ", &spaceW, &spaceH);

                        if (simulatedW + spaceW > maxWidth && simulatedW > 0) {
                            rows++;
                            lineStartIndex = i; 
                            simulatedW = 0;
                        } else {
                            simulatedW += spaceW;
                        }
                        i++;
                        continue;
                    }

                    size_t nextDelimiter = UserText.find_first_of(" \n", i);
                    size_t endOfWord = (nextDelimiter == std::string::npos || nextDelimiter >= cursorIndex) ? cursorIndex : nextDelimiter;

                    std::string word = UserText.substr(i, endOfWord - i);
                    int wordW, wordH;
                    TTF_SizeUTF8(UserTextFont, word.c_str(), &wordW, &wordH);

                    if (simulatedW + wordW <= maxWidth) {
                        simulatedW += wordW;
                        i = endOfWord;
                    } else if (wordW <= maxWidth) {
                        rows++;
                        lineStartIndex = i; 
                        simulatedW = wordW;
                        i = endOfWord;
                    } else {
                        while (i < endOfWord) {
                            unsigned char c = (unsigned char)UserText[i];
                            int charLen = (c >= 0xF0) ? 4 : (c >= 0xE0) ? 3 : (c >= 0xC0) ? 2 : 1;
                            if (i >= cursorIndex) break;

                            std::string sub = UserText.substr(i, charLen);
                            int charW, charH;
                            TTF_SizeUTF8(UserTextFont, sub.c_str(), &charW, &charH);

                            if (simulatedW + charW > maxWidth && simulatedW > 0) {
                                rows++;
                                lineStartIndex = i; 
                                simulatedW = 0;
                            }

                            simulatedW += charW;
                            i += charLen;
                        }
                    }
                }

                if (cursorIndex > lineStartIndex) {
                    std::string finalLineStr = UserText.substr(lineStartIndex, cursorIndex - lineStartIndex);
                    int dummyH;
                    TTF_SizeUTF8(UserTextFont, finalLineStr.c_str(), &currentLineW, &dummyH);
                }

                int cursorX = UserArea.x + padding + currentLineW - 2;
                int cursorY = UserArea.y + padding + (rows * lineHeight) - userScrollY;

                if ((SDL_GetTicks() / 500) % 2 == 0) {
                    SDL_Rect cursorRect = { cursorX, cursorY, 2, fontHeight };
                    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255); 
                    SDL_RenderFillRect(renderer, &cursorRect);
                }

                SDL_RenderSetClipRect(renderer, NULL);

                std::string newMicText = stt.TextFromMic();
                if (isListening && !newMicText.empty()) {
                    UserText.insert(cursorIndex, newMicText);
                    cursorIndex += strlen(newMicText.c_str());
                    
                    if (UserTextSurface != nullptr) SDL_FreeSurface(UserTextSurface);
                    if (UserTextTexture != nullptr) SDL_DestroyTexture(UserTextTexture);

                    UserTextSurface = TTF_RenderUTF8_Blended_Wrapped(UserTextFont, UserText.c_str(), {0, 0, 0, 255}, maxInputWidth);
                    UserTextTexture = SDL_CreateTextureFromSurface(renderer, UserTextSurface);

                    UserTextRect.x = UserArea.x + 20;
                    UserTextRect.y = UserArea.y + 20;
                    UserTextRect.w = UserTextSurface->w;
                    UserTextRect.h = UserTextSurface->h;

                    if (UserTextRect.h > (UserArea.h - 40)) {
                        maxUserScrollY = UserTextRect.h - (UserArea.h - 40);
                        userScrollY = maxUserScrollY; 
                    } else {
                        maxUserScrollY = 0;
                        userScrollY = 0;
                    }
                }

                if (isListening && (SDL_GetTicks() % 1000 < 500)) {
                    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
                    DrawFilledCircle(renderer, 1200, 300, 10);
                }
            }

            reminders.CheckReminders();
            SDL_RenderPresent(renderer);

            auto frameEnd = std::chrono::high_resolution_clock::now();
            auto frameTime = std::chrono::duration_cast<std::chrono::milliseconds>(frameEnd - frameStart);

            if (frameTime < FRAME_DURATION) {
                std::this_thread::sleep_for(FRAME_DURATION - frameTime);
            }
        } else {
            SDL_Delay(100);
        }
    }
}