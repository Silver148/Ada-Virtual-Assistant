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
#ifdef _WIN32

    HWND hwnd = FindWindowA(NULL, "Ada"); 

    NOTIFYICONDATA nid = { sizeof(NOTIFYICONDATA) };
    nid.hWnd = hwnd;
    nid.uID = 100;
    nid.uFlags = NIF_INFO | NIF_ICON | NIF_TIP;
    nid.dwInfoFlags = NIIF_INFO;
        
    strncpy(nid.szInfoTitle, title, sizeof(nid.szInfoTitle) - 1);
    strncpy(nid.szInfo, message, sizeof(nid.szInfo) - 1);
    strncpy(nid.szTip, "Ada", sizeof(nid.szTip) - 1);

    Shell_NotifyIcon(NIM_ADD, &nid);
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

    std::ofstream file(get_config_path() + "/reminders.json");

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

            std::ofstream file(get_config_path() + "/reminders.json"); //Save changes

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

    std::ifstream file(get_config_path() + "/reminders.json");

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