#ifndef __STRING_UTILS_HPP__
#define __STRING_UTILS_HPP__

#include "core/logger.hpp"
#include "system/file_utils.hpp"
#include <algorithm>
#include <string>

static auto findInput(const std::string& path, const char* input_prefix, std::string& input, const std::string& name) -> bool
{
    auto files = ls(path.c_str(), input_prefix, LS_FILES);
    for (auto& file : files) {
        if (std::string(file).ends_with("_label")) {
            continue;
}

        auto label = readLine(path + "/" + file);
        if (label != name) {
            continue;
}

        auto uscore = file.find_first_of("_");
        if (uscore != std::string::npos) {
            file.erase(uscore, std::string::npos);
            input = path + "/" + file + "_input";
            //9 characters should not overflow the 32-bit int
            return std::stoi(readLine(input).substr(0, 9)) > 0;
        }
    }
    return false;
}

static auto findFallbackInput(const std::string& path, const char* input_prefix, std::string& input) -> bool
{
    auto files = ls(path.c_str(), input_prefix, LS_FILES);
    if (files.empty()) {
        return false;
}

    std::sort(files.begin(), files.end());
    for (auto& file : files) {
        if (!std::string(file).ends_with("_input")) {
            continue;
}
        input = path + "/" + file;
        core::Logger::log_(core::LogLevel::INFO) << std::format("fallback cpu {} input: {}", input_prefix, input) << '\n';
        return true;
    }
    return false;
}
#endif //!__STRING_UTILS_HPP__
