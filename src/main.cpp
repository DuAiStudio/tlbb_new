#include "config.hpp"
#include "logger.hpp"
#include "share_memory.hpp"

#include <csignal>
#include <iostream>

namespace {
ShareMemoryService* g_service = nullptr;

void handleSignal(int) {
    if (g_service != nullptr) {
        g_service->stop();
    }
}
}  // namespace

int main(int argc, char** argv) {
    std::string configPath = "Config/ShareMemInfo.ini";
    if (argc > 1) {
        configPath = argv[1];
    }

    Logger::instance().init(
#ifdef _WIN32
        "D:/Log"
#else
        "./Log"
#endif
    );
    Logger::instance().logShare("(###) main...");
    Logger::instance().logShare("ShareMemory Start...");

    std::signal(SIGINT, handleSignal);
    std::signal(SIGTERM, handleSignal);

    Config cfg;
    if (!cfg.load(configPath)) {
        Logger::instance().logShare("Read Config Files...FAIL!");
        std::cerr << "failed to load config: " << configPath << '\n';
        return 1;
    }

    ShareMemoryService service;
    g_service = &service;
    if (!service.init(cfg)) {
        Logger::instance().logShare("New Managers...FAIL!");
        std::cerr << "service init failed\n";
        return 2;
    }

    if (!service.run()) {
        Logger::instance().logShare("Loop...FAIL");
        std::cerr << "service run failed\n";
        return 3;
    }
    Logger::instance().logShare("Exit ShareMemory Program");
    return 0;
}
