#ifndef REMINDERS_HPP
#define REMINDERS_HPP
#include <iostream>
#include <fstream>
#include <chrono>
#include <ctime>
#include <string>
#include <sstream>
#include <iomanip>
#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>

#if defined(_WIN32) || defined(_WIN64)
    #include <windows.h>
    #include <shellapi.h>
#else
    extern "C" {
        #include <libnotify/notify.h>
    }
    #include "main.hpp"
#endif
#include "json.hpp"

using json = nlohmann::json;

class Reminders{

private:
    json J_reminders;
    std::time_t now;
    std::tm* local_time;

public:

    Reminders();
    std::string GetDateAndHour();
    void CreateReminder(std::string name, std::string when_day, int hour, int minute, std::string am_or_pm);
    void LoadReminders();

    void CheckReminders();
    void ShowNotification(const char* title, const char* message);

};

#endif