#pragma once
#include <string>
#include <unordered_map>
#include <functional>

class CommandManager {
public:
    using Handler = std::function<void()>;

    static CommandManager& getInstance();
    CommandManager(const CommandManager&) = delete;
    CommandManager& operator=(const CommandManager&) = delete;

    void initialize();

    void registerCommand(const std::string& command, Handler handler);
    bool dispatch(const std::string& command) const;

private:
    CommandManager() = default;

    std::unordered_map<std::string, Handler> m_handlers;
};
