#include "core/logger.hpp"

#include <ctime>
#include <iomanip>
#include <ios>
#include <iostream>
#include <ostream>
#include <sstream>
#include <string>
// Получение экземпляра логгера (Singleton)

namespace core {

Logger Logger::log;

Logger::Logger() = default;

Logger::~Logger() {
    std::scoped_lock<std::mutex> const LOCK(logMutex);
    if (logFile.is_open()) {
        logFile.close();
    }
}

std::string Logger::getTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto now_c = std::chrono::system_clock::to_time_t(now);

    std::ostringstream oss;
    oss << std::put_time(std::localtime(&now_c), "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

auto Logger::getColorCode(LogLevel level) const -> std::string {
    if (!useColor) {
        return "";
    }
    switch (level) {
        case LogLevel::INFO:
            return "\033[32m";  // Зеленый
        case LogLevel::WARNING:
            return "\033[33m";  // Желтый
        case LogLevel::ERROR:
            return "\033[31m";  // Красный
        default:
            return "\033[0m";
    }
}

auto Logger::resetColor() const -> std::string {
    return useColor ? "\033[0m" : "";
}

auto Logger::shouldLog(LogLevel level) -> bool {
    if constexpr (!INFO_LOGS_ENABLE) {
        return level != LogLevel::INFO;
    }
    return true;
}

auto Logger::operator()(LogLevel level) -> Logger& {
    std::scoped_lock const LOCK(logMutex);

    if (!shouldLog(level)) {
        return *this;  // INFO игнорируется, если флаг выключен
    }

    currentLevel = level;
    logBuffer.str("");  // Очищаем буфер перед новой записью
    logBuffer.clear();
    logBuffer << getTimestamp() << " " << "["
              << (level == LogLevel::INFO      ? "INFO"
                  : level == LogLevel::WARNING ? "WARNING"
                                               : "ERROR")
              << "] ";
    flush(logBuffer.str());
    return *this;
}

auto Logger::operator<<(std::ostream& (*manip)(std::ostream&)) -> Logger& {
    std::scoped_lock const LOCK(logMutex);

    if (!shouldLog(currentLevel)) {
        return *this;  // INFO игнорируется, если флаг выключен
    }

    if (manip == static_cast<std::ostream& (*)(std::ostream&)>(std::endl)) {
        logBuffer.clear();
        currentLevel = LogLevel::INFO;
        flush("\n", true);  // Завершаем строку
    }
    return *this;
}

void Logger::setLogFile(std::string const& filename) {
    std::scoped_lock const LOCK(logMutex);
    flush(logBuffer.str(), true);
    logFile.open(filename, std::ios::app);
}

void Logger::enableConsoleLogging(bool enable) {
    std::scoped_lock const LOCK(logMutex);
    logToConsole = enable;
}

void Logger::enableColorLogging(bool enable) {
    std::scoped_lock const LOCK(logMutex);
    useColor = enable;
}

};  // namespace core
