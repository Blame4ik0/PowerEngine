#pragma once
#include <string>
#include <format>

namespace Engine
{
    class Logger
    {
    public:
        enum class Level { Info, Warn, Error };

        static void Log(Level level, const std::string& message);

        template<typename... Args>
        static void Info(std::format_string<Args...> fmt, Args&&... args)
        {
            Log(Level::Info, std::format(fmt, std::forward<Args>(args)...));
        }

        template<typename... Args>
        static void Warn(std::format_string<Args...> fmt, Args&&... args)
        {
            Log(Level::Warn, std::format(fmt, std::forward<Args>(args)...));
        }

        template<typename... Args>
        static void Error(std::format_string<Args...> fmt, Args&&... args)
        {
            Log(Level::Error, std::format(fmt, std::forward<Args>(args)...));
        }
    };
}

#define LOG_INFO(...)  ::Engine::Logger::Info(__VA_ARGS__)
#define LOG_WARN(...)  ::Engine::Logger::Warn(__VA_ARGS__)
#define LOG_ERROR(...) ::Engine::Logger::Error(__VA_ARGS__)