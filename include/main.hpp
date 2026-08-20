#ifndef MAIN_HPP
#define MAIN_HPP
#include <filesystem>
#include <cstdlib>
#include <string>
#include <libgen.h>

namespace fs = std::filesystem;

std::string get_config_path();
std::string get_base_dir();


#endif