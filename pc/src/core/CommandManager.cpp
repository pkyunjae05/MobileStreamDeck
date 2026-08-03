#include "CommandManager.h"
#include "../utils/PlatformLauncher.h"
#include "../utils/Logger.h"

CommandManager& CommandManager::getInstance() {
    static CommandManager inst;
    return inst;
}

void CommandManager::initialize() {
    registerCommand("Youtube", []() {
        platform::openUrl("https://www.youtube.com");
    });

    // registerCommand("Spotify", []() { platform::openUrl("https://open.spotify.com"); });
}

void CommandManager::registerCommand(const std::string& command, Handler handler) {
    m_handlers[command] = std::move(handler);
}

bool CommandManager::dispatch(const std::string& command) const {
    auto it = m_handlers.find(command);
    if (it == m_handlers.end()) {
        return false;
    }

    LOG_INFO("Dispatching command: %s", command.c_str());
    it->second();
    return true;
}
