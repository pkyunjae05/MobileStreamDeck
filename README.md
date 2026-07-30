# MobileStreamDeck

스마트폰을 유선/무선으로 연결하여 PC를 제어하는 커스텀 스트림 덱(Stream Deck) 프로젝트입니다.  


## 기술 스택 (Tech Stack)

- PC (Server): C++ (libusb)
- Android (Client): Java ()
- Development Environment: Visual Studio Code (VS Code)

---

## 📂 프로젝트 구조 (Directory Structure)

```text
my-mobile-streamdeck/
├── pc-server/            # C++ 기반 Windows 백그라운드 서버 프로그램
└── android-client/       # Java 기반 안드로이드 스트림덱 앱 프로젝트

## 프로젝트 로드맵
- [x] 프로젝트 초기 환경 세팅 및 구조 설계
- [x] `libusb` 기반 안드로이드 디바이스 탐색 (PC)
- [x] pc <-> app read/write 확인
- [x] app -> pc 아이콘 클릭 하여 명령어 보내기