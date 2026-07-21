#pragma once
#include <cstdio>
#include <cstdarg>
#include <chrono>
#include <ctime>
#include <mutex>

namespace sd {
namespace log {

enum class Level { Debug = 0, Info, Warn, Error };

inline Level g_minLevel = Level::Debug;
inline std::mutex g_mutex;

inline const char* tag(Level lv) {
    switch (lv) {
        case Level::Debug: return "[DBG]";
        case Level::Info:  return "[INF]";
        case Level::Warn:  return "[WRN]";
        case Level::Error: return "[ERR]";
    }
    return "[???]";
}

inline void print(Level level, const char* fmt, ...) {
    if (level < g_minLevel) return;

    std::lock_guard<std::mutex> lock(g_mutex);

    auto now = std::chrono::system_clock::now();
    auto ms  = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
    auto t   = std::chrono::system_clock::to_time_t(now);

    std::tm tm{};
#ifdef _WIN32
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif

    std::fprintf(stdout, "%02d:%02d:%02d.%03d %s ",
                 tm.tm_hour, tm.tm_min, tm.tm_sec,
                 static_cast<int>(ms.count()), tag(level));

    va_list args;
    va_start(args, fmt);
    std::vfprintf(stdout, fmt, args);
    va_end(args);

    std::fprintf(stdout, "\n");
    std::fflush(stdout);
}

} // namespace log
} // namespace sd

#define LOG_DEBUG(...) sd::log::print(sd::log::Level::Debug, __VA_ARGS__)
#define LOG_INFO(...)  sd::log::print(sd::log::Level::Info,  __VA_ARGS__)
#define LOG_WARN(...)  sd::log::print(sd::log::Level::Warn,  __VA_ARGS__)
#define LOG_ERROR(...) sd::log::print(sd::log::Level::Error, __VA_ARGS__)
