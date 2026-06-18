#ifndef SEARCH_EXE_HPP
#define SEARCH_EXE_HPP
#include <string>
#include <algorithm>
#include <filesystem>
#include <cstdlib>
#include <vector>
#include <iostream>
#include <fstream> 
#include <unordered_set>
#include <unordered_map>
#include "json.hpp"

class SearchExe{

private:


public:

    std::string FindExe(const std::string& exeName);
};

#endif