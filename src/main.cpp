#include "config.hpp"
#include "logger.hpp"
#include "share_memory.hpp"

#include <csignal>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

#ifndef SHAREMEMORY_SERVER_DEV
#define SHAREMEMORY_SERVER_DEV 1271
#endif
#ifndef SHAREMEMORY_CLIENT_VER
#define SHAREMEMORY_CLIENT_VER 533
#endif

namespace {
ShareMemoryService* g_service = nullptr;

void handleSignal(int) {
    if (g_service != nullptr) {
        g_service->stop();
    }
}

/// Same ASCII banner as legacy Linux ShareMemory foreground (see `D:\yuan.txt`).
void printStartupBanner() {
    static const char* lines[] = {
        "#     #                                                    ",
        "#     # #    # #####  #####  #  ####    ##   #    # ###### ",
        "####### #    # #    # #    # # #      #    # # #  # #####  ",
        "#     # #    # #####  #####  # #      ###### #  # # #      ",
        "#     # #    # #   #  #   #  # #    # #    # #   ## #      ",
        "#     #  ####  #    # #    # #  ####  #    # #    # ###### ",
    };
    for (const char* line : lines) {
        std::cout << line << '\n';
    }
    std::cout.flush();
}

/// `YYMMDDHHMM` stamp for `ShareMemory Starting... (…)(0)` (legacy shape).
std::string yyMmDdHhMmStamp() {
    const auto t = std::time(nullptr);
    std::tm tmv{};
#ifdef _WIN32
    localtime_s(&tmv, &t);
#else
    localtime_r(&t, &tmv);
#endif
    std::ostringstream oss;
    oss << std::setfill('0') << std::setw(2) << ((tmv.tm_year + 1900) % 100) << std::setw(2) << (tmv.tm_mon + 1)
        << std::setw(2) << tmv.tm_mday << std::setw(2) << tmv.tm_hour << std::setw(2) << tmv.tm_min;
    return oss.str();
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

    printStartupBanner();

    {
        std::ostringstream hur;
        hur << "(###Hurricane###) Server Dev:" << SHAREMEMORY_SERVER_DEV
            << " ClientVer:" << SHAREMEMORY_CLIENT_VER;
        const std::string hurLine = hur.str();
        Logger::instance().logShare(hurLine);
        // Legacy binary also writes the same version lines to Debug_*.log (see yuan Debug_*.log).
        Logger::instance().logDebug(hurLine);
    }
    const std::string startingLine =
        std::string("ShareMemory Starting... (") + yyMmDdHhMmStamp() + ")(0)";
    Logger::instance().logShare(startingLine);
    Logger::instance().logDebug(startingLine);

    std::signal(SIGINT, handleSignal);
    std::signal(SIGTERM, handleSignal);

    Logger::instance().logShare("Start Read Config Files...");
    Config cfg;
    if (!cfg.load(configPath)) {
        Logger::instance().logShare("Read Config Files...FAIL!");
        std::cerr << "failed to load config: " << configPath << '\n';
        return 1;
    }
    Logger::instance().logShare("Read Config Files...OK!");

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
