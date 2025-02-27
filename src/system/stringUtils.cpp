#include "system/stringUtils.hpp"
#include "core/logger.hpp"
#include "system/fileUtils.hpp"

auto findInput(std::string const& path, char const* input_prefix,
                      std::string& input, std::string const& name) -> bool {
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
            // 9 characters should not overflow the 32-bit int
            return std::stoi(readLine(input).substr(0, 9)) > 0;
        }
    }
    return false;
}

auto findFallbackInput(std::string const& path, char const* input_prefix,
                              std::string& input) -> bool {
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
        core::Logger::log(core::LogLevel::INFO)
            << std::format("fallback cpu {} input: {}", input_prefix, input)
            << '\n';
        return true;
    }
    return false;
}
