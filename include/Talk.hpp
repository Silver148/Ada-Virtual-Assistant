#ifndef TALK_HPP
#define TALK_HPP
#include <iostream>
#include <string>
#include <regex>
#include <windows.h>
#include <sapi.h>
#include <sphelper.h>

class AdaVoice{

private:
    ISpVoice* pVoice;
    HRESULT hr;

public:
    
    void TalkAda(std::string message);
    AdaVoice();
    ~AdaVoice();
    std::string CleanTextForTalk(const std::string& message);
    void ShutUpAda();

};

#endif