#ifndef __FILE_UTILS_HPP__
#define __FILE_UTILS_HPP__

#include <filesystem>
#include <string>
#include <vector>

enum LsFlags {
    LS_DIRS = 0x01,
    LS_FILES = 0x02,
};

std::string readLine(std::string const& filename);
std::vector<std::string> ls(std::filesystem::path const& root,
                            std::string_view prefix = "",
                            LsFlags flags = LS_DIRS);
bool fileExists(std::string const& path);
bool dirExists(std::string const& path);

#endif  //__FILE_UTILS_HPP__
