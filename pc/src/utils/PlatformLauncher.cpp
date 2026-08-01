#include "PlatformLauncher.h"
#include "Logger.h"

#ifdef _WIN32
    #include <windows.h>
    #include <shellapi.h>
#endif

namespace platform {

bool openUrl(const std::string& url) {
#ifdef _WIN32
    // ShellExecuteW는 브라우저 경로를 직접 알 필요 없이
    // 사용자가 Windows에 지정해둔 "기본 브라우저"로 URL을 열어줌.
    HINSTANCE result = ShellExecuteA(
        nullptr,        // 부모 윈도우 없음
        "open",         // 동작: 열기
        url.c_str(),    // 대상 URL
        nullptr,        // 파라미터 없음
        nullptr,        // 작업 디렉터리 기본값
        SW_SHOWNORMAL   // 창을 보통 크기로 표시
    );

    // ShellExecute는 성공 시 32보다 큰 값을 반환 (문서화된 규약)
    auto code = reinterpret_cast<INT_PTR>(result);
    if (code <= 32) {
        LOG_ERROR("ShellExecute failed to open URL '%s' (code=%lld)", url.c_str(), (long long)code);
        return false;
    }

    LOG_INFO("Opened URL via default browser: %s", url.c_str());
    return true;
#else
    // [ADD] Windows 외 플랫폼은 아직 미지원. 필요 시 각 OS의 오픈 명령으로 구현.
    LOG_WARN("openUrl() is not implemented on this platform. url=%s", url.c_str());
    return false;
#endif
}

} // namespace platform
