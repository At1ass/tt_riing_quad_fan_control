#include "system/file_utils.hpp"
#include <cerrno>
#include <climits>
#include <cstring>
#include <dirent.h>
#include <fstream>
#include <iostream>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

#ifndef PROCDIR
#define PROCDIR "/proc"
#endif

auto readLine(const std::string& filename) -> std::string
{
    std::string line;
    std::ifstream file(filename);
    if (file.fail()){
        return line;
    }
    std::getline(file, line);
    return line;
}

auto getBasename(const std::string&& path) -> std::string
{
    auto npos = path.find_last_of("/\\");
    if (npos == std::string::npos) {
        return path;
}

    if (npos < path.size() - 1) {
        return path.substr(npos + 1);
}
    return path;
}

#ifdef __linux__
auto ls(const char* root, const char* prefix, LS_FLAGS flags) -> std::vector<std::string>
{
    std::vector<std::string> list;
    struct dirent* dp = nullptr;

    DIR* dirp = opendir(root);
    if (dirp == nullptr) {
        std::cerr << std::format("Error opening directory '{}': {}", root, strerror(errno)) << '\n';
        return list;
    }

    while ((dp = readdir(dirp)) != nullptr) {
        if (((prefix != nullptr) && !std::string(dp->d_name).starts_with(prefix))
            || (strcmp(dp->d_name, ".") == 0)
            || (strcmp(dp->d_name, "..") == 0)) {
            continue;
}

        switch (dp->d_type) {
        case DT_LNK: {
            struct stat s{};
            std::string path(root);
            if (path.back() != '/') {
                path += "/";
}
            path += dp->d_name;

            if (stat(path.c_str(), &s) != 0) {
                continue;
}

            if ((((flags & LS_DIRS) != 0) && S_ISDIR(s.st_mode))
                || (((flags & LS_FILES) != 0) && S_ISREG(s.st_mode))) {
                list.emplace_back(dp->d_name);
            }
            break;
        }
        case DT_DIR:
            if ((flags & LS_DIRS) != 0) {
                list.emplace_back(dp->d_name);
}
            break;
        case DT_REG:
            if ((flags & LS_FILES) != 0) {
                list.emplace_back(dp->d_name);
}
            break;
        }
    }

    closedir(dirp);
    return list;
}

auto fileExists(const std::string& path) -> bool
{
    struct stat s{};
    return (stat(path.c_str(), &s) == 0) && !S_ISDIR(s.st_mode);
}

auto dirExists(const std::string& path) -> bool
{
    struct stat s{};
    return (stat(path.c_str(), &s) == 0) && S_ISDIR(s.st_mode);
}

#endif // __linux__
