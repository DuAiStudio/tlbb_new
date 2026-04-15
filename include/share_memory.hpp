#pragma once

#include "config.hpp"
#include "odbc_interface.hpp"
#include "share_mem_linux.hpp"
#include "types.hpp"

#include <atomic>
#include <cstdint>
#include <memory>
#include <string>
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
    std::string logPoolProbe();

    const Config* config_{nullptr};
    OdbcInterface odbc_;
    std::atomic<bool> exiting_{false};
    std::atomic<bool> forceSaveAll_{false};
    std::atomic<bool> saveLogoutEnabled_{false};
    std::atomic<bool> pendingExitAfterSave_{false};
    std::atomic<bool> pendingCleanBattleGuild_{false};
    std::atomic<bool> pendingCrashSave_{false};
    std::atomic<bool> pendingPoolProbe_{false};
    std::atomic<bool> pendingProbeOnce_{false};
    std::atomic<bool> pendingProbeResult_{false};
    std::atomic<bool> pendingDispatchSummary_{false};
    std::atomic<bool> pendingDispatchSummaryReset_{false};
    std::atomic<bool> pendingDispatchSummaryTotal_{false};
    std::atomic<bool> pendingDispatchSummaryPeek_{false};
    std::vector<int> e2StrideStates_;
    std::vector<ShareMemType> runtimeProbePoolTypes_;
    std::vector<std::string> runtimeInvalidProbeTokens_;
    bool hasRuntimeProbePoolTypes_{false};
    std::vector<ShareMemType> runtimeSummaryTypes_;
    std::vector<std::string> runtimeInvalidSummaryTokens_;
    bool hasRuntimeSummaryTypes_{false};
    /// B3: Declare **before** `managers_` so on process exit, **managers are destroyed first**
    /// (`shmdt` happens after SMU teardown). Linux SysV SHM (see `ShareMemAPI.cpp` __LINUX__).
    std::vector<std::unique_ptr<ShareMemLinux>> shmRegions_;
    std::vector<std::pair<ShareMemType, std::unique_ptr<ISmuLogicManager>>> managers_;
    std::vector<std::uint32_t> managerIntervalsMs_;
    std::vector<std::uint32_t> managerElapsedMs_;
    std::vector<std::uint64_t> managerDispatchCalls_;
    std::vector<std::uint64_t> managerDispatchErrors_;
    std::vector<std::uint64_t> managerDispatchCallsTotal_;
    std::vector<std::uint64_t> managerDispatchErrorsTotal_;
    std::uint32_t dispatchSummaryElapsedSec_{0};
    std::string lastPoolProbeVerdict_{"NA"};
    std::uint32_t lastPoolProbeTotal_{0};
    std::uint32_t lastPoolProbeWarn_{0};
    std::uint32_t lastPoolProbeFail_{0};
    std::uint32_t lastPoolProbeInvalid_{0};
};
