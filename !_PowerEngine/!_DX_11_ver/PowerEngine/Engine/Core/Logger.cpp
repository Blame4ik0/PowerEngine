#include "Logger.h"
#include <iostream>
#include <chrono>

namespace Engine
{
    static constexpr const char* RESET = "\033[0m";
    static constexpr const char* GREY = "\033[90m";
    static constexpr const char* CYAN = "\033[96m";
    static constexpr const char* YELLOW = "\033[93m";
    static constexpr const char* RED = "\033[91m";

    static std::string CurrentTime()
    {
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()) % 1000;
        char buf[16];
        std::strftime(buf, sizeof(buf), "%H:%M:%S", std::localtime(&time));
        return std::format("{}.{:03d}", buf, (int)ms.count());
    }

    void Logger::Log(Level level, const std::string& message)
    {
        const char* levelColor = CYAN;
        const char* levelTag = "INFO";

        switch (level)
        {
        case Level::Warn:
            levelColor = YELLOW;
            levelTag = "WARN";
            break;
        case Level::Error:
            levelColor = RED;
            levelTag = "ERROR";
            break;
        default:
            break;
        }

        std::cout << GREY << "[" << CurrentTime() << "] "
            << levelColor << "[" << levelTag << "] "
            << RESET << message << "\n";
    }
}