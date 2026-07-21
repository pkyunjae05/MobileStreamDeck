# Stream Deck PC — Phase 1 PoC

## 프로젝트 구조

```
StreamDeck-PC/
├── CMakeLists.txt                        # 빌드 설정
├── README.md                             # 이 파일
│
├── src/
│   ├── main.cpp                          # Phase 1 테스트 엔트리 포인트
│   │
│   ├── communication/
│   │   ├── AoaConstants.h                # AOA 프로토콜 상수
│   │   ├── AoaHostManager.h              # [Singleton] AOA 관리자 헤더
│   │   ├── AoaHostManager.cpp            # AOA 핸드셰이크 + 통신 구현
│   │   ├── IConnectionObserver.h         # [Observer] 연결 상태 인터페이스
│   │   └── IMessageObserver.h            # [Observer] 메시지 수신 인터페이스
│   │
│   └── utils/
│       └── Logger.h                      # 콘솔 로거
│
├── tests/
│   └── test_aoa_connection.cpp           # 연결 테스트
│
└── third_party/
    └── nlohmann/                         # (Phase 2에서 추가)
```

## 적용 디자인 패턴

| 패턴 | 클래스 | 설명 |
|------|--------|------|
| **Singleton** | `AoaHostManager` | USB 연결 인스턴스 유일성 보장 |
| **Observer** | `IConnectionObserver` | 연결/해제 이벤트 알림 |
| **Observer** | `IMessageObserver` | 수신 데이터 알림 |

## 빌드 방법

### 사전 요구사항

- CMake 3.20+
- C++17 지원 컴파일러 (MSVC 2022, GCC 9+, Clang 10+)
- libusb 1.0

### Windows (vcpkg)

```powershell
# 1. vcpkg로 libusb 설치
vcpkg install libusb:x64-windows

# 2. 빌드
cmake -B build -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build build --config Debug

# 3. 실행
.\build\Debug\StreamDeckPC.exe
```

### Windows (수동 libusb)

```powershell
# libusb 릴리즈 다운로드: https://github.com/libusb/libusb/releases
# 압축 해제 후:

cmake -B build ^
    -DLIBUSB_INCLUDE_DIR=C:/libusb/include ^
    -DLIBUSB_LIBRARY=C:/libusb/VS2022/MS64/static/libusb-1.0.lib
cmake --build build --config Debug
```

### Linux

```bash
# 1. libusb 설치
sudo apt install libusb-1.0-0-dev    # Ubuntu/Debian
# sudo dnf install libusb1-devel      # Fedora

# 2. 빌드
cmake -B build
cmake --build build

# 3. 실행 (root 또는 udev 규칙 필요)
sudo ./build/StreamDeckPC
```

## WinUSB 드라이버 설치 (Windows 필수)

AOA 모드 전환 후 **최초 1회** WinUSB 드라이버를 설치해야 합니다.

1. [Zadig](https://zadig.akeo.ie/) 다운로드
2. Android 기기를 USB로 연결
3. StreamDeckPC 실행 → 핸드셰이크 시도 (기기가 AOA 모드로 전환됨)
4. Zadig 실행 → Options → List All Devices 체크
5. `Android Accessory` (VID: 18D1, PID: 2D01) 선택
6. 드라이버를 `WinUSB` 로 설정 → Install/Replace Driver 클릭

## 실행 및 테스트

```
StreamDeck> Hello from PC          # 텍스트 전송
StreamDeck> /ping                  # PING 전송
StreamDeck> /status                # 연결 상태 확인
StreamDeck> /help                  # 도움말
StreamDeck> /quit                  # 종료
```

## Phase 1 완료 기준

- [x] AOA 핸드셰이크 성공 (Android 권한 다이얼로그 표시)
- [ ] Android → PC 텍스트 전송 성공
- [ ] PC → Android 텍스트 전송 성공
- [ ] USB 분리/재연결 감지
- [ ] Observer 패턴 동작 확인
