#pragma once

#include "types.hpp"

#include <string>
#include <unordered_map>
#include <vector>

struct DbConfig {
    std::string dsn;
    std::string user;
    std::string password;
};

struct ServiceConfig {
    std::vector<ShareMemData> shareMemObjects;
    std::unordered_map<std::uint64_t, int> shareMemKeyToServerId;
    DbConfig db;
    std::string sqlDir;
    int currentServerId{0};
    /// From Config/WorldInfo.ini (世界 / 区服 ID), used in Config::WorldID / Config::ZoneID compare log.
    int worldId{0};
    int zoneId{0};
    /// When currentServerId is still unset, set it from this (does not override zoneId / WorldInfo ZoneID for DB compare).
    int fallbackServerId{0};
    /// D3: if true, startup fails when Config::WorldID/ZoneID differs from DB values (legacy default: loose / OK).
    bool strictWorldZoneCheck{false};
    /// E2: if true, startup fails when pilot stride check mismatches (GlobalData/ItemSerial).
    bool strictStrideProbeCheck{false};
    /// E5: optional POOL-PROBE type filter; empty means all SMU types.
    std::vector<ShareMemType> probePoolTypes;
    /// C4: rounds for Human normal-save stable log output (default 6 ~= 30s).
    std::uint32_t humanNormalSaveLogEvery{6};
    /// C4: consecutive Human save failures to escalate warning (default 3).
    std::uint32_t humanSaveFailureWarnThreshold{3};
    /// C4.7: consecutive failures to escalate from warn to error (default 6).
    std::uint32_t humanSaveFailureErrorThreshold{6};
    /// C4.7: emit escalated persistent-failure log every N new failures (default 3).
    std::uint32_t humanSaveFailureEscalateLogEvery{3};
    /// C4.9: if true, stage window summary logs only when failures exist in window.
    bool humanStageWindowLogAnomalyOnly{true};
    /// D-stage: periodic SMU dispatch summary interval in seconds (0 disables, default 60).
    std::uint32_t smuDispatchSummaryIntervalSec{60};
    /// D6: if true, per-SMU summary lines are printed only for entries with errors>0.
    bool smuDispatchSummaryAnomalyOnly{false};
    /// If true, startup fails when currentServerId<=0 (prevents World/GW serverid=0 mismatch).
    bool strictServerIdCheck{true};
    /// B4: If true, call `shmctl(IPC_RMID)` on each segment during process exit.
    bool removeShareMemOnExit{false};
    bool enableShareMem{true};
};

class Config {
public:
    bool load(const std::string& filePath);
    const ServiceConfig& data() const { return data_; }

private:
    ServiceConfig data_;
};
