#include "CommandManager.h"
#include "../utils/PlatformLauncher.h"
#include "../utils/Logger.h"

CommandManager& CommandManager::getInstance() {
    static CommandManager inst;
    return inst;
}

// [ADD] 여기에 실제 지원하는 명령들을 등록.
//       새 버튼/명령이 늘어나면 이 함수 안에 registerCommand 한 줄만 추가하면 됨.
//       StreamDeckService나 다른 코드는 전혀 건드릴 필요 없음.
void CommandManager::initialize() {
    registerCommand("Youtube", []() {
        platform::openUrl("https://www.youtube.com");
    });

    // [향후 확장 예시]
    // registerCommand("Spotify", []() { platform::openUrl("https://open.spotify.com"); });
}

void CommandManager::registerCommand(const std::string& command, Handler handler) {
    m_handlers[command] = std::move(handler);
}

bool CommandManager::dispatch(const std::string& command) const {
    auto it = m_handlers.find(command);
    if (it == m_handlers.end()) {
        return false; // 등록되지 않은 명령
    }

    LOG_INFO("Dispatching command: %s", command.c_str());
    it->second();
    return true;
}
