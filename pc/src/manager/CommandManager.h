#pragma once
#include <string>
#include <unordered_map>
#include <functional>

// [ADD] 기존 CommandDispatcher의 역할(명령 문자열 <-> 실행 함수 등록/조회)을
//       이 클래스가 그대로 흡수함. UsbManager와 동일하게 싱글톤으로 구성해
//       프로젝트 내 매니저 클래스들의 디자인 일관성을 맞춤.
//
// 사용 흐름:
//   1) CommandManager::getInstance().initialize()  -> 내부적으로 등록 함수 실행
//   2) StreamDeckService가 수신 문자열을 dispatch(text)로 넘김
//   3) 등록된 명령이면 해당 핸들러(PlatformLauncher 호출 등)가 실행됨
class CommandManager {
public:
    using Handler = std::function<void()>;

    static CommandManager& getInstance();
    CommandManager(const CommandManager&) = delete;
    CommandManager& operator=(const CommandManager&) = delete;

    // 내부 명령 테이블을 채움. 앱 시작 시 한 번 호출.
    void initialize();

    // 명령 문자열에 대응하는 실행 함수를 등록.
    // 이미 등록된 명령이면 덮어씀 (마지막 등록이 우선).
    void registerCommand(const std::string& command, Handler handler);

    // 명령을 조회해서 실행. 등록되지 않은 명령이면 false 반환(호출자가 별도 처리 가능).
    bool dispatch(const std::string& command) const;

private:
    CommandManager() = default;

    std::unordered_map<std::string, Handler> m_handlers;
};
