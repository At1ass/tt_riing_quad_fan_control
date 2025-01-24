#ifndef __LOGGER_HPP__
#define __LOGGER_HPP__

#include <iostream>
#include <fstream>
#include <string>
#include <mutex>
#include <sstream>

namespace core {
    enum class LogLevel {
        INFO,
        WARNING,
        ERROR
    };

#ifdef ENABLE_INFO_LOGS
#define ENABLE_INFO_LOGS 1
#else
#define ENABLE_INFO_LOGS 0
#endif

    class Logger {
        public:
            static Logger log_;

            Logger& operator()(LogLevel level); // Установка уровня лога
            template<typename T>
                Logger& operator<<(const T& value) {
                    if constexpr (std::is_arithmetic_v<T> || std::is_convertible_v<T, std::string> ||
                            std::is_invocable_v<decltype(std::declval<std::ostream&>() << std::declval<T>()), std::ostream&, T>) {

                        std::scoped_lock lock(logMutex);

                        if (!shouldLog(currentLevel)) {
                            return *this;
                        }

                        logBuffer << value;
                        flush(value); // Немедленный вывод в консоль/файл
                    } else {
                        static_assert(std::is_arithmetic_v<T> || std::is_convertible_v<T, std::string>,
                                "Logger operator<< can only be used with types that support ostream output.");
                    }

                    return *this;
                }

            Logger& operator<<(std::ostream& (*manip)(std::ostream&));

            void setLogFile(const std::string& filename); // Установить файл для логирования
            void enableConsoleLogging(bool enable); // Включить/выключить логирование в консоль
            void enableColorLogging(bool enable); // Включить/выключить цветной вывод

        private:
            Logger(); // Закрытый конструктор (Singleton)
            ~Logger();

            std::ofstream logFile;
            bool logToConsole = true;
            bool useColor = true;
            std::mutex logMutex; // Потокобезопасность

            LogLevel currentLevel = LogLevel::INFO; // Текущий уровень лога
            std::ostringstream logBuffer; // Буфер для лог-сообщения

            // Вывод буфера в лог
            template <typename T>
                void flush(const T& value, bool forceNewLine = false) {
                    std::string logMessage = logBuffer.str();

                    if (logMessage.empty()) {
                        return;
                    }

                    if (logToConsole) {
                        std::cout << getColorCode(currentLevel) << value << resetColor();
                        if (forceNewLine) { std::cout.flush();
                        }
                    }

                    if (logFile.is_open()) {
                        logFile << logMessage;
                        if (forceNewLine) { logFile.flush();
                        }
                    }
                }
            std::string getColorCode(LogLevel level) const;
            std::string resetColor() const;
            static std::string getTimestamp() ;
            static bool shouldLog(LogLevel level);
    };
};
#endif // __LOGGER_HPP__
