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
        << "  /find             scan and list Android devices\n"
        << "  /wired            start wired connection now\n"
        << "  /wireless         start wireless connection now\n"
        << "  /disconnect       disconnect\n"
        << "  /quit             exit\n"
        << "  <text>            send text (only when connected)\n\n";
}

int main() {
    std::signal(SIGINT, onSigInt);

    std::cout << "===============================================\n";
    std::cout << "             MobileStreamDeck PC               \n";
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

        if (line == "/quit") {
            break;
        }
        if (line == "/wired") {
            bool ok = service.wiredConnect();
            LOG_INFO("Service Connect=%s", ok ? "OK" : "FAIL");
            continue;
        }
        if (line == "/wireless") {
            bool ok = service.wirelessConnect();
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