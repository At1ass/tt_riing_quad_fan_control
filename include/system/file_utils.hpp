#ifndef __FILE_UTILS_HPP__
#define __FILE_UTILS_HPP__

#include <string>
#include <vector>
enum LS_FLAGS
{
    LS_DIRS = 0x01,
    LS_FILES = 0x02,
};

std::string readLine(const std::string& filename);
std::vector<std::string> ls(const char* root, const char* prefix = nullptr, LS_FLAGS flags = LS_DIRS);
bool fileExists(const std::string& path);
bool dirExists(const std::string& path);

#endif //__FILE_UTILS_HPP__
