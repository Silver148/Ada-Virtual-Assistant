/*


Talk.cpp
*/

#include "Talk.hpp"

void AdaVoice::TalkAda(std::string message) {

    HRESULT hr_com = ::CoInitialize(NULL);
    
    ISpVoice* pLocalVoice = NULL;

    HRESULT hr = ::CoCreateInstance(CLSID_SpVoice, NULL, CLSCTX_ALL, IID_ISpVoice, (void**)&pLocalVoice);
    
    if (SUCCEEDED(hr) && pLocalVoice != NULL) {
        
        IEnumSpObjectTokens* pEnum = NULL;
        if (SUCCEEDED(SpEnumTokens(SPCAT_VOICES, L"Gender=Female;Name=Microsoft Sabina", NULL, &pEnum))) { //By default in spanish, haha, xD
            ISpObjectToken* pToken = NULL;
            if (pEnum->Next(1, &pToken, NULL) == S_OK) {
                pLocalVoice->SetVoice(pToken);
                pToken->Release();
            }
            pEnum->Release();
        }

        if (!message.empty()) {
            int size_needed = MultiByteToWideChar(CP_UTF8, 0, &message[0], (int)message.size(), NULL, 0);
            
            std::wstring wide_text(size_needed, 0);
            
            MultiByteToWideChar(CP_UTF8, 0, &message[0], (int)message.size(), &wide_text[0], size_needed);

            pLocalVoice->Speak(wide_text.c_str(), SPF_DEFAULT, NULL); 
        }

        pLocalVoice->Release();
    }

    if (SUCCEEDED(hr_com)) {
        ::CoUninitialize();
    }
}

std::string AdaVoice::CleanTextForTalk(const std::string& message) {
    std::regex commands("\\[[^\\]]*\\]");
    std::string without_commands = std::regex_replace(message, commands, "");

    std::regex gestures("\\([^\\)]*\\)");
    std::string without_gestures = std::regex_replace(without_commands, gestures, "");

    std::regex markdown("[\\*_#`\\-]");
    std::string without_markdown = std::regex_replace(without_gestures, markdown, "");

    std::string without_emojis = "";
    without_emojis.reserve(without_markdown.size());

    for (size_t i = 0; i < without_markdown.size(); ++i) {
        unsigned char c = without_markdown[i];
        if (c >= 0xF0 && c <= 0xF4) {
            i += 3;
            continue;
        }
        else if (c == 0xE2 && (i + 2 < without_markdown.size())) {
            unsigned char next1 = without_markdown[i+1];
            if (next1 >= 0x80) {
                i += 2;
                continue;
            }
        }
        
        without_emojis += without_markdown[i];
    }

    std::regex spaces(" +");
    std::string clean_text = std::regex_replace(without_emojis, spaces, " ");

    if (!clean_text.empty() && clean_text.front() == ' ') {
        clean_text.erase(0, 1);
    }
    if (!clean_text.empty() && clean_text.back() == ' ') {
        clean_text.pop_back();
    }

    return clean_text;
}