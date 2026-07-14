/*
Reminders :D

Reminders.cpp
*/

#include "Reminders.hpp"

Reminders::Reminders(){
    J_reminders = json::array();
    now = 0;
    local_time = nullptr;
}

void Reminders::ShowNotification(const char* title, const char* message){
#if defined(_WIN32) || defined(_WIN64)

    HWND hwnd = FindWindowA(NULL, "Ada"); 
    if (!hwnd) {
        hwnd = GetConsoleWindow(); 
    }

    NOTIFYICONDATAA nid = {};
    nid.cbSize = sizeof(NOTIFYICONDATAA);
    nid.hWnd = hwnd;
    nid.uID = 100;
    
    nid.uFlags = NIF_INFO | NIF_ICON | NIF_TIP;
    
    nid.dwInfoFlags = NIIF_USER | NIIF_LARGE_ICON; 
    
    nid.hIcon = (HICON)LoadImageA(
        GetModuleHandleA(NULL), 
        MAKEINTRESOURCEA(1), 
        IMAGE_ICON, 
        GetSystemMetrics(SM_CXICON),
        GetSystemMetrics(SM_CYICON),
        LR_SHARED
    );

    if (!nid.hIcon) {
        nid.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    }

    memset(nid.szInfoTitle, 0, sizeof(nid.szInfoTitle));
    memset(nid.szInfo, 0, sizeof(nid.szInfo));
    memset(nid.szTip, 0, sizeof(nid.szTip));

    snprintf(nid.szInfoTitle, sizeof(nid.szInfoTitle), "%s", title);
    snprintf(nid.szInfo, sizeof(nid.szInfo), "%s", message);
    snprintf(nid.szTip, sizeof(nid.szTip), "Ada");

    if (!Shell_NotifyIconA(NIM_ADD, &nid)) {
        Shell_NotifyIconA(NIM_MODIFY, &nid);
    }
#else
    // Versión para Linux
    static bool initialized = false;
    if (!initialized) {
        notify_init("Ada");
        initialized = true;
    }

    NotifyNotification* n = notify_notification_new(title, message, "dialog-information");
    
    notify_notification_set_urgency(n, NOTIFY_URGENCY_CRITICAL);

    if (!notify_notification_show(n, NULL)) {
        std::cerr << "Failed to show notification" << std::endl;
    }

    notify_notification_set_timeout(n, 0);

    if (notify_notification_show(n, NULL)) {
        usleep(100000);
    }
    
    g_object_unref(G_OBJECT(n));
#endif
}

std::string Reminders::GetDateAndHour(){
    auto hour = std::chrono::system_clock::now();
    now = std::chrono::system_clock::to_time_t(hour);

    local_time = std::localtime(&now);
    
    char buffer[128];

    std::strftime(buffer, sizeof(buffer), "%d/%m/%Y %H:%M:%S", local_time);

    return std::string(buffer);
}

void Reminders::CreateReminder(std::string name, std::string when_day, int hour, int minute, std::string am_or_pm) {
    
    if (am_or_pm == "PM" && hour < 12) {
        hour += 12;
    } else if (am_or_pm == "AM" && hour == 12) {
        hour = 0;
    }

    int day = 0, month = 0, year = 0;

    if (when_day == "TODAY") {
        std::time_t t = std::time(nullptr);
        std::tm* now = std::localtime(&t);
        
        day = now->tm_mday;
        month = now->tm_mon + 1;
        year = now->tm_year + 1900;
    }else if(when_day == "TOMORROW"){
        std::time_t t = std::time(nullptr);
    
        //24 hours in seconds
        t += 86400; 
    
        std::tm* tomorrow = std::localtime(&t);
    
        day = tomorrow->tm_mday;
        month = tomorrow->tm_mon + 1;
        year = tomorrow->tm_year + 1900;
    }

    std::ostringstream ss;
    ss << std::setfill('0') 
       << std::setw(2) << day << "/"
       << std::setw(2) << month << "/"
       << std::setw(4) << year << " "
       << std::setw(2) << hour << ":"
       << std::setw(2) << minute << ":00";

    std::string formattedDate = ss.str();

    //Save in a json
    json newReminder;
    newReminder["name"] = name;
    newReminder["date"] = formattedDate;

    J_reminders.push_back(newReminder);

    #if defined(__linux__) || defined(__unix__)
    std::ofstream file(get_config_path() + "/reminders.json");
    #else
    std::ofstream file("reminders.json");
    #endif

    if (file.is_open()) {
        file << J_reminders.dump(4);
        file.close();
    }
}

void Reminders::CheckReminders(){

    std::string date = GetDateAndHour();

    for(auto it = J_reminders.begin(); it != J_reminders.end();){

        if((*it)["date"] == date){

            ShowNotification("Reminder", (*it)["name"].get<std::string>().c_str());

            it = J_reminders.erase(it); //Delete old reminder

            #if defined(__linux__) || (__unix__)
            std::ofstream file(get_config_path() + "/reminders.json"); //Save changes
            #else
            std::ofstream file("reminders.json"); //Save changes
            #endif

            if (file.is_open()) {
                file << J_reminders.dump(4);
                file.close();
            }

        }else{
            ++it;
        }
    }
}

void Reminders::LoadReminders(){

    #if defined(__linux__) || (__unix__)
    std::ifstream file(get_config_path() + "/reminders.json");
    #else
    std::ifstream file("reminders.json");
    #endif

    if(file.is_open()){
        try{
            file >> J_reminders;
        }catch(const json::exception& e){
            J_reminders = json::array();
        }
    }else{
        J_reminders = json::array();
    }

}