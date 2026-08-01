cmake_minimum_required(VERSION 3.20)
project(MobileStreamDeck VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

# libusb path (your folder structure)
set(LIBUSB_ROOT "${CMAKE_SOURCE_DIR}/lib/libusb-1.0.29")
set(LIBUSB_INCLUDE_DIR "${LIBUSB_ROOT}/include")
set(LIBUSB_DLL_DIR "${LIBUSB_ROOT}/MinGW64/dll")
set(LIBUSB_LIBRARY "${LIBUSB_ROOT}/MinGW64/static/libusb-1.0.a")

if(NOT EXISTS "${LIBUSB_INCLUDE_DIR}/libusb.h")
    message(FATAL_ERROR "libusb.h not found: ${LIBUSB_INCLUDE_DIR}/libusb.h")
endif()
if(NOT EXISTS "${LIBUSB_LIBRARY}")
    message(FATAL_ERROR "libusb library not found: ${LIBUSB_LIBRARY}")
endif()

message(STATUS "Compiler  : ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "libusb inc: ${LIBUSB_INCLUDE_DIR}")
message(STATUS "libusb lib: ${LIBUSB_LIBRARY}")

# --- 변경된 구조에 맞게 소스 파일 목록 업데이트 ---
set(SOURCES
    src/main.cpp
    src/manager/UsbManager.cpp
    src/manager/CommandManager.cpp
    src/protocol/AoaProtocol.cpp
    src/service/StreamDeckService.cpp
    src/transport/UsbTransport.cpp
    src/utils/PlatformLauncher.cpp
)

# --- 변경된 구조에 맞게 헤더 파일 목록 업데이트 ---
set(HEADERS
    src/interface/IConnectionObserver.h
    src/interface/IMessageObserver.h
    src/manager/UsbManager.h
    src/manager/CommandManager.h
    src/protocol/AoaConstants.h
    src/protocol/AoaProtocol.h
    src/service/StreamDeckService.h
    src/transport/UsbTransport.h
    src/utils/Logger.h
    src/utils/PlatformLauncher.h
)

add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/src
)

# external header warnings suppressed
target_include_directories(${PROJECT_NAME} SYSTEM PRIVATE
    ${LIBUSB_INCLUDE_DIR}
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    ${LIBUSB_LIBRARY}
)

if(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE
        setupapi
        ws2_32
    )
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        _CRT_SECURE_NO_WARNINGS
        NOMINMAX
        WIN32_LEAN_AND_MEAN
    )
endif()

if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /W4 /utf-8)
else()
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra)
endif()

# Copy DLL next to exe
if(WIN32 AND EXISTS "${LIBUSB_DLL_DIR}/libusb-1.0.dll")
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${LIBUSB_DLL_DIR}/libusb-1.0.dll"
            $<TARGET_FILE_DIR:${PROJECT_NAME}>
        COMMENT "Copying libusb-1.0.dll to output directory"
    )
endif()