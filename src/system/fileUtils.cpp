#include "system/fileUtils.hpp"

#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <cerrno>
#include <climits>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <vector>

#ifndef PROCDIR
#define PROCDIR "/proc"
#endif

auto readLine(std::string const& filename) -> std::string {
    std::string line;
    std::ifstream file(filename);
    if (file.fail()) {
        return line;
    }
    std::getline(file, line);
    return line;
}

auto getBasename(std::string const&& path) -> std::string {
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
auto ls(std::filesystem::path const& root, std::string_view prefix,
        LsFlags flags) -> std::vector<std::string> {
    std::vector<std::string> list;
    std::error_code ec;

    namespace fs = std::filesystem;
    if (!fs::exists(root, ec)) {
        std::cerr << std::format("Error: directory '{}' does not exist.\n",
                                 root.string());
        return list;
    }

    for (auto const& entry : fs::directory_iterator(root, ec)) {
        if (ec) {
            std::cerr << std::format("Error iterating directory '{}': {}\n",
                                     root.string(), ec.message());
            break;
        }

        std::string name = entry.path().filename().string();

        if ((!prefix.empty() && !name.starts_with(prefix)) || name == "." ||
            name == "..") {
            continue;
        }

        if (entry.is_symlink(ec)) {
            auto target_status = fs::status(entry, ec);
            if (ec) continue;
            if ((flags & LS_DIRS) && fs::is_directory(target_status)) {
                list.push_back(name);
            } else if ((flags & LS_FILES) &&
                       fs::is_regular_file(target_status)) {
                list.push_back(name);
            }
        } else if (entry.is_directory(ec)) {
            if (flags & LS_DIRS) {
                list.push_back(name);
            }
        } else if (entry.is_regular_file(ec)) {
            if (flags & LS_FILES) {
                list.push_back(name);
            }
        }
    }

    return list;
}

auto fileExists(std::string const& path) -> bool {
    struct stat s{};
    return (stat(path.c_str(), &s) == 0) && !S_ISDIR(s.st_mode);
}

auto dirExists(std::string const& path) -> bool {
    struct stat s{};
    return (stat(path.c_str(), &s) == 0) && S_ISDIR(s.st_mode);
}

#endif  // __linux__
