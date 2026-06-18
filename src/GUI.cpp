/*
GUI ;)

GUI.cpp
*/

#include "GUI.hpp"
#include "SearchEXE.hpp"
#include <windows.h>
#define FRAME_WIDTH 205
#define FRAME_HEIGHT 200

GUI::GUI(){
    SDL_Init(SDL_INIT_VIDEO);
    IMG_Init(IMG_INIT_PNG);
    TTF_Init();

    window = SDL_CreateWindow("Ada", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 1280, 720, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    Ada_SpriteSheet_surface = IMG_Load("Ada_SpriteSheet.png");

    Ada_SpriteSheet_texture = SDL_CreateTextureFromSurface(renderer, Ada_SpriteSheet_surface);

    if(Ada_SpriteSheet_surface){
        SDL_FreeSurface(Ada_SpriteSheet_surface);
        Ada_SpriteSheet_surface = nullptr;
    }
        
    Ada_src_rect.x = 0;
    Ada_src_rect.y = 0;
    Ada_src_rect.w = FRAME_WIDTH;
    Ada_src_rect.h = FRAME_HEIGHT;

    Ada_dest_rect.x = 1280 / 2 - FRAME_WIDTH / 2;
    Ada_dest_rect.y = 0;
    Ada_dest_rect.w = FRAME_WIDTH;
    Ada_dest_rect.h = FRAME_HEIGHT;

    UserTextFont = TTF_OpenFont("fonts/Segoe-UI-EMOJI.ttf", 20);
    AdaTextFont = TTF_OpenFont("fonts/Segoe-UI-EMOJI.ttf", 20);
    CopyFont = TTF_OpenFont("fonts/Segoe-UI-EMOJI.ttf", 16);
    
    CopySurface = TTF_RenderText_Solid(CopyFont, "Copy", {0, 0, 0});
    CopyTexture = SDL_CreateTextureFromSurface(renderer, CopySurface);

    if(CopySurface){
        SDL_FreeSurface(CopySurface);
        CopySurface = nullptr;
    }

    r = Reminders();
    r.LoadReminders();
}

GUI::~GUI() {

    if(Ada_SpriteSheet_texture) {
        SDL_DestroyTexture(Ada_SpriteSheet_texture);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    IMG_Quit();
    TTF_CloseFont(UserTextFont);
    TTF_CloseFont(AdaTextFont);
    TTF_Quit();
    SDL_Quit();
}


/* PARSER MD4C */

static std::vector<RenderLine>* g_lines = nullptr;
static RenderLine currentLine;
static bool nextTextIsBold = false;

int cb_enter_block(MD_BLOCKTYPE type, void* detail, void* userdata) {
    currentLine.tokens.clear();
    
    if (type == MD_BLOCK_H) {
        currentLine.type = LINE_TITLE;
        if (detail != nullptr) {
            MD_BLOCK_H_DETAIL* h_detail = static_cast<MD_BLOCK_H_DETAIL*>(detail);
            if (h_detail->level > 1) currentLine.type = LINE_SUBTITLE;
        }
    }
    else if (type == MD_BLOCK_LI) {
        currentLine.type = LINE_BULLET;
    }
    else {
        currentLine.type = LINE_NORMAL;
    }
    return 0;
}

int cb_leave_block(MD_BLOCKTYPE type, void* detail, void* userdata) {
    if (!currentLine.tokens.empty()) {
        g_lines->push_back(currentLine);
        currentLine.tokens.clear();
    }
    return 0;
}

int cb_enter_span(MD_SPANTYPE type, void* detail, void* userdata) {
    if (type == MD_SPAN_STRONG) {
        nextTextIsBold = true;
    }
    return 0;
}

int cb_leave_span(MD_SPANTYPE type, void* detail, void* userdata) {
    if (type == MD_SPAN_STRONG) {
        nextTextIsBold = false;
    }
    return 0;
}

int cb_text(MD_TEXTTYPE type, const MD_CHAR* text, MD_SIZE size, void* userdata) {
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
        if (tex) SDL_DestroyTexture(tex);
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

/* FUNCS GUI*/
void GUI::GestosAda(int id){

    int column = id % 5;
    int row = id / 5;

    Ada_src_rect.x = column * FRAME_WIDTH;
    Ada_src_rect.y = row * FRAME_HEIGHT;
}

void GUI::RenderGui(AI_ENGINE AI){
    bool running = true;
    bool fullscreen = false;

    SDL_Rect ResponseArea = { 140, 250, 1000, 350 };
    SDL_Rect UserArea = { 140, 630, 1000, 80 };

    CopyButton = { 
    ResponseArea.x + ResponseArea.w - 100, // x = 140 + 1000 - 100 = 1040
    ResponseArea.y + ResponseArea.h - 45,  // y = 250 + 350 - 45 = 555
    80,                                    // Ancho
    30                                     // Alto
    };

    std::string LastResponse = "";
    int maxInputWidth = UserArea.w - 40;

    SDL_RenderSetLogicalSize(renderer, 1280, 720);

    SDL_StartTextInput();

    while(running){

        SDL_Event e;
        while(SDL_PollEvent(&e)){
            if(e.type == SDL_QUIT){
                running = false;
            }

            //Scroll
            else if (e.type == SDL_MOUSEWHEEL) {
                int mouseX, mouseY;
                SDL_GetMouseState(&mouseX, &mouseY);

                if (mouseX >= UserArea.x && mouseX <= (UserArea.x + UserArea.w) &&
                    mouseY >= UserArea.y && mouseY <= (UserArea.y + UserArea.h)) 
                {
                    if (e.wheel.y > 0) { // Up
                        userScrollY -= 20;
                        if (userScrollY < 0) userScrollY = 0;
                    }
                    else if (e.wheel.y < 0) { // Down
                        userScrollY += 20;
                        if (userScrollY > maxUserScrollY) userScrollY = maxUserScrollY;
                    }
                } 
                else {
                    if (e.wheel.y > 0) { // Up
                        scrollY -= 24;
                        if (scrollY < 0) scrollY = 0;
                    }
                    else if (e.wheel.y < 0) { // Down
                        scrollY += 24;
                        if (scrollY > maxScrollY) scrollY = maxScrollY;
                    }
                }
            }

            else if (e.type == SDL_MOUSEBUTTONDOWN) {
                if (e.button.button == SDL_BUTTON_LEFT) { // Clic izquierdo
                    int mouseX = e.button.x;
                    int mouseY = e.button.y;

                    if (mouseX >= CopyButton.x && mouseX <= (CopyButton.x + CopyButton.w) &&
                        mouseY >= CopyButton.y && mouseY <= (CopyButton.y + CopyButton.h)) 
                    {
                        if (!this->ResponseText.empty()) {
                        SDL_SetClipboardText(this->ResponseText.c_str());
                
                        }
                    }
                }
            }

            else if(e.type == SDL_TEXTINPUT){

                if (static_cast<unsigned char>(e.text.text[0]) >= 32) {
                    UserText += e.text.text;
                
                    if(UserTextSurface != nullptr) SDL_FreeSurface(UserTextSurface);
                    if(UserTextTexture != nullptr) SDL_DestroyTexture(UserTextTexture);

                    UserTextSurface = TTF_RenderUTF8_Blended_Wrapped(UserTextFont, UserText.c_str(), {0, 0, 0}, maxInputWidth);
                    UserTextTexture = SDL_CreateTextureFromSurface(renderer, UserTextSurface);

                    UserTextRect.x = UserArea.x + 20;
                    UserTextRect.y = UserArea.y + 20;
                    UserTextRect.w = UserTextSurface->w;
                    UserTextRect.h = UserTextSurface->h;

                    if (UserTextRect.h > (UserArea.h - 40)) {
                        maxUserScrollY = UserTextRect.h - (UserArea.h - 40);
                        // Auto-scroll
                        userScrollY = maxUserScrollY; 
                    } else {
                        maxUserScrollY = 0;
                        userScrollY = 0;
                    }

                }
            }

            else if(e.type == SDL_KEYDOWN){
                if(e.key.keysym.sym == SDLK_BACKSPACE && !UserText.empty()){

                    size_t cut_pos = UserText.length() - 1;
                    while (cut_pos > 0 && 
                           static_cast<unsigned char>(UserText[cut_pos]) >= 128 && 
                           static_cast<unsigned char>(UserText[cut_pos]) <= 191) 
                    {
                        cut_pos--;
                    }

                    UserText.resize(cut_pos);

                    if(!UserText.empty()){
                        if(UserTextSurface != nullptr) SDL_FreeSurface(UserTextSurface);
                        if(UserTextTexture != nullptr) SDL_DestroyTexture(UserTextTexture);

                        UserTextSurface = TTF_RenderUTF8_Blended_Wrapped(UserTextFont, UserText.c_str(), {0, 0, 0}, maxInputWidth);
                        UserTextTexture = SDL_CreateTextureFromSurface(renderer, UserTextSurface);

                        UserTextRect.x = UserArea.x + 20;
                        UserTextRect.y = UserArea.y + 20;
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
                        if(UserTextTexture != nullptr) {
                            SDL_DestroyTexture(UserTextTexture);
                            UserTextTexture = nullptr;
                        }
                        maxUserScrollY = 0;
                        userScrollY = 0;
                    }

                }

                else if(e.key.keysym.sym == SDLK_F11){
                    if(!fullscreen){
                        SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN);
                        fullscreen = true;
                    }
                    else{
                        SDL_SetWindowFullscreen(window, 0);
                        fullscreen = false;
                    }
                }

                else if(e.key.keysym.sym == SDLK_RETURN){

                    std::string prompt = UserText;
                    UserText = "";

                    if(UserTextTexture != nullptr) {
                        SDL_DestroyTexture(UserTextTexture);
                        UserTextTexture = nullptr;
                    }
                    if(UserTextSurface != nullptr) {
                        SDL_FreeSurface(UserTextSurface);
                        UserTextSurface = nullptr;
                    }
                    userScrollY = 0;
                    maxUserScrollY = 0;

                    std::thread AIThread([&AI, prompt, this](){
                        this->ResponseText = AI.SendPrompt(prompt);

                        if(!this->ResponseText.empty()){

                            if(this->ResponseText.rfind("[CMD_SHUTDOWN: TIME=") != std::string::npos){

                                size_t start_lenght = this->ResponseText.rfind("[CMD_SHUTDOWN: TIME=") + 20;
                                size_t end = this->ResponseText.find("]", start_lenght);

                                if(end != std::string::npos){
                                    size_t length = end - start_lenght;
                                    std::string time = this->ResponseText.substr(start_lenght, length);

                                    std::string cmd = "shutdown /s /t " + time;
                                    system(cmd.c_str()); //BYEEEE :D

                                }
                            }

                            if(this->ResponseText.rfind("[CMD_RESTART: TIME=") != std::string::npos){

                                size_t start_length = this->ResponseText.rfind("[CMD_RESTART: TIME=") + 23;
                                size_t end = this->ResponseText.find("]", start_length);

                                if(end != std::string::npos){
                                    size_t length = end - start_length;
                                    std::string time = this->ResponseText.substr(start_length, length);

                                    std::string cmd = "shutdown /r /t " + time;
                                    system(cmd.c_str()); //RESTART ;)

                                }
                            }

                            size_t cmd_pos = this->ResponseText.rfind("[CMD_EXECUTE: APP_NAME=");

                            if(cmd_pos != std::string::npos){
                                size_t start_length = cmd_pos + 23;
                                size_t end = this->ResponseText.find("]", start_length);

                                if(end != std::string::npos){
                                    size_t length = end - start_length;
                                    std::string app_name = this->ResponseText.substr(start_length, length);

                                    if (app_name.find(".exe") != std::string::npos) {
                                        app_name = app_name.substr(0, app_name.find(".exe") + 4);
                                    }

                                    this->ResponseText = this->ResponseText.substr(0, cmd_pos);

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

                                        BOOL execute = CreateProcessA(
                                                NULL,
                                                lpCommandLine,
                                                NULL,
                                                NULL,
                                                FALSE,
                                                0,
                                                NULL,
                                                NULL,
                                                &si,
                                                &pi 
                                            );

                                        if (execute) {
                                            CloseHandle(pi.hProcess);
                                            CloseHandle(pi.hThread);
                                        }
                                        
                                    });

                                    SearchExeThread.detach();

                                }
                            }

                            if(this->ResponseText.rfind("[REMINDER: NAME=") != std::string::npos){
                                
                                size_t start_length = this->ResponseText.rfind("[REMINDER: NAME=") + 16;
                                size_t end = this->ResponseText.find("]", start_length);

                                if(end != std::string::npos){

                                    size_t when_marker = this->ResponseText.find(", WHEN=", start_length);

                                    if (when_marker != std::string::npos && when_marker < end){
                                        size_t name_length = when_marker - start_length;
                                        std::string name = this->ResponseText.substr(start_length, name_length);

                                        size_t start_when = when_marker + 7; 
                                        size_t when_length = end - start_when;
                                        std::string when = this->ResponseText.substr(start_when, when_length);

                                        size_t slash_pos = when.find("/");

                                        if (slash_pos != std::string::npos) {
                                            std::string when_day = when.substr(0, slash_pos);

                                            std::string time_part = when.substr(slash_pos + 1);

                                            size_t colon_pos = time_part.find(":");

                                            if (colon_pos != std::string::npos) {
                                                try{
                                                    std::string str_hora = time_part.substr(0, colon_pos);
                                                    std::string string_end = time_part.substr(colon_pos + 1);

                                                    int hora = std::stoi(str_hora);
                                                    int minuto = std::stoi(string_end.substr(0, 2));

                                                    std::string am_or_pm = "";
                                                    if(string_end.find("PM") != std::string::npos || string_end.find("pm") != std::string::npos){
                                                        am_or_pm = "PM";
                                                    }else if(string_end.find("AM") != std::string::npos || string_end.find("am") != std::string::npos){
                                                        am_or_pm = "AM";
                                                    }

                                                    this->r.CreateReminder(name, when_day, hora, minuto, am_or_pm);
                                                }
                                                catch(const std::exception& e){
                                                }
                                            }
                                        }
                                    }

                                }
                            }

                            if(this->ResponseText.rfind("(alegre)") != std::string::npos){
                                this->GestosAda(0);
                                size_t pos = this->ResponseText.find("(alegre)");
                                this->ResponseText.erase(pos, this->ResponseText.size() - pos);
                            }
                            else if(this->ResponseText.rfind("(sorpresa)") != std::string::npos){
                                this->GestosAda(1);
                                size_t pos = this->ResponseText.rfind("(sorpresa)");
                                this->ResponseText.erase(pos, this->ResponseText.size() - pos);
                            }
                            else if(this->ResponseText.rfind("(tristeza)") != std::string::npos){
                                this->GestosAda(4);
                                size_t pos = this->ResponseText.rfind("(tristeza)");
                                this->ResponseText.erase(pos, this->ResponseText.size() - pos);
                            }
                            
                            else if(this->ResponseText.rfind("(amor)") != std::string::npos){
                                this->GestosAda(11);
                                size_t pos = this->ResponseText.rfind("(amor)");
                                this->ResponseText.erase(pos, this->ResponseText.size() - pos);
                            }
                            else if(this->ResponseText.rfind("(explicación)") != std::string::npos){
                                this->GestosAda(10);
                                size_t pos = this->ResponseText.rfind("(explicación)");
                                this->ResponseText.erase(pos, this->ResponseText.size() - pos);
                            }
                            else if(this->ResponseText.rfind("(festejando)") != std::string::npos){
                                this->GestosAda(6);
                                size_t pos = this->ResponseText.rfind("(festejando)");
                                this->ResponseText.erase(pos, this->ResponseText.size() - pos);
                            }
                            else if(this->ResponseText.rfind("(preocupación)") != std::string::npos){
                                this->GestosAda(9);
                                size_t pos = this->ResponseText.rfind("(preocupación)");
                                this->ResponseText.erase(pos, this->ResponseText.size() - pos);
                            }
                            else if(this->ResponseText.rfind("(tomando en cuenta)") != std::string::npos){
                                this->GestosAda(7);
                                size_t pos = this->ResponseText.rfind("(tomando en cuenta)");
                                this->ResponseText.erase(pos, this->ResponseText.size() - pos);
                            }
                            std::string TalkString = voice.CleanTextForTalk(this->ResponseText);
                            voice.TalkAda(TalkString);

                            //std::cout << "Ada says: " << this->ResponseText << std::endl;
                                
                        }
                    });

                    AIThread.detach();
                }
            }
        }

        if (LastResponse != this->ResponseText && !this->ResponseText.empty()) {
            LastResponse = this->ResponseText;
            scrollY = 0; // Reiniciamos scroll

            //Process AI Response
            ProcessAIResponse(this->ResponseText);

             //Make response texture
            MakeResponseTexture(ResponseArea);
        }

        SDL_SetRenderDrawColor(renderer, 244, 226, 198, 255);

        SDL_RenderClear(renderer);

        SDL_RenderCopy(renderer, Ada_SpriteSheet_texture, &Ada_src_rect, &Ada_dest_rect);

        //Reponse area and user area
        SDL_SetRenderDrawColor(renderer, 43, 43, 43, 50);
        SDL_RenderDrawRect(renderer, &ResponseArea);
        SDL_SetRenderDrawColor(renderer, 43, 43, 43, 50);
        SDL_RenderDrawRect(renderer, &UserArea);

        int mx, my;
        SDL_GetMouseState(&mx, &my);

        if (mx >= CopyButton.x && mx <= (CopyButton.x + CopyButton.w) &&
            my >= CopyButton.y && my <= (CopyButton.y + CopyButton.h)) {
            SDL_SetRenderDrawColor(renderer, 170, 160, 140, 255);
        } else {
            SDL_SetRenderDrawColor(renderer, 200, 190, 170, 255);
        }
        SDL_RenderFillRect(renderer, &CopyButton);  

        int textW = 0, textH = 0;
        SDL_QueryTexture(CopyTexture, NULL, NULL, &textW, &textH);

        CopyRect.x = CopyButton.x + (CopyButton.w - textW) / 2;
        CopyRect.y = CopyButton.y + (CopyButton.h - textH) / 2;
        CopyRect.w = textW;
        CopyRect.h = textH;

        SDL_RenderCopy(renderer, CopyTexture, NULL, &CopyRect);

        //Copy button
        SDL_SetRenderDrawColor(renderer, 43, 43, 43, 255);
        SDL_RenderDrawRect(renderer, &CopyButton);

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

        if(!UserText.empty() && UserTextTexture != nullptr) {
            //Make a mask
            SDL_Rect userClipRegion = { UserArea.x + 10, UserArea.y + 10, UserArea.w - 20, UserArea.h - 20 };
            SDL_RenderSetClipRect(renderer, &userClipRegion);

            SDL_Rect currentMinInputRect = UserTextRect;
            currentMinInputRect.y -= userScrollY; //dinamic scroll

            SDL_RenderCopy(renderer, UserTextTexture, NULL, &currentMinInputRect);

            //Break the mask
            SDL_RenderSetClipRect(renderer, NULL);
        }

        r.CheckReminders();

        SDL_RenderPresent(renderer);

    }
}