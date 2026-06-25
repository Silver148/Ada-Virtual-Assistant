#ifndef MAIN_HPP
#define MAIN_HPP
#include <filesystem>
#include <cstdlib>
#include <string>

namespace fs = std::filesystem;

    #if defined(__linux__) || defined(__unix__)
    std::string get_config_path();

    #endif

#endif