# MobileStreamDeck

This is a custom Stream Deck project that controls a PC by connecting a smartphone via a wired or wireless connection.


# Setup

## PC

### 1. WinUSB Driver Installation (Windows Only)
You only need to install the WinUSB driver the **first time** your device switches to AOA mode.

1. Download [Zadig](https://zadig.akeo.ie/).
2. Connect your Android device to your PC via USB.
3. Run `StreamDeckPC` and attempt a handshake (the device will switch to AOA mode).
4. Open Zadig, go to **Options**, and check **List All Devices**.
5. Select `Android Accessory` (VID: 18D1, PID: 2D01) from the dropdown.
6. Set the target driver to **WinUSB**, then click **Install/Replace Driver**.

### 2. Build
Configure and build the project using CMake:

bash
cmake -B build -S . -DCMAKE_BUILD_TYPE=Debug
cmake --build build --config Debug

### 3. Run

build\StreamDeckPC.exe

## Android

### 1. Build
Build the project to generate the APK.

### 2. Install
Transfer and install the APK onto your Android device.

# Directory Structure

MobileStreamDeck/
├── pc(server)/
└── android(client)/

## 폴더 구조

### PC (`pc/src/`)

```text
pc/src/
├── core/
│   ├── UsbManager.h/.cpp
│   ├── TcpManager.h/.cpp
│   └── CommandManager.h/.cpp
├── service/
│   └── StreamDeckService.h/.cpp
├── transport/
│   ├── TransportInputOutput.h
│   ├── Transport.h/.cpp
│   ├── PipeTransport.h/.cpp
│   ├── UsbTransport.h/.cpp
│   └── TcpTransport.h/.cpp
├── protocol/
│   ├── AoaConstants.h
│   └── AoaProtocol.h/.cpp
├── interface/
│   ├── IConnectionObserver.h
│   └── IMessageObserver.h
└── utils/
    ├── Logger.h
    └── PlatformLauncher.h/.cpp
```

### Android (`app/src/main/`)

```text
app/src/main/
├── java/com/pkj/app/streamdeckcontroller/
│   ├── MainActivity.java
│   ├── usb/
│   │   └── AoaCommunicationManager.java
│   └── tcp/
│       └── tcpCommunicator.java
├── res/
│   ├── layout/
│   │   └── activity_main.xml
│   ├── values/
│   │   └── strings.xml
│   └── drawable/
│       └── ic_action_play.xml
└── AndroidManifest.xml
```
