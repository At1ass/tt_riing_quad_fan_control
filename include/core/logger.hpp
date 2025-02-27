#ifndef __LOGGER_HPP__
#define __LOGGER_HPP__

#include <fstream>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>
#include <string_view>

namespace core {

enum class LogLevel { INFO, WARNING, ERROR };

#ifdef ENABLE_INFO_LOGS
constexpr bool INFO_LOGS_ENABLE = true;
#else
constexpr bool INFO_LOGS_ENABLE = false;
#endif

class Logger {
   public:
    Logger(Logger const&) = delete;
    Logger(Logger&&) = delete;
    Logger& operator=(Logger const&) = delete;
    Logger& operator=(Logger&&) = delete;

    static Logger log;

    Logger& operator()(LogLevel level);
    template <typename T>
    Logger& operator<<(T const& value) {
        if constexpr (std::is_same_v<T, std::string_view>) {
            logBuffer.write(value.data(), static_cast<std::streamsize>(value.size()));
        } else if constexpr (std::is_arithmetic_v<T> ||
                      std::is_convertible_v<T, std::string> ||
                      std::is_invocable_v<decltype(std::declval<std::ostream&>()
                                                   << std::declval<T>()),
                                          std::ostream&, T>) {
            std::scoped_lock lock(logMutex);

            if (!shouldLog(currentLevel)) {
                return *this;
            }

            logBuffer << value;
            flush(value);
        } else {
            static_assert(std::is_arithmetic_v<T> ||
                              std::is_convertible_v<T, std::string>,
                          "Logger operator<< can only be used with types that "
                          "support ostream output.");
        }

        return *this;
    }

    Logger& operator<<(std::ostream& (*manip)(std::ostream&));

    void setLogFile(
        std::string const& filename);
    void enableConsoleLogging(
        bool enable);
    void enableColorLogging(bool enable);

   private:
    Logger();
    ~Logger();

    std::ofstream logFile;
    bool logToConsole = true;
    bool useColor = true;
    std::mutex logMutex;

    LogLevel currentLevel = LogLevel::INFO;
    std::ostringstream logBuffer;

    template <typename T>
    void flush(T const& value, bool force_new_line = false) {
        std::string log_message = logBuffer.str();

        if (log_message.empty()) {
            return;
        }

        if (logToConsole) {
            std::cout << getColorCode(currentLevel) << value << resetColor();
            if (force_new_line) {
                std::cout.flush();
            }
        }

        if (logFile.is_open()) {
            logFile << log_message;
            if (force_new_line) {
                logFile.flush();
            }
        }
    }
    std::string getColorCode(LogLevel level) const;
    std::string resetColor() const;
    static std::string getTimestamp();
    static bool shouldLog(LogLevel level);
};

}  // namespace core
#endif  // __LOGGER_HPP__
