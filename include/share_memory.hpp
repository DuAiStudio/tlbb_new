#pragma once

#include "config.hpp"
#include "odbc_interface.hpp"
#include "types.hpp"

#include <atomic>
#include <cstdint>
#include <memory>
#include <utility>
#include <vector>

class ISmuLogicManager {
public:
    virtual ~ISmuLogicManager() = default;
    virtual bool init(const ShareMemData& data) = 0;
    virtual void doDefault() = 0;
    virtual void doNormalSave() = 0;
    virtual void doSaveAll() = 0;
    virtual void heartbeat(bool forceSave, bool saveLogoutMode, bool crashSave) = 0;
};

class ShareMemoryService {
public:
    bool init(const Config& config);
    bool run();
    void stop();

private:
    bool handleCommandFiles();

    const Config* config_{nullptr};
    OdbcInterface odbc_;
    std::atomic<bool> exiting_{false};
    std::atomic<bool> forceSaveAll_{false};
    std::atomic<bool> saveLogoutEnabled_{false};
    std::atomic<bool> pendingExitAfterSave_{false};
    std::atomic<bool> pendingCleanBattleGuild_{false};
    std::atomic<bool> pendingCrashSave_{false};
    std::vector<std::pair<ShareMemType, std::unique_ptr<ISmuLogicManager>>> managers_;
    std::vector<std::uint32_t> managerIntervalsMs_;
    std::vector<std::uint32_t> managerElapsedMs_;
};
