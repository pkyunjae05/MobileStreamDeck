#include "service/StreamDeckService.h"
#include "utils/Logger.h"

#include <iostream>
#include <string>
#include <atomic>
#include <csignal>

static std::atomic<bool> g_loop{true};
static void onSigInt(int) { g_loop.store(false); }

static void printHelp() {
    std::cout
        << "\nCommands:\n"
        << "  /help         show help\n"
        << "  /status       connection status\n"
        << "  /find         scan and list Android devices\n" // <-- Added
        << "  /connect      start connection now\n"
        << "  /disconnect   disconnect\n"
        << "  /quit         exit\n"
        << "  <text>        send text (only when connected)\n\n";
}

int main() {
    std::signal(SIGINT, onSigInt);

    std::cout << "===============================================\n";
    std::cout << "MobileStreamDeck PC - Phase 1 PoC (Refactored)\n";
    std::cout << "No hotplug. Connection starts on /connect.\n";
    std::cout << "===============================================\n\n";

    StreamDeckService service;

    if (!service.initialize()) {
        LOG_ERROR("Service initialization failed");
        return 1;
    }

    printHelp();

    while (g_loop.load()) {
        std::cout << "MSD> ";
        std::cout.flush();

        std::string line;
        if (!std::getline(std::cin, line)) break;
        if (line.empty()) continue;

        if (line == "/help") {
            printHelp();
            continue;
        }
        if (line == "/quit") {
            break;
        }
        if (line == "/status") {
            LOG_INFO("connected=%s", service.isConnected() ? "YES" : "NO");
            continue;
        }
        if (line == "/connect") {
            bool ok = service.connect();
            LOG_INFO("Service Connect=%s", ok ? "OK" : "FAIL");
            continue;
        }
        if (line == "/disconnect") {
            service.disconnect();
            continue;
        }
        if (line == "/find") {
            service.findDevices();
            continue;
        }

        // send text
        service.sendMessage(line);
    }

    LOG_INFO("Shutdown...");
    service.shutdown();
    return 0;
}