#pragma once
#include <string>

// [ADD] "특정 URL/프로그램을 실제로 여는 방법"을 플랫폼별로 캡슐화하는 계층.
//       지금은 Windows만 지원하지만, 나중에 Linux/Mac 지원이 필요해지면
//       이 파일 안에서만 #ifdef 분기를 추가하면 되고,
//       CommandDispatcher/StreamDeckService 쪽 코드는 전혀 건드릴 필요가 없음.
namespace platform {

// 기본 브라우저로 지정된 URL을 엽니다.
// 반환값: 실행 요청이 OS에 성공적으로 전달되었는지 여부
bool openUrl(const std::string& url);

} // namespace platform
