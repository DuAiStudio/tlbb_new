#include "share_memory.hpp"
#include "db_access.hpp"
#include "logger.hpp"
#include "share_mem_sizes.hpp"

#include <algorithm>
#include <chrono>
#include <array>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <memory>
#include <sstream>
#include <thread>

namespace fs = std::filesystem;

std::string legacyTypeToken(ShareMemType type) {
    switch (type) {
        case ShareMemType::Human: return "8HumanSMU";
        case ShareMemType::Guild: return "8GuildSMU";
        case ShareMemType::Mail: return "7MailSMU";
        case ShareMemType::PlayerShop: return "N11PLAYER_SHOP12PlayerShopSME";
        case ShareMemType::GlobalData: return "13GlobalDataSMU";
        case ShareMemType::CommissionShop: return "16CommisionShopSMU";
        case ShareMemType::ItemSerial: return "16ItemSerialKeySMU";
        case ShareMemType::PetProcreate: return "18PetProcreateItemSM";
        case ShareMemType::City: return "7CitySMU";
        case ShareMemType::GuildLeague: return "14GuildLeagueSMU";
        case ShareMemType::Auction: return "10AuctionSMU";
        case ShareMemType::TopList: return "10TopListSMU";
        default: return "UnknownSMU";
    }
}

std::uint32_t idaSlotStrideBytes(ShareMemType type) {
    // E1 readonly probe: stride values from IDA/decompile (SMUPool Init loops).
    switch (type) {
        case ShareMemType::Human: return 191489;
        case ShareMemType::Guild: return 73850;
        case ShareMemType::Mail: return 377;
        case ShareMemType::PlayerShop: return 522528;
        case ShareMemType::GlobalData: return 21;
        case ShareMemType::CommissionShop: return 6981;
        case ShareMemType::ItemSerial: return 21;
        case ShareMemType::PetProcreate: return 4580;
        case ShareMemType::City: return 38353;
        case ShareMemType::GuildLeague: return 369;
        case ShareMemType::Auction: return 2482;
        case ShareMemType::TopList: return 13697;
        default: return 0;
    }
}

std::string hexAddr(std::uintptr_t v) {
    std::ostringstream oss;
    oss << "0x" << std::hex << std::uppercase << v;
    return oss.str();
}

bool isE2PilotType(ShareMemType type) {
    return type == ShareMemType::GlobalData || type == ShareMemType::ItemSerial;
}

enum class E2StrideState : int {
    NotApplicable = 0,
    Ok = 1,
    Warn = 2,
    Fail = 3
};

const char* e2StrideStateText(E2StrideState s) {
    switch (s) {
        case E2StrideState::Ok: return "OK";
        case E2StrideState::Warn: return "WARN";
        case E2StrideState::Fail: return "FAIL";
        default: return "NA";
    }
}

std::string probeTypesText(const ServiceConfig& cfg) {
    if (cfg.probePoolTypes.empty()) return "ALL";
    std::string out;
    for (std::size_t i = 0; i < cfg.probePoolTypes.size(); ++i) {
        if (!out.empty()) out += ",";
        out += legacyTypeToken(cfg.probePoolTypes[i]);
    }
    return out;
}

std::string probeTypesText(const std::vector<ShareMemType>& types) {
    if (types.empty()) return "ALL";
    std::string out;
    for (std::size_t i = 0; i < types.size(); ++i) {
        if (!out.empty()) out += ",";
        out += legacyTypeToken(types[i]);
    }
    return out;
}

std::string trimWs(const std::string& s) {
    const auto left = s.find_first_not_of(" \t\r\n");
    if (left == std::string::npos) return {};
    const auto right = s.find_last_not_of(" \t\r\n");
    return s.substr(left, right - left + 1);
}

std::string toLowerCopyLocal(std::string v) {
    for (auto& ch : v) {
        if (ch >= 'A' && ch <= 'Z') ch = static_cast<char>(ch - 'A' + 'a');
    }
    return v;
}

std::vector<std::string> splitCsvLowerLocal(const std::string& raw) {
    std::vector<std::string> out;
    std::string cur;
    for (char ch : raw) {
        if (ch == ',') {
            const auto t = toLowerCopyLocal(trimWs(cur));
            if (!t.empty()) out.push_back(t);
            cur.clear();
        } else {
            cur.push_back(ch);
        }
    }
    const auto t = toLowerCopyLocal(trimWs(cur));
    if (!t.empty()) out.push_back(t);
    return out;
}

bool parseProbeTypeToken(const std::string& token, ShareMemType& out) {
    if (token == "human" || token == "humansmu") out = ShareMemType::Human;
    else if (token == "guild" || token == "guildsmu") out = ShareMemType::Guild;
    else if (token == "mail" || token == "mailsmu") out = ShareMemType::Mail;
    else if (token == "playershop" || token == "playershopsm") out = ShareMemType::PlayerShop;
    else if (token == "globaldata" || token == "globaldatasmu") out = ShareMemType::GlobalData;
    else if (token == "commissionshop" || token == "commisionshop" || token == "commisionshopsmu") out = ShareMemType::CommissionShop;
    else if (token == "itemserial" || token == "itemserialkeysmu") out = ShareMemType::ItemSerial;
    else if (token == "petprocreate" || token == "petprocreateitemsm") out = ShareMemType::PetProcreate;
    else if (token == "city" || token == "citysmu") out = ShareMemType::City;
    else if (token == "guildleague" || token == "guildleaguesmu") out = ShareMemType::GuildLeague;
    else if (token == "auction" || token == "auctionsmu") out = ShareMemType::Auction;
    else if (token == "toplist" || token == "toplistsmu") out = ShareMemType::TopList;
    else return false;
    return true;
}

std::string joinTokens(const std::vector<std::string>& tokens) {
    std::string out;
    for (std::size_t i = 0; i < tokens.size(); ++i) {
        if (!out.empty()) out += ",";
        out += tokens[i];
    }
    return out;
}

bool parseProbeTypesFile(
    const std::string& path,
    std::vector<ShareMemType>& out,
    std::vector<std::string>& invalidTokens
) {
    std::ifstream in(path);
    if (!in.is_open()) return false;
    std::string content;
    std::string line;
    while (std::getline(in, line)) {
        const auto t = trimWs(line);
        if (t.empty() || t.starts_with("#") || t.starts_with(";")) continue;
        if (!content.empty()) content += ",";
        content += t;
    }
    out.clear();
    invalidTokens.clear();
    for (const auto& token : splitCsvLowerLocal(content)) {
        if (token == "all") {
            out.clear();
            invalidTokens.clear();
            return true;
        }
        ShareMemType parsed = ShareMemType::Invalid;
        if (parseProbeTypeToken(token, parsed)) {
            out.push_back(parsed);
        } else {
            invalidTokens.push_back(token);
        }
    }
    return true;
}

bool validatePilotStrideCheck(const ShareMemData& smu, const ShareMemLinux& shm, bool strictMode, E2StrideState& outState) {
    outState = E2StrideState::NotApplicable;
    if (!isE2PilotType(smu.type) || shm.headerPtr() == nullptr) {
        return true;
    }
    const auto stride = static_cast<std::uint64_t>(idaSlotStrideBytes(smu.type));
    if (stride == 0) {
        return true;
    }
    const auto expected = static_cast<std::uint64_t>(sizeof(SMHead)) +
                          stride * static_cast<std::uint64_t>(smu.poolSize);
    const auto* head = static_cast<const SMHead*>(shm.headerPtr());
    const auto headSize = head->m_Size;
    if (headSize == expected) {
        outState = E2StrideState::Ok;
        Logger::instance().logShare(
            "[E2-STRIDE] " + smu.name + " key=" + std::to_string(smu.key) +
            " check OK head.size=" + std::to_string(headSize) + " expected=" + std::to_string(expected));
        return true;
    }
    outState = strictMode ? E2StrideState::Fail : E2StrideState::Warn;
    Logger::instance().logShare(
        "[E2-STRIDE] " + smu.name + " key=" + std::to_string(smu.key) +
        " check mismatch head.size=" + std::to_string(headSize) +
        " expected=" + std::to_string(expected) +
        (strictMode ? " (strict fail)" : " (warn only)"));
    return !strictMode;
}

class GenericSmuLogicManager final : public ISmuLogicManager {
public:
    bool init(const ShareMemData& data) override {
        data_ = data;
        return true;
    }

    void doDefault() override {}

    void doNormalSave() override {}

    void doSaveAll() override {}

    void heartbeat(bool forceSave, bool /*saveLogoutMode*/, bool crashSave) override {
        if (crashSave) {
            Logger::instance().logShare("Receive Server Crash command..");
            doSaveAll();
            return;
        }
        if (forceSave) {
            doSaveAll();
            return;
        }
        doDefault();
        doNormalSave();
    }


private:
    ShareMemData data_;
};

class HumanSmuLogicManager final : public ISmuLogicManager {
public:
    enum class SaveStage : std::uint32_t {
        Normal = 0,
        Logout = 1,
        SaveAll = 2,
        Count = 3
    };

    HumanSmuLogicManager(
        OdbcInterface& db,
        std::uint32_t logEveryRounds,
        std::uint32_t warnThreshold,
        std::uint32_t errorThreshold,
        std::uint32_t escalateLogEvery,
        bool stageWindowLogAnomalyOnly)
        : repo_(db),
          normalSaveLogEvery_(logEveryRounds == 0 ? 6 : logEveryRounds),
          saveFailureWarnThreshold_(warnThreshold == 0 ? 3 : warnThreshold),
          saveFailureErrorThreshold_(errorThreshold == 0 ? 6 : errorThreshold),
          saveFailureEscalateLogEvery_(escalateLogEvery == 0 ? 3 : escalateLogEvery),
          stageWindowLogAnomalyOnly_(stageWindowLogAnomalyOnly) {
        if (saveFailureErrorThreshold_ < saveFailureWarnThreshold_) {
            saveFailureErrorThreshold_ = saveFailureWarnThreshold_;
        }
    }

    bool init(const ShareMemData& data) override {
        data_ = data;
        if (!repo_.loadCount(charCount_)) {
            return false;
        }
        return true;
    }

    void doDefault() override {}

    void doNormalSave() override {
        long long affectedMain = 0;
        long long affectedExtra = 0;
        if (!repo_.saveNormalTransactional(affectedMain, affectedExtra)) {
            Logger::instance().logShare("Human NormalSave...Get Errors!");
            noteSaveFailure(SaveStage::Normal);
            return;
        }
        noteSaveSuccess(SaveStage::Normal);
        ++normalSaveRounds_;
        const long long total = affectedMain + affectedExtra;
        const bool changed = (affectedMain != lastNormalMainRows_) || (affectedExtra != lastNormalExtraRows_);
        if (total > 0 && (changed || (normalSaveRounds_ % normalSaveLogEvery_ == 0))) {
            Logger::instance().logShare(
                "Human NormalSave...OK! Rows=" + std::to_string(total) +
                " (char=" + std::to_string(affectedMain) +
                ", extra=" + std::to_string(affectedExtra) + ")");
        }
        lastNormalMainRows_ = affectedMain;
        lastNormalExtraRows_ = affectedExtra;
    }

    void doSaveAll() override {
        persistHuman(true);
    }

    void persistHuman(bool logOkLines) {
        long long affectedMain = 0;
        long long affectedExtra = 0;
        if (!repo_.saveAllTransactional(affectedMain, affectedExtra)) {
            Logger::instance().logShare("End HumanSMU_" + std::to_string(data_.key) + " SaveAll...Get Errors!");
            noteSaveFailure(SaveStage::SaveAll);
            return;
        }
        noteSaveSuccess(SaveStage::SaveAll);
        if (logOkLines) {
            Logger::instance().logShare(
                "End HumanSMU_" + std::to_string(data_.key) + " SaveAll...OK! Rows=" +
                std::to_string(affectedMain + affectedExtra) + " (char=" + std::to_string(affectedMain) +
                ", extra=" + std::to_string(affectedExtra) + ")");
            Logger::instance().logShare("SMUCount = 0");
            Logger::instance().logShare("TotalSMUSize = 0");
            if (affectedMain + affectedExtra == 0) {
                Logger::instance().logShare("[DB] Human Save touched 0 rows (check online char pool/state)");
            }
        }
    }

    void heartbeat(bool forceSave, bool saveLogoutMode, bool crashSave) override {
        if (crashSave) {
            Logger::instance().logShare("Receive Server Crash command..");
            doSaveAll();
            return;
        }
        if (forceSave) {
            doSaveAll();
            return;
        }
        doDefault();
        doNormalSave();
        if (saveLogoutMode) {
            long long affectedMain = 0;
            long long affectedExtra = 0;
            if (!repo_.saveLogoutFocusedTransactional(affectedMain, affectedExtra)) {
                Logger::instance().logShare("Human SaveLogout...Get Errors!");
                noteSaveFailure(SaveStage::Logout);
            } else if (affectedMain + affectedExtra > 0) {
                noteSaveSuccess(SaveStage::Logout);
                Logger::instance().logShare(
                    "Human SaveLogout...OK! Rows=" + std::to_string(affectedMain + affectedExtra) +
                    " (char=" + std::to_string(affectedMain) +
                    ", extra=" + std::to_string(affectedExtra) + ")");
            } else {
                noteSaveSuccess(SaveStage::Logout);
            }
        }
    }


private:
    struct StageCounters {
        std::uint32_t consecutiveFailures{0};
        std::uint32_t windowFailures{0};
        std::uint32_t windowSuccess{0};
        std::uint32_t totalFailures{0};
        std::uint32_t totalSuccess{0};
        std::uint32_t lastEscalatedAt{0};
    };

    static constexpr std::size_t kStageCount = static_cast<std::size_t>(SaveStage::Count);

    static const char* stageName(SaveStage stage) {
        switch (stage) {
            case SaveStage::Normal: return "NormalSave";
            case SaveStage::Logout: return "SaveLogout";
            case SaveStage::SaveAll: return "SaveAll";
            default: return "Unknown";
        }
    }

    StageCounters& stageCounters(SaveStage stage) {
        return stageCounters_[static_cast<std::size_t>(stage)];
    }

    void logStageWindowSummary(SaveStage stage, StageCounters& counters, const char* reasonTag) {
        Logger::instance().logShare(
            std::string("[DB] Human stage window summary (") + reasonTag +
            ") stage=" + stageName(stage) +
            " window_fail=" + std::to_string(counters.windowFailures) +
            " window_ok=" + std::to_string(counters.windowSuccess) +
            " total_fail=" + std::to_string(counters.totalFailures) +
            " total_ok=" + std::to_string(counters.totalSuccess) +
            " stage_consecutive=" + std::to_string(counters.consecutiveFailures));
        counters.windowFailures = 0;
        counters.windowSuccess = 0;
    }

    void noteSaveFailure(SaveStage stage) {
        auto& perStage = stageCounters(stage);
        ++perStage.consecutiveFailures;
        ++perStage.windowFailures;
        ++perStage.totalFailures;
        ++consecutiveSaveFailures_;
        Logger::instance().logShare(
            std::string("[DB] Human ") + stageName(stage) + " failed (consecutive=" +
            std::to_string(consecutiveSaveFailures_) + ")");
        if (consecutiveSaveFailures_ >= saveFailureWarnThreshold_ &&
            (perStage.lastEscalatedAt == 0 ||
             perStage.consecutiveFailures >= perStage.lastEscalatedAt + saveFailureEscalateLogEvery_)) {
            const bool isErrorLevel = consecutiveSaveFailures_ >= saveFailureErrorThreshold_;
            Logger::instance().logShare(
                std::string(isErrorLevel ? "[DB][ERROR] " : "[DB][WARN] ") +
                "Human persistent save failure, check DB/locks (stage=" + stageName(stage) +
                ", global_consecutive=" + std::to_string(consecutiveSaveFailures_) +
                ", stage_consecutive=" + std::to_string(perStage.consecutiveFailures) + ")");
            perStage.lastEscalatedAt = perStage.consecutiveFailures;
        }
        if (perStage.windowFailures >= saveFailureEscalateLogEvery_) {
            logStageWindowSummary(stage, perStage, "fail-window");
        }
    }

    void noteSaveSuccess(SaveStage stage) {
        auto& perStage = stageCounters(stage);
        ++perStage.totalSuccess;
        ++perStage.windowSuccess;
        if (perStage.consecutiveFailures >= saveFailureWarnThreshold_) {
            Logger::instance().logShare(
                std::string("[DB] Human ") + stageName(stage) +
                " recovered after " + std::to_string(perStage.consecutiveFailures) + " failures");
        }
        const bool shouldLogSuccessWindow = stageWindowLogAnomalyOnly_
            ? (perStage.windowFailures > 0)
            : (perStage.windowFailures > 0 || perStage.windowSuccess >= saveFailureEscalateLogEvery_);
        if (shouldLogSuccessWindow) {
            logStageWindowSummary(stage, perStage, "success-window");
        }
        perStage.consecutiveFailures = 0;
        perStage.lastEscalatedAt = 0;
        if (consecutiveSaveFailures_ >= saveFailureWarnThreshold_) {
            Logger::instance().logShare(
                "[DB] Human save path recovered after " + std::to_string(consecutiveSaveFailures_) + " failures");
        }
        consecutiveSaveFailures_ = 0;
    }

    HumanDbRepo repo_;
    ShareMemData data_;
    long long charCount_{0};
    std::uint32_t normalSaveRounds_{0};
    long long lastNormalMainRows_{-1};
    long long lastNormalExtraRows_{-1};
    std::uint32_t consecutiveSaveFailures_{0};
    std::uint32_t normalSaveLogEvery_{6};
    std::uint32_t saveFailureWarnThreshold_{3};
    std::uint32_t saveFailureErrorThreshold_{6};
    std::uint32_t saveFailureEscalateLogEvery_{3};
    bool stageWindowLogAnomalyOnly_{true};
    std::array<StageCounters, kStageCount> stageCounters_{};
};

class GuildSmuLogicManager final : public ISmuLogicManager {
public:
    explicit GuildSmuLogicManager(OdbcInterface& db) : repo_(db) {}

    bool init(const ShareMemData& data) override {
        data_ = data;
        if (!repo_.loadCount(guildCount_)) {
            return false;
        }
        return true;
    }

    void doDefault() override {}

    void doNormalSave() override {
        // IDA: Guild normal path is time-gated and may call DoSaveAll.
        constexpr std::uint32_t kHeartbeatMs = 5000;
        normalElapsedMs_ += kHeartbeatMs;
        const auto intervalMs = data_.intervalMs == 0 ? kHeartbeatMs : data_.intervalMs;
        if (normalElapsedMs_ < intervalMs) {
            return;
        }
        normalElapsedMs_ = 0;
        doSaveAll();
    }

    void doSaveAll() override {
        long long affectedGuild = 0;
        long long affectedUser = 0;
        if (!repo_.saveAllTransactional(affectedGuild, affectedUser)) {
            Logger::instance().logShare("End GuildSMU_" + std::to_string(data_.key) + " SaveAll...Get Errors!");
            return;
        }
        Logger::instance().logShare(
            "End GuildSMU_" + std::to_string(data_.key) + " SaveAll...OK! Rows=" +
            std::to_string(affectedGuild + affectedUser) + " (guild=" + std::to_string(affectedGuild) +
            ", user=" + std::to_string(affectedUser) + ")");
        if (affectedGuild + affectedUser == 0) {
            Logger::instance().logShare("[DB] Guild Save touched 0 rows (check pool/state)");
        }
        Logger::instance().logShare("SMUCount = 0");
        Logger::instance().logShare("TotalSMUSize = 0");
        (void)repo_.markGeneralSetDirty();
    }

    void heartbeat(bool forceSave, bool /*saveLogoutMode*/, bool crashSave) override {
        if (crashSave) {
            Logger::instance().logShare("Receive Server Crash command..");
            doSaveAll();
            return;
        }
        if (forceSave) {
            doSaveAll();
            return;
        }
        doDefault();
        doNormalSave();
    }


private:
    GuildDbRepo repo_;
    ShareMemData data_;
    long long guildCount_{0};
    std::uint32_t normalElapsedMs_{0};
};

class MailSmuLogicManager final : public ISmuLogicManager {
public:
    explicit MailSmuLogicManager(OdbcInterface& db) : repo_(db) {}

    bool init(const ShareMemData& data) override {
        data_ = data;
        if (!repo_.loadCount(mailCount_)) {
            return false;
        }
        return true;
    }

    void doDefault() override {}

    void doNormalSave() override {
        // IDA: Mail normal path is time-gated and may call DoSaveAll.
        constexpr std::uint32_t kHeartbeatMs = 5000;
        normalElapsedMs_ += kHeartbeatMs;
        const auto intervalMs = data_.intervalMs == 0 ? kHeartbeatMs : data_.intervalMs;
        if (normalElapsedMs_ < intervalMs) {
            return;
        }
        normalElapsedMs_ = 0;
        doSaveAll();
    }

    void doSaveAll() override {
        long long affected = 0;
        if (!repo_.saveAllWithRows(affected)) {
            Logger::instance().logShare("End MailSMU_" + std::to_string(data_.key) + " SaveAll...Get Errors!");
            return;
        }
        Logger::instance().logShare(
            "End MailSMU_" + std::to_string(data_.key) + " SaveAll...OK! Rows=" + std::to_string(affected));
        if (affected == 0) {
            Logger::instance().logShare("[DB] Mail Save touched 0 rows (check pool/state)");
        }
        Logger::instance().logShare("SMUCount = 0");
        Logger::instance().logShare("TotalSMUSize = 0");
    }

    void heartbeat(bool forceSave, bool /*saveLogoutMode*/, bool crashSave) override {
        if (crashSave) {
            Logger::instance().logShare("Receive Server Crash command..");
            doSaveAll();
            return;
        }
        if (forceSave) {
            doSaveAll();
            return;
        }
        doDefault();
        doNormalSave();
    }


private:
    MailDbRepo repo_;
    ShareMemData data_;
    long long mailCount_{0};
    std::uint32_t normalElapsedMs_{0};
};

class ItemSerialSmuLogicManager final : public ISmuLogicManager {
public:
    ItemSerialSmuLogicManager(OdbcInterface& db, int serverId) : repo_(db), serverId_(serverId) {}

    bool init(const ShareMemData& data) override {
        data_ = data;
        if (!repo_.loadCount(rowCount_)) {
            return false;
        }
        long long createdRows = 0;
        if (!repo_.ensureServerRow(serverId_, static_cast<long long>(data_.key), createdRows)) {
            return false;
        }
        if (createdRows > 0) {
            Logger::instance().logShare(
                "ItemSerial Init...Create sid row OK! ServerID = " + std::to_string(serverId_) +
                " ,SMKey = " + std::to_string(data_.key));
        }
        if (!repo_.loadMaxSerial(maxSerial_, serverId_)) {
            return false;
        }
        return true;
    }

    long long loadedMaxSerial() const { return maxSerial_; }

    void doDefault() override {}

    void doNormalSave() override {
        // IDA: ItemSerial normal save uses fixed 30s gate.
        constexpr std::uint32_t kHeartbeatMs = 5000;
        constexpr std::uint32_t kNormalGateMs = 30000;
        normalElapsedMs_ += kHeartbeatMs;
        if (normalElapsedMs_ < kNormalGateMs) {
            return;
        }
        normalElapsedMs_ = 0;
        doSaveAll();
    }

    void doSaveAll() override {
        long long affected = 0;
        if (!repo_.saveAllWithRows(affected, serverId_)) {
            Logger::instance().logShare(
                "ItemSerial Save...Error! Serial = " + std::to_string(maxSerial_) +
                " ,ServerID = " + std::to_string(serverId_));
            return;
        }
        if (affected == 0) {
            long long createdRows = 0;
            if (repo_.ensureServerRow(serverId_, static_cast<long long>(data_.key), createdRows) && createdRows > 0) {
                (void)repo_.saveAllWithRows(affected, serverId_);
            }
        }
        long long serial = 0;
        if (!repo_.loadMaxSerial(serial, serverId_)) {
            serial = maxSerial_;
        } else {
            maxSerial_ = serial;
        }
        Logger::instance().logShare(
            "ItemSerial Save...OK! Serial = " + std::to_string(serial) +
            " ,ServerID = " + std::to_string(serverId_) + " ,Rows=" + std::to_string(affected));
        if (affected == 0) {
            Logger::instance().logShare("[DB] ItemSerial Save touched 0 rows (sid row missing or not writable)");
        }
    }

    void heartbeat(bool forceSave, bool /*saveLogoutMode*/, bool crashSave) override {
        if (crashSave) {
            Logger::instance().logShare("Receive Server Crash command..");
            doSaveAll();
            return;
        }
        if (forceSave) {
            doSaveAll();
            return;
        }
        doDefault();
        doNormalSave();
    }


private:
    ItemSerialDbRepo repo_;
    ShareMemData data_;
    long long rowCount_{0};
    long long maxSerial_{0};
    std::uint32_t normalElapsedMs_{0};
    int serverId_{0};
};

class CitySmuLogicManager final : public ISmuLogicManager {
public:
    explicit CitySmuLogicManager(OdbcInterface& db) : repo_(db) {}

    bool init(const ShareMemData& data) override {
        data_ = data;
        if (!repo_.loadCount(cityCount_)) {
            return false;
        }
        return true;
    }

    void doDefault() override {}

    void doNormalSave() override {
        // IDA: City normal path is interval-gated and calls DoSaveAll.
        constexpr std::uint32_t kHeartbeatMs = 5000;
        normalElapsedMs_ += kHeartbeatMs;
        const auto intervalMs = data_.intervalMs == 0 ? kHeartbeatMs : data_.intervalMs;
        if (normalElapsedMs_ < intervalMs) {
            return;
        }
        normalElapsedMs_ = 0;
        doSaveAll();
    }

    void doSaveAll() override {
        long long affected = 0;
        if (!repo_.saveAllTransactional(affected)) {
            lastSaveAllOk_ = false;
            Logger::instance().logShare("End CityInfoSM_" + std::to_string(data_.key) + " SaveAll...Get Errors!");
            return;
        }
        lastSaveAllOk_ = true;
        Logger::instance().logShare(
            "End CityInfoSM_" + std::to_string(data_.key) + " SaveAll...OK! Rows=" + std::to_string(affected));
        if (affected == 0) {
            Logger::instance().logShare("[DB] City Save touched 0 rows (check pool/state)");
        }
        Logger::instance().logShare("SMUCount = 0");
        Logger::instance().logShare("TotalSMUSize = 0");
        (void)repo_.markGeneralSetDirty();
    }

    bool lastSaveAllOk() const { return lastSaveAllOk_; }

    void heartbeat(bool forceSave, bool /*saveLogoutMode*/, bool crashSave) override {
        if (crashSave) {
            Logger::instance().logShare("Receive Server Crash command..");
            doSaveAll();
            return;
        }
        if (forceSave) {
            doSaveAll();
            return;
        }
        doDefault();
        doNormalSave();
    }


private:
    CityDbRepo repo_;
    ShareMemData data_;
    long long cityCount_{0};
    std::uint32_t normalElapsedMs_{0};
    bool lastSaveAllOk_{true};
};

class GlobalDataSmuLogicManager final : public ISmuLogicManager {
public:
    explicit GlobalDataSmuLogicManager(OdbcInterface& db) : repo_(db) {}

    bool init(const ShareMemData& data) override {
        data_ = data;
        if (!repo_.loadCount(globalCount_)) {
            return false;
        }
        return true;
    }

    void doDefault() override {}

    void doNormalSave() override {
        // IDA: GlobalData normal path uses fixed 30s gate.
        constexpr std::uint32_t kHeartbeatMs = 5000;
        constexpr std::uint32_t kNormalGateMs = 30000;
        normalElapsedMs_ += kHeartbeatMs;
        if (normalElapsedMs_ < kNormalGateMs) {
            return;
        }
        normalElapsedMs_ = 0;
        doSaveAll();
    }

    void doSaveAll() override {
        long long affected = 0;
        if (!repo_.saveAllTransactional(affected)) {
            Logger::instance().logShare("GlobalData Save...Error!");
            return;
        }
        Logger::instance().logShare("GlobalData Save...OK! Rows=" + std::to_string(affected));
        if (affected == 0) {
            Logger::instance().logShare("[DB] GlobalData Save touched 0 rows (check pool/state)");
        }
    }

    void heartbeat(bool forceSave, bool /*saveLogoutMode*/, bool crashSave) override {
        if (crashSave) {
            Logger::instance().logShare("Receive Server Crash command..");
            doSaveAll();
            return;
        }
        if (forceSave) {
            doSaveAll();
            return;
        }
        doDefault();
        doNormalSave();
    }


private:
    GlobalDataDbRepo repo_;
    ShareMemData data_;
    long long globalCount_{0};
    std::uint32_t normalElapsedMs_{0};
};

class AuctionSmuLogicManager final : public ISmuLogicManager {
public:
    explicit AuctionSmuLogicManager(OdbcInterface& db) : repo_(db) {}

    bool init(const ShareMemData& data) override {
        data_ = data;
        if (!repo_.loadCounts(auctionCount_, itemCount_, petCount_)) return false;
        return true;
    }

    void doDefault() override {}

    void doNormalSave() override {
        // IDA: Auction normal path uses fixed 30s gate.
        constexpr std::uint32_t kHeartbeatMs = 5000;
        constexpr std::uint32_t kNormalGateMs = 30000;
        normalElapsedMs_ += kHeartbeatMs;
        if (normalElapsedMs_ < kNormalGateMs) {
            return;
        }
        normalElapsedMs_ = 0;
        doSaveAll();
    }

    void doSaveAll() override {
        long long affectedAuction = 0;
        long long affectedItem = 0;
        long long affectedPet = 0;
        if (!repo_.saveAllTransactional(affectedAuction, affectedItem, affectedPet)) {
            Logger::instance().logShare("CommisionShop Save...Error!");
            return;
        }
        const long long affectedUnits = affectedItem + affectedPet;
        Logger::instance().logShare(
            "Auction Save...OK! Rows=" + std::to_string(affectedAuction + affectedUnits) +
            " (auction=" + std::to_string(affectedAuction) +
            ", item=" + std::to_string(affectedItem) +
            ", pet=" + std::to_string(affectedPet) + ")");
        if (affectedAuction + affectedUnits == 0) {
            Logger::instance().logShare("[DB] Auction Save touched 0 rows (check pool/state)");
        }
    }

    void heartbeat(bool forceSave, bool /*saveLogoutMode*/, bool crashSave) override {
        if (crashSave) {
            Logger::instance().logShare("Receive Server Crash command..");
            doSaveAll();
            return;
        }
        if (forceSave) {
            doSaveAll();
            return;
        }
        doDefault();
        doNormalSave();
    }


private:
    AuctionDbRepo repo_;
    ShareMemData data_;
    long long auctionCount_{0};
    long long itemCount_{0};
    long long petCount_{0};
    std::uint32_t normalElapsedMs_{0};
};

class TopListSmuLogicManager final : public ISmuLogicManager {
public:
    explicit TopListSmuLogicManager(OdbcInterface& db) : repo_(db) {}

    bool init(const ShareMemData& data) override {
        data_ = data;
        if (!repo_.loadCount(rowCount_)) {
            return false;
        }
        return true;
    }

    void doDefault() override {}

    void doNormalSave() override {
        // IDA: TopList normal path is interval-gated and calls DoSaveAll.
        constexpr std::uint32_t kHeartbeatMs = 5000;
        normalElapsedMs_ += kHeartbeatMs;
        const auto intervalMs = data_.intervalMs == 0 ? kHeartbeatMs : data_.intervalMs;
        if (normalElapsedMs_ < intervalMs) {
            return;
        }
        normalElapsedMs_ = 0;
        doSaveAll();
    }

    void doSaveAll() override {
        long long affectedAid = 0;
        long long affectedType = 0;
        if (!repo_.saveAllTransactional(affectedAid, affectedType)) {
            Logger::instance().logShare("TopList[ID:0] Save...Error!");
            Logger::instance().logShare("TopList[ID:1] Save...Error!");
            Logger::instance().logShare("TopList[ID:2] Save...Error!");
            return;
        }
        const long long total = affectedAid + affectedType;
        Logger::instance().logShare("TopList[ID:0] Save...OK! Rows=" + std::to_string(total));
        Logger::instance().logShare("TopList[ID:1] Save...OK! Rows=" + std::to_string(total));
        Logger::instance().logShare("TopList[ID:2] Save...OK! Rows=" + std::to_string(total));
        if (total == 0) {
            Logger::instance().logShare("[DB] TopList Save touched 0 rows (check pool/state)");
        }
    }

    void heartbeat(bool forceSave, bool /*saveLogoutMode*/, bool crashSave) override {
        if (crashSave) {
            Logger::instance().logShare("Receive Server Crash command..");
            doSaveAll();
            return;
        }
        if (forceSave) {
            doSaveAll();
            return;
        }
        doDefault();
        doNormalSave();
    }


private:
    TopListDbRepo repo_;
    ShareMemData data_;
    long long rowCount_{0};
    std::uint32_t normalElapsedMs_{0};
};

class CommissionShopSmuLogicManager final : public ISmuLogicManager {
public:
    explicit CommissionShopSmuLogicManager(OdbcInterface& db) : repo_(db) {}

    bool init(const ShareMemData& data) override {
        data_ = data;
        if (!repo_.loadCounts(shopCount_, itemCount_)) {
            return false;
        }
        return true;
    }

    void doDefault() override {}

    void doNormalSave() override {
        // IDA: CommisionShop normal path uses fixed 30s gate.
        constexpr std::uint32_t kHeartbeatMs = 5000;
        constexpr std::uint32_t kNormalGateMs = 30000;
        normalElapsedMs_ += kHeartbeatMs;
        if (normalElapsedMs_ < kNormalGateMs) {
            return;
        }
        normalElapsedMs_ = 0;
        doSaveAll();
    }

    void doSaveAll() override {
        long long affectedItem = 0;
        long long affectedShop = 0;
        if (!repo_.saveAllTransactional(affectedItem, affectedShop)) {
            Logger::instance().logShare("CommisionShop Save...Error!");
            return;
        }
        const long long affected = affectedItem + affectedShop;
        Logger::instance().logShare(
            "CommisionShop Save...OK! Rows=" + std::to_string(affected) +
            " (item=" + std::to_string(affectedItem) +
            ", shop=" + std::to_string(affectedShop) + ")");
        if (affected == 0) {
            Logger::instance().logShare("[DB] CommisionShop Save touched 0 rows (check pool/state)");
        }
    }

    void heartbeat(bool forceSave, bool /*saveLogoutMode*/, bool crashSave) override {
        if (crashSave) {
            Logger::instance().logShare("Receive Server Crash command..");
            doSaveAll();
            return;
        }
        if (forceSave) {
            doSaveAll();
            return;
        }
        doDefault();
        doNormalSave();
    }


private:
    CommissionShopDbRepo repo_;
    ShareMemData data_;
    long long shopCount_{0};
    long long itemCount_{0};
    std::uint32_t normalElapsedMs_{0};
};

class PlayerShopSmuLogicManager final : public ISmuLogicManager {
public:
    explicit PlayerShopSmuLogicManager(OdbcInterface& db) : repo_(db) {}

    bool init(const ShareMemData& data) override {
        data_ = data;
        if (!repo_.loadCounts(shopCount_, stallCount_)) return false;
        return true;
    }

    void doDefault() override {}

    void doNormalSave() override {
        // IDA: PlayerShop normal path is interval-gated and calls DoSaveAll.
        constexpr std::uint32_t kHeartbeatMs = 5000;
        normalElapsedMs_ += kHeartbeatMs;
        const auto intervalMs = data_.intervalMs == 0 ? kHeartbeatMs : data_.intervalMs;
        if (normalElapsedMs_ < intervalMs) {
            return;
        }
        normalElapsedMs_ = 0;
        doSaveAll();
    }

    void doSaveAll() override {
        long long affectedShop = 0;
        long long affectedStall = 0;
        if (!repo_.saveAllTransactional(affectedShop, affectedStall)) {
            Logger::instance().logShare("End PlayerShopSM_" + std::to_string(data_.key) + " SaveAll...Get Errors!");
            return;
        }
        const long long affected = affectedShop + affectedStall;
        Logger::instance().logShare(
            "End PlayerShopSM_" + std::to_string(data_.key) + " SaveAll...OK! Rows=" + std::to_string(affected) +
            " (shop=" + std::to_string(affectedShop) + ", stall=" + std::to_string(affectedStall) + ")");
        if (affected == 0) {
            Logger::instance().logShare("[DB] PlayerShop Save touched 0 rows (check pool/state)");
        }
        Logger::instance().logShare("SMUCount = 0");
        Logger::instance().logShare("TotalSMUSize = 0");
        (void)repo_.markGeneralSetDirty();
    }

    void heartbeat(bool forceSave, bool /*saveLogoutMode*/, bool crashSave) override {
        if (crashSave) {
            Logger::instance().logShare("Receive Server Crash command..");
            doSaveAll();
            return;
        }
        if (forceSave) {
            doSaveAll();
            return;
        }
        doDefault();
        doNormalSave();
    }


private:
    PlayerShopDbRepo repo_;
    ShareMemData data_;
    long long shopCount_{0};
    long long stallCount_{0};
    std::uint32_t normalElapsedMs_{0};
};

class PetProcreateSmuLogicManager final : public ISmuLogicManager {
public:
    explicit PetProcreateSmuLogicManager(OdbcInterface& db) : repo_(db) {}

    bool init(const ShareMemData& data) override {
        data_ = data;
        if (!repo_.loadCount(rowCount_)) return false;
        return true;
    }

    void doDefault() override {}

    void doNormalSave() override {
        // IDA: PetProcreate normal path is interval-gated and calls DoSaveAll.
        constexpr std::uint32_t kHeartbeatMs = 5000;
        normalElapsedMs_ += kHeartbeatMs;
        const auto intervalMs = data_.intervalMs == 0 ? kHeartbeatMs : data_.intervalMs;
        if (normalElapsedMs_ < intervalMs) {
            return;
        }
        normalElapsedMs_ = 0;
        doSaveAll();
    }

    void doSaveAll() override {
        if (!repo_.saveAll()) {
            Logger::instance().logShare("End PetProcreateItemSM_" + std::to_string(data_.key) + " SaveAll...Get Errors!");
            return;
        }
        Logger::instance().logShare("End PetProcreateItemSM_" + std::to_string(data_.key) + " SaveAll...OK!");
        Logger::instance().logShare("SMUCount = 0");
        Logger::instance().logShare("TotalSMUSize = 0");
    }

    void heartbeat(bool forceSave, bool /*saveLogoutMode*/, bool crashSave) override {
        if (crashSave) {
            Logger::instance().logShare("Receive Server Crash command..");
            doSaveAll();
            return;
        }
        if (forceSave) {
            doSaveAll();
            return;
        }
        doDefault();
        doNormalSave();
    }


private:
    PetProcreateDbRepo repo_;
    ShareMemData data_;
    long long rowCount_{0};
    std::uint32_t normalElapsedMs_{0};
};

class GuildLeagueSmuLogicManager final : public ISmuLogicManager {
public:
    explicit GuildLeagueSmuLogicManager(OdbcInterface& db) : repo_(db) {}

    bool init(const ShareMemData& data) override {
        data_ = data;
        if (!repo_.loadCounts(leagueCount_, userCount_)) return false;
        return true;
    }

    void doDefault() override {}

    void doNormalSave() override {
        // IDA: GuildLeague normal path uses fixed 30s gate.
        constexpr std::uint32_t kHeartbeatMs = 5000;
        constexpr std::uint32_t kNormalGateMs = 30000;
        normalElapsedMs_ += kHeartbeatMs;
        if (normalElapsedMs_ < kNormalGateMs) {
            return;
        }
        normalElapsedMs_ = 0;
        doSaveAll();
    }

    void doSaveAll() override {
        if (!repo_.saveAll()) {
            // Keep IDA quirked error text.
            Logger::instance().logShare("End PetProcreateItemSM_" + std::to_string(data_.key) + " SaveAll...Get Errors!");
            return;
        }
        Logger::instance().logShare("End GuildLeagueSMU_" + std::to_string(data_.key) + " SaveAll...OK!");
        Logger::instance().logShare("SMUCount = 0");
        Logger::instance().logShare("TotalSMUSize = 0");
    }

    void heartbeat(bool forceSave, bool /*saveLogoutMode*/, bool crashSave) override {
        if (crashSave) {
            Logger::instance().logShare("Receive Server Crash command..");
            doSaveAll();
            return;
        }
        if (forceSave) {
            doSaveAll();
            return;
        }
        doDefault();
        doNormalSave();
    }


private:
    GuildLeagueDbRepo repo_;
    ShareMemData data_;
    long long leagueCount_{0};
    long long userCount_{0};
    std::uint32_t normalElapsedMs_{0};
};

namespace {

std::unique_ptr<ISmuLogicManager> makeManagerFor(
    const ServiceConfig& cfg,
    OdbcInterface& odbc,
    const ShareMemData& smu
) {
    switch (smu.type) {
        case ShareMemType::Human:
            return std::make_unique<HumanSmuLogicManager>(
                odbc,
                cfg.humanNormalSaveLogEvery,
                cfg.humanSaveFailureWarnThreshold,
                cfg.humanSaveFailureErrorThreshold,
                cfg.humanSaveFailureEscalateLogEvery,
                cfg.humanStageWindowLogAnomalyOnly);
        case ShareMemType::Guild:
            return std::make_unique<GuildSmuLogicManager>(odbc);
        case ShareMemType::Mail:
            return std::make_unique<MailSmuLogicManager>(odbc);
        case ShareMemType::ItemSerial: {
            int sid = cfg.currentServerId;
            const auto it = cfg.shareMemKeyToServerId.find(smu.key);
            if (it != cfg.shareMemKeyToServerId.end()) {
                sid = it->second;
            }
            return std::make_unique<ItemSerialSmuLogicManager>(odbc, sid);
        }
        case ShareMemType::City:
            return std::make_unique<CitySmuLogicManager>(odbc);
        case ShareMemType::GlobalData:
            return std::make_unique<GlobalDataSmuLogicManager>(odbc);
        case ShareMemType::Auction:
            return std::make_unique<AuctionSmuLogicManager>(odbc);
        case ShareMemType::TopList:
            return std::make_unique<TopListSmuLogicManager>(odbc);
        case ShareMemType::CommissionShop:
            return std::make_unique<CommissionShopSmuLogicManager>(odbc);
        case ShareMemType::PlayerShop:
            return std::make_unique<PlayerShopSmuLogicManager>(odbc);
        case ShareMemType::PetProcreate:
            return std::make_unique<PetProcreateSmuLogicManager>(odbc);
        case ShareMemType::GuildLeague:
            return std::make_unique<GuildLeagueSmuLogicManager>(odbc);
        default:
            return std::make_unique<GenericSmuLogicManager>();
    }
}

void logPostInit(const ShareMemData& smu, ISmuLogicManager* mgr) {
    switch (smu.type) {
        case ShareMemType::Guild:
            Logger::instance().logShare("PostInit GuildSMU_" + std::to_string(smu.key) + " from database  Ok!");
            break;
        case ShareMemType::Mail:
            Logger::instance().logShare("PostInit MailSMU_" + std::to_string(smu.key) + " from database  Ok!");
            break;
        case ShareMemType::PlayerShop:
            Logger::instance().logShare("PostInit PlayerShopSM_" + std::to_string(smu.key) + " from database  Ok!");
            break;
        case ShareMemType::GlobalData:
            Logger::instance().logShare("PostInit GlobalDataSMU=" + std::to_string(smu.key) + " from database Ok!");
            break;
        case ShareMemType::CommissionShop:
            Logger::instance().logShare("PostInit CommisionShopSMU=" + std::to_string(smu.key) + " from database Ok!");
            break;
        case ShareMemType::ItemSerial: {
            const auto* p = dynamic_cast<ItemSerialSmuLogicManager*>(mgr);
            const long long ser = p ? p->loadedMaxSerial() : 0;
            Logger::instance().logShare(
                "PostInit ItemSerialSMU=" + std::to_string(smu.key) + " from database Ok!  Serial = " +
                std::to_string(ser));
            break;
        }
        case ShareMemType::PetProcreate:
            Logger::instance().logShare("PostInit PetProcreateItemSM" + std::to_string(smu.key) + " from database  Ok!");
            break;
        case ShareMemType::City:
            Logger::instance().logShare("PostInit CityInfoSM" + std::to_string(smu.key) + " from database  Ok!");
            break;
        case ShareMemType::GuildLeague:
            Logger::instance().logShare("PostInit GuildLeagueSMU_" + std::to_string(smu.key) + " from database  Ok!");
            break;
        case ShareMemType::Auction:
            Logger::instance().logShare("PostInit AuctionSMU=" + std::to_string(smu.key) + " from database Ok!");
            break;
        case ShareMemType::TopList:
            Logger::instance().logShare("PostInit TopList[Id=0]SMU=" + std::to_string(smu.key) + " from database Ok!");
            Logger::instance().logShare("PostInit TopList[Id=1]SMU=" + std::to_string(smu.key) + " from database Ok!");
            Logger::instance().logShare("PostInit TopList[Id=2]SMU=" + std::to_string(smu.key) + " from database Ok!");
            break;
        default:
            break;
    }
}

}  // namespace

bool ShareMemoryService::init(const Config& config) {
    config_ = &config;
    Logger::instance().logShare("Start New Managers...");
    if (config.data().currentServerId <= 0) {
        if (config.data().strictServerIdCheck) {
            Logger::instance().logShare("CheckCurrentServerID()...Fails");
            return false;
        }
        Logger::instance().logShare("CheckCurrentServerID()...Warn (strict disabled)");
    }
    if (!config.data().enableShareMem) {
        Logger::instance().logShare("EnableShareMem=0, ShareMemory service disabled by ServerInfo.ini");
        return false;
    }
    Logger::instance().logShare("new ShareDBManager ...OK");

    const auto& smus = config.data().shareMemObjects;
    Logger::instance().logShare("Start new SMUObj Count=" + std::to_string(smus.size()));

    managers_.clear();
    managerIntervalsMs_.clear();
    managerElapsedMs_.clear();
    managerDispatchCalls_.clear();
    managerDispatchErrors_.clear();
    managerDispatchCallsTotal_.clear();
    managerDispatchErrorsTotal_.clear();
    dispatchSummaryElapsedSec_ = 0;
    e2StrideStates_.clear();

    for (const auto& smu : smus) {
        auto mgr = makeManagerFor(config.data(), odbc_, smu);
        const auto legacyToken = legacyTypeToken(smu.type);
        Logger::instance().logShare("new SMUPool<" + legacyToken + ">()...OK");
        Logger::instance().logShare("new SMULogicManager<" + legacyToken + ">()...OK");
        managers_.push_back({smu.type, std::move(mgr)});
        managerIntervalsMs_.push_back(smu.intervalMs);
        managerElapsedMs_.push_back(0);
        managerDispatchCalls_.push_back(0);
        managerDispatchErrors_.push_back(0);
        managerDispatchCallsTotal_.push_back(0);
        managerDispatchErrorsTotal_.push_back(0);
    }

    Logger::instance().logShare("New Managers...OK!");
    Logger::instance().logShare("Start Init Managers...");

    if (!odbc_.connect(config.data().db)) {
        Logger::instance().logShare("g_pDBManager->Init()...Fails");
        return false;
    }
    Logger::instance().logShare("g_pDBManager->Init()...OK");
    long long dbWorldId = 0;
    long long dbZoneId = 0;
    (void)odbc_.queryInt64(
        "SELECT nVal FROM t_general_set WHERE sKey='WORLD_ID' LIMIT 1",
        dbWorldId);
    (void)odbc_.queryInt64(
        "SELECT nVal FROM t_general_set WHERE sKey='ZONE_ID' LIMIT 1",
        dbZoneId);
    Logger::instance().logShare(
        "DBGeneralSet::GetValue(4) = " + std::to_string(dbWorldId) + " Successful!");
    Logger::instance().logShare(
        "DBGeneralSet::GetValue(5) = " + std::to_string(dbZoneId) + " Successful!");
    Logger::instance().logShare(
        "Config::WorldID=" + std::to_string(config.data().worldId) +
        ",Config::ZoneID=" + std::to_string(config.data().zoneId) +
        "......DB::WorldID=" + std::to_string(dbWorldId) +
        ",DB::ZoneID=" + std::to_string(dbZoneId) + "...Compare");
    const bool worldZoneMatch =
        (dbWorldId == static_cast<long long>(config.data().worldId)) &&
        (dbZoneId == static_cast<long long>(config.data().zoneId));
    if (!worldZoneMatch && config.data().strictWorldZoneCheck) {
        Logger::instance().logShare("CheckWorldIDZoneID()...Fails");
        return false;
    }
    Logger::instance().logShare("CheckWorldIDZoneID()...OK");

    shmRegions_.clear();
    shmRegions_.reserve(managers_.size());

    for (std::size_t i = 0; i < managers_.size(); ++i) {
        const auto& smu = smus[i];
        auto& mgr = managers_[i].second;
        const std::uint64_t sz = attachSizeBytes(smu.type, smu.poolSize);
        const std::string szStr = std::to_string(sz);

        auto shm = std::make_unique<ShareMemLinux>();
        bool openFail = false;
        bool created = false;
        if (!shm->attachOrCreate(static_cast<std::uint32_t>(smu.key), static_cast<std::size_t>(sz), openFail, created)) {
            Logger::instance().logShare("Map ShareMem Error SM_KET=" + std::to_string(smu.key));
            return false;
        }
#if defined(__linux__)
        if (openFail) {
            Logger::instance().logShare("Attach ShareMem Error SM_KET=" + std::to_string(smu.key));
        }
        if (created) {
            Logger::instance().logShare("Create ShareMem Ok SM_KET = " + std::to_string(smu.key));
        }
#endif
        Logger::instance().logShare("Attach ShareMem... SM_KET=" + std::to_string(smu.key) + " Size=" + szStr + "," + szStr);
        Logger::instance().logShare("Attach ShareMem OK! SM_KET=" + std::to_string(smu.key) + " Size=" + szStr + "," + szStr);

        E2StrideState e2State = E2StrideState::NotApplicable;
        if (!validatePilotStrideCheck(smu, *shm, config.data().strictStrideProbeCheck, e2State)) {
            return false;
        }
        e2StrideStates_.push_back(static_cast<int>(e2State));

        shmRegions_.push_back(std::move(shm));

        if (!mgr->init(smu)) {
            return false;
        }
        logPostInit(smu, mgr.get());
    }

    Logger::instance().logShare("Init Managers...OK!");
    return true;
}

bool ShareMemoryService::handleCommandFiles() {
    const bool hasSaveAll = fs::exists("saveall.cmd");
    const bool hasStartSave = fs::exists("startsave.cmd");
    const bool hasStopSave = fs::exists("stopsave.cmd");
    const bool hasExit = fs::exists("exit.cmd");
    const bool hasCleanBattleGuild = fs::exists("cleanbattleguild.cmd");
    const bool hasCrash = fs::exists("crash.cmd");
    const bool hasProbePool = fs::exists("probe_pool.cmd");
    const bool hasProbeOnce = fs::exists("probe_once.cmd");
    const bool hasProbeResult = fs::exists("probe_result.cmd");
    const bool hasProbePoolTypes = fs::exists("probe_pool.types");
    const bool hasSmuSummary = fs::exists("smu_summary.cmd");
    const bool hasSmuSummaryReset = fs::exists("smu_summary_reset.cmd");
    const bool hasSmuSummaryTotal = fs::exists("smu_summary_total.cmd");
    const bool hasSmuSummaryPeek = fs::exists("smu_summary_peek.cmd");
    const bool hasSmuSummaryTypes = fs::exists("smu_summary.types");

    if (hasSaveAll) fs::remove("saveall.cmd");
    if (hasStartSave) fs::remove("startsave.cmd");
    if (hasStopSave) fs::remove("stopsave.cmd");
    if (hasExit) fs::remove("exit.cmd");
    if (hasCleanBattleGuild) fs::remove("cleanbattleguild.cmd");
    if (hasCrash) fs::remove("crash.cmd");
    if (hasProbePool) fs::remove("probe_pool.cmd");
    if (hasProbeOnce) fs::remove("probe_once.cmd");
    if (hasProbeResult) fs::remove("probe_result.cmd");
    if (hasSmuSummary) fs::remove("smu_summary.cmd");
    if (hasSmuSummaryReset) fs::remove("smu_summary_reset.cmd");
    if (hasSmuSummaryTotal) fs::remove("smu_summary_total.cmd");
    if (hasSmuSummaryPeek) fs::remove("smu_summary_peek.cmd");

    if (hasSaveAll) {
        Logger::instance().logShare("Get SaveAll Commands");
        forceSaveAll_.store(true);
    }
    if (hasExit) {
        Logger::instance().logShare("Get Exit Commands");
        forceSaveAll_.store(true);
        pendingExitAfterSave_.store(true);
    }
    if (hasCrash) {
        Logger::instance().logShare("Get Crash Commands");
        pendingCrashSave_.store(true);
    }
    if (hasProbePool) {
        Logger::instance().logShare("Get ProbePool Commands");
        if (hasProbePoolTypes) {
            std::vector<ShareMemType> runtimeTypes;
            std::vector<std::string> invalidTokens;
            if (parseProbeTypesFile("probe_pool.types", runtimeTypes, invalidTokens)) {
                runtimeProbePoolTypes_ = std::move(runtimeTypes);
                runtimeInvalidProbeTokens_ = std::move(invalidTokens);
                hasRuntimeProbePoolTypes_ = true;
                Logger::instance().logShare(
                    "Get ProbePool Types override: " + probeTypesText(runtimeProbePoolTypes_));
                if (!runtimeInvalidProbeTokens_.empty()) {
                    Logger::instance().logShare(
                        "Get ProbePool Types invalid tokens: " + joinTokens(runtimeInvalidProbeTokens_));
                }
            } else {
                hasRuntimeProbePoolTypes_ = false;
                runtimeInvalidProbeTokens_.clear();
                Logger::instance().logShare("Get ProbePool Types override: read failed, fallback to config");
            }
            fs::remove("probe_pool.types");
        } else {
            hasRuntimeProbePoolTypes_ = false;
            runtimeInvalidProbeTokens_.clear();
        }
        pendingPoolProbe_.store(true);
    }
    if (hasProbeOnce) {
        Logger::instance().logShare("Get ProbeOnce Commands");
        pendingProbeOnce_.store(true);
    }
    if (hasProbeResult) {
        Logger::instance().logShare("Get ProbeResult Commands");
        pendingProbeResult_.store(true);
    }
    if (hasSmuSummary) {
        Logger::instance().logShare("Get SmuSummary Commands");
        pendingDispatchSummary_.store(true);
    }
    if (hasSmuSummaryReset) {
        Logger::instance().logShare("Get SmuSummaryReset Commands");
        pendingDispatchSummaryReset_.store(true);
    }
    if (hasSmuSummaryTotal) {
        Logger::instance().logShare("Get SmuSummaryTotal Commands");
        pendingDispatchSummaryTotal_.store(true);
    }
    if (hasSmuSummaryPeek) {
        Logger::instance().logShare("Get SmuSummaryPeek Commands");
        pendingDispatchSummaryPeek_.store(true);
    }
    const bool hasSummaryTrigger = hasSmuSummary || hasSmuSummaryTotal || hasSmuSummaryPeek;
    if (hasSummaryTrigger && hasSmuSummaryTypes) {
        std::vector<ShareMemType> runtimeTypes;
        std::vector<std::string> invalidTokens;
        if (parseProbeTypesFile("smu_summary.types", runtimeTypes, invalidTokens)) {
            runtimeSummaryTypes_ = std::move(runtimeTypes);
            runtimeInvalidSummaryTokens_ = std::move(invalidTokens);
            hasRuntimeSummaryTypes_ = true;
            Logger::instance().logShare(
                "Get SmuSummary Types override: " + probeTypesText(runtimeSummaryTypes_));
            if (!runtimeInvalidSummaryTokens_.empty()) {
                Logger::instance().logShare(
                    "Get SmuSummary Types invalid tokens: " + joinTokens(runtimeInvalidSummaryTokens_));
            }
        } else {
            hasRuntimeSummaryTypes_ = false;
            runtimeInvalidSummaryTokens_.clear();
            Logger::instance().logShare("Get SmuSummary Types override: read failed, fallback to ALL");
        }
        fs::remove("smu_summary.types");
    } else if (hasSummaryTrigger) {
        hasRuntimeSummaryTypes_ = false;
        runtimeInvalidSummaryTokens_.clear();
    }

    if (hasStartSave) {
        Logger::instance().logShare("Get StartSavLogout Commands");
        saveLogoutEnabled_.store(true);
    }
    if (hasStopSave) {
        Logger::instance().logShare("Get StopSavLogout Commands");
        saveLogoutEnabled_.store(false);
    }
    if (hasCleanBattleGuild) {
        Logger::instance().logShare("Get Clean All City's BattleGuild Commands");
        pendingCleanBattleGuild_.store(true);
        pendingExitAfterSave_.store(true);
    }
    return true;
}

std::string ShareMemoryService::logPoolProbe() {
    if (config_ == nullptr) return "NA";
    const auto& smus = config_->data().shareMemObjects;
    const std::size_t n = std::min(smus.size(), shmRegions_.size());
    std::uint32_t okCount = 0;
    std::uint32_t warnCount = 0;
    std::uint32_t failCount = 0;
    std::uint32_t filteredOutCount = 0;
    std::uint32_t scannedCount = 0;
    std::uint32_t idaAlignedCount = 0;
    std::uint32_t idaDeltaNonZeroCount = 0;
    std::uint32_t idaDeltaPositiveCount = 0;
    std::uint32_t idaDeltaNegativeCount = 0;
    long long idaDeltaAbsMax = 0;
    std::uint32_t e2OkCount = 0;
    std::uint32_t e2WarnCount = 0;
    std::uint32_t e2FailCount = 0;
    const std::uint32_t invalidTokenCount = hasRuntimeProbePoolTypes_
        ? static_cast<std::uint32_t>(runtimeInvalidProbeTokens_.size())
        : 0U;
    const auto& activeFilter =
        hasRuntimeProbePoolTypes_ ? runtimeProbePoolTypes_ : config_->data().probePoolTypes;
    Logger::instance().logShare("[POOL-PROBE] Begin readonly probe");
    Logger::instance().logShare("[POOL-PROBE] Active Filter: " + probeTypesText(activeFilter));
    for (std::size_t i = 0; i < n; ++i) {
        const auto& smu = smus[i];
        bool enabled = activeFilter.empty();
        if (!enabled) {
            for (const auto one : activeFilter) {
                if (one == smu.type) {
                    enabled = true;
                    break;
                }
            }
        }
        if (!enabled) {
            ++filteredOutCount;
            continue;
        }
        const auto& shm = shmRegions_[i];
        if (!shm || shm->headerPtr() == nullptr) {
            Logger::instance().logShare(
                "[POOL-PROBE] idx=" + std::to_string(i) + " key=" + std::to_string(smu.key) + " no-mapping");
            continue;
        }
        const auto* head = static_cast<const SMHead*>(shm->headerPtr());
        const std::uintptr_t body = reinterpret_cast<std::uintptr_t>(shm->headerPtr()) + sizeof(SMHead);
        const std::uint32_t stride = idaSlotStrideBytes(smu.type);
        const std::uint64_t expectedAttach = attachSizeBytes(smu.type, smu.poolSize);
        const std::uint64_t idaTotal =
            static_cast<std::uint64_t>(sizeof(SMHead)) +
            static_cast<std::uint64_t>(stride) * static_cast<std::uint64_t>(smu.poolSize);
        const long long idaDelta =
            static_cast<long long>(head->m_Size) - static_cast<long long>(idaTotal);
        const bool keyMismatch = head->m_Key != smu.key;
        const bool attachMismatch = head->m_Size != expectedAttach;
        const E2StrideState e2State =
            i < e2StrideStates_.size() ? static_cast<E2StrideState>(e2StrideStates_[i]) : E2StrideState::NotApplicable;
        std::string status = "OK";
        if (e2State == E2StrideState::Fail) {
            status = "FAIL";
        } else if (keyMismatch || attachMismatch || e2State == E2StrideState::Warn) {
            status = "WARN";
        }
        std::string line =
            "[POOL-PROBE] idx=" + std::to_string(i) + " type=" + smu.name + " key=" + std::to_string(smu.key) +
            " head.key=" + std::to_string(head->m_Key) + " head.size=" + std::to_string(head->m_Size) +
            " slots=" + std::to_string(smu.poolSize) + " stride=" + std::to_string(stride) +
            " ida_total=" + std::to_string(idaTotal) +
            " delta=" + std::to_string(idaDelta) +
            " body=" + hexAddr(body);
        Logger::instance().logShare(line);
        Logger::instance().logShare(
            "[POOL-PROBE] status=" + status + " idx=" + std::to_string(i) +
            " e2=" + e2StrideStateText(e2State) +
            " key_match=" + std::string(keyMismatch ? "0" : "1") +
            " attach_match=" + std::string(attachMismatch ? "0" : "1"));
        ++scannedCount;
        if (status == "OK") {
            ++okCount;
        } else if (status == "WARN") {
            ++warnCount;
        } else {
            ++failCount;
        }
        if (e2State == E2StrideState::Ok) ++e2OkCount;
        else if (e2State == E2StrideState::Warn) ++e2WarnCount;
        else if (e2State == E2StrideState::Fail) ++e2FailCount;
        if (idaDelta == 0) {
            ++idaAlignedCount;
        } else {
            ++idaDeltaNonZeroCount;
            if (idaDelta > 0) {
                ++idaDeltaPositiveCount;
            } else {
                ++idaDeltaNegativeCount;
            }
            const long long absDelta = idaDelta >= 0 ? idaDelta : -idaDelta;
            if (absDelta > idaDeltaAbsMax) {
                idaDeltaAbsMax = absDelta;
            }
        }
        if (stride > 0 && smu.poolSize > 0) {
            const std::uintptr_t s0 = body;
            const std::uintptr_t s1 = body + stride;
            const std::uintptr_t s2 = body + stride * 2ULL;
            Logger::instance().logShare(
                "[POOL-PROBE] slots ptr: s0=" + hexAddr(s0) + " s1=" + hexAddr(s1) + " s2=" + hexAddr(s2));
        }
    }
    Logger::instance().logShare(
        "[POOL-PROBE] summary total=" + std::to_string(scannedCount) +
        " filtered=" + std::to_string(filteredOutCount) +
        " ok=" + std::to_string(okCount) +
        " warn=" + std::to_string(warnCount) +
        " fail=" + std::to_string(failCount) +
        " ida_aligned=" + std::to_string(idaAlignedCount) +
        " ida_delta_nonzero=" + std::to_string(idaDeltaNonZeroCount) +
        " ida_delta_pos=" + std::to_string(idaDeltaPositiveCount) +
        " ida_delta_neg=" + std::to_string(idaDeltaNegativeCount) +
        " ida_delta_abs_max=" + std::to_string(idaDeltaAbsMax) +
        " e2_ok=" + std::to_string(e2OkCount) +
        " e2_warn=" + std::to_string(e2WarnCount) +
        " e2_fail=" + std::to_string(e2FailCount) +
        " invalid_tokens=" + std::to_string(invalidTokenCount));
    std::string verdict = "OK";
    if (failCount > 0 || e2FailCount > 0) {
        verdict = "FAIL";
    } else if (warnCount > 0 || e2WarnCount > 0 || invalidTokenCount > 0) {
        verdict = "WARN";
    }
    Logger::instance().logShare(
        "[POOL-PROBE][RESULT] verdict=" + verdict +
        " total=" + std::to_string(scannedCount) +
        " ok=" + std::to_string(okCount) +
        " warn=" + std::to_string(warnCount) +
        " fail=" + std::to_string(failCount) +
        " e2_ok=" + std::to_string(e2OkCount) +
        " e2_warn=" + std::to_string(e2WarnCount) +
        " e2_fail=" + std::to_string(e2FailCount) +
        " invalid_tokens=" + std::to_string(invalidTokenCount));
    lastPoolProbeVerdict_ = verdict;
    lastPoolProbeTotal_ = scannedCount;
    lastPoolProbeWarn_ = warnCount;
    lastPoolProbeFail_ = failCount;
    lastPoolProbeInvalid_ = invalidTokenCount;
    {
        std::ofstream out("probe_last.result", std::ios::trunc);
        if (out.is_open()) {
            out << "verdict=" << verdict
                << " total=" << scannedCount
                << " warn=" << warnCount
                << " fail=" << failCount
                << " invalid_tokens=" << invalidTokenCount
                << "\n";
        }
    }
    Logger::instance().logShare("[POOL-PROBE] End readonly probe");
    return verdict;
}

bool ShareMemoryService::run() {
    constexpr std::uint32_t kHeartbeatIntervalMs = 5000;
    Logger::instance().logShare("Loop...Start");
    while (!exiting_.load()) {
        if (!odbc_.isConnected()) {
            Logger::instance().logShare("connect database is fails");
            if (!odbc_.connect(config_->data().db)) {
                Logger::instance().logShare("Can't connect database");
                std::this_thread::sleep_for(std::chrono::seconds(5));
                continue;
            }
            Logger::instance().logShare("Reconnect database");
        }

        if (!handleCommandFiles()) {
            return false;
        }
        const bool forceSave = forceSaveAll_.exchange(false);
        const bool saveLogoutMode = saveLogoutEnabled_.load();
        const bool cleanBattleGuild = pendingCleanBattleGuild_.exchange(false);
        const bool crashSave = pendingCrashSave_.exchange(false);
        const bool poolProbe = pendingPoolProbe_.exchange(false);
        const bool probeOnce = pendingProbeOnce_.exchange(false);
        const bool probeResult = pendingProbeResult_.exchange(false);
        const bool forceDispatchSummary = pendingDispatchSummary_.exchange(false);
        const bool resetDispatchSummary = pendingDispatchSummaryReset_.exchange(false);
        const bool forceDispatchSummaryTotal = pendingDispatchSummaryTotal_.exchange(false);
        const bool peekDispatchSummary = pendingDispatchSummaryPeek_.exchange(false);
        if (poolProbe || probeOnce) {
            const std::string probeVerdict = logPoolProbe();
            if (probeOnce) {
                const std::string line =
                    "[F-PROBE-ONCE][RESULT] verdict=" + probeVerdict +
                    " total=" + std::to_string(lastPoolProbeTotal_) +
                    " warn=" + std::to_string(lastPoolProbeWarn_) +
                    " fail=" + std::to_string(lastPoolProbeFail_) +
                    " invalid_tokens=" + std::to_string(lastPoolProbeInvalid_);
                Logger::instance().logShare(line);
                std::ofstream out("probe_once.result", std::ios::trunc);
                if (out.is_open()) {
                    out << line << "\n";
                }
            }
        }
        if (probeResult) {
            Logger::instance().logShare(
                "[F-PROBE-LAST][RESULT] verdict=" + lastPoolProbeVerdict_ +
                " total=" + std::to_string(lastPoolProbeTotal_) +
                " warn=" + std::to_string(lastPoolProbeWarn_) +
                " fail=" + std::to_string(lastPoolProbeFail_) +
                " invalid_tokens=" + std::to_string(lastPoolProbeInvalid_));
        }
        if (resetDispatchSummary) {
            dispatchSummaryElapsedSec_ = 0;
            for (std::size_t i = 0; i < managerDispatchCalls_.size(); ++i) {
                managerDispatchCalls_[i] = 0;
            }
            for (std::size_t i = 0; i < managerDispatchErrors_.size(); ++i) {
                managerDispatchErrors_[i] = 0;
            }
            Logger::instance().logShare("[D-SUMMARY] counters reset by command");
        }
        auto driveManager = [&](std::size_t i) {
            auto& slot = managers_[i];
            managerElapsedMs_[i] += 1000;
            const bool heartbeatDue = managerElapsedMs_[i] >= kHeartbeatIntervalMs;
            const bool shouldDispatch = crashSave || heartbeatDue || forceSave;
            if (!shouldDispatch) {
                return;
            }
            try {
                if (crashSave) {
                    slot.second->heartbeat(false, saveLogoutMode, true);
                } else {
                    slot.second->heartbeat(forceSave, saveLogoutMode, false);
                    if (heartbeatDue) {
                        managerElapsedMs_[i] = 0;
                    }
                }
                ++managerDispatchCalls_[i];
                ++managerDispatchCallsTotal_[i];
            } catch (...) {
                ++managerDispatchErrors_[i];
                ++managerDispatchErrorsTotal_[i];
                Logger::instance().logShare(
                    "[D-SUMMARY] SMU heartbeat dispatch exception type=" +
                    legacyTypeToken(slot.first) + " idx=" + std::to_string(i));
            }
        };
        if (!cleanBattleGuild) {
            for (std::size_t i = 0; i < managers_.size(); ++i) {
                if (managers_[i].first == ShareMemType::Human) {
                    driveManager(i);
                }
            }
            for (std::size_t i = 0; i < managers_.size(); ++i) {
                if (managers_[i].first != ShareMemType::Human) {
                    driveManager(i);
                }
            }
        }
        if (cleanBattleGuild) {
            Logger::instance().logShare("0.Begin Clean All City BattleGuild.");
            bool hasCity = false;
            bool citySaved = false;
            for (std::size_t i = 0; i < managers_.size(); ++i) {
                if (managers_[i].first == ShareMemType::City) {
                    hasCity = true;
                    Logger::instance().logShare("1.CitySMU BattleGuild Clean In Memory.");
                    managers_[i].second->doSaveAll();
                    if (const auto* cityMgr = dynamic_cast<const CitySmuLogicManager*>(managers_[i].second.get())) {
                        citySaved = cityMgr->lastSaveAllOk();
                    } else {
                        citySaved = true;
                    }
                    break;
                }
            }
            if (!hasCity) {
                Logger::instance().logShare("FAIL:m_PoolSharePtr is NULL");
            } else if (!citySaved) {
                Logger::instance().logShare("FAIL:m_bReady is FALSE");
            } else {
                Logger::instance().logShare("2.SMULogicManager<CitySMU>::DoSaveAll() OK.");
                Logger::instance().logShare("3.End Clean All City BattleGuild.");
            }
        }
        if (crashSave) {
            Logger::instance().logShare("Crash SaveAll cycle...OK");
        }
        if (pendingExitAfterSave_.load()) {
            pendingExitAfterSave_.store(false);
            Logger::instance().logShare("Loop...Exit requested after save");
            exiting_.store(true);
        }
        if (config_ != nullptr &&
            (config_->data().smuDispatchSummaryIntervalSec > 0 ||
             forceDispatchSummary ||
             forceDispatchSummaryTotal ||
             peekDispatchSummary)) {
            ++dispatchSummaryElapsedSec_;
            const bool dueByInterval =
                config_->data().smuDispatchSummaryIntervalSec > 0 &&
                dispatchSummaryElapsedSec_ >= config_->data().smuDispatchSummaryIntervalSec;
            if (forceDispatchSummary || forceDispatchSummaryTotal || peekDispatchSummary || dueByInterval) {
                if (!peekDispatchSummary) {
                    dispatchSummaryElapsedSec_ = 0;
                }
                Logger::instance().logShare("[D-SUMMARY] SMU dispatch window begin");
                const auto& smus = config_->data().shareMemObjects;
                const std::size_t n = std::min({smus.size(), managers_.size(), managerDispatchCalls_.size(), managerDispatchErrors_.size()});
                std::uint64_t totalCalls = 0;
                std::uint64_t totalErrors = 0;
                std::uint64_t totalCallsAll = 0;
                std::uint64_t totalErrorsAll = 0;
                std::uint32_t emitted = 0;
                const std::vector<ShareMemType>* activeSummaryFilter = hasRuntimeSummaryTypes_
                    ? &runtimeSummaryTypes_
                    : nullptr;
                const std::uint32_t invalidSummaryTokenCount = hasRuntimeSummaryTypes_
                    ? static_cast<std::uint32_t>(runtimeInvalidSummaryTokens_.size())
                    : 0U;
                Logger::instance().logShare(
                    "[D-SUMMARY] active_filter=" + probeTypesText(activeSummaryFilter ? *activeSummaryFilter : std::vector<ShareMemType>{}) +
                    " invalid_tokens=" + std::to_string(invalidSummaryTokenCount));
                for (std::size_t i = 0; i < n; ++i) {
                    bool filterHit = activeSummaryFilter == nullptr;
                    if (!filterHit) {
                        for (const auto t : *activeSummaryFilter) {
                            if (t == smus[i].type) {
                                filterHit = true;
                                break;
                            }
                        }
                    }
                    if (!filterHit) {
                        if (!peekDispatchSummary) {
                            managerDispatchCalls_[i] = 0;
                            managerDispatchErrors_[i] = 0;
                        }
                        continue;
                    }
                    totalCalls += managerDispatchCalls_[i];
                    totalErrors += managerDispatchErrors_[i];
                    totalCallsAll += managerDispatchCallsTotal_[i];
                    totalErrorsAll += managerDispatchErrorsTotal_[i];
                    const bool shouldEmit = !config_->data().smuDispatchSummaryAnomalyOnly || managerDispatchErrors_[i] > 0;
                    if (shouldEmit) {
                        Logger::instance().logShare(
                            "[D-SUMMARY] idx=" + std::to_string(i) +
                            " type=" + smus[i].name +
                            " calls=" + std::to_string(managerDispatchCalls_[i]) +
                            " errors=" + std::to_string(managerDispatchErrors_[i]));
                        ++emitted;
                    }
                    if (!peekDispatchSummary) {
                        managerDispatchCalls_[i] = 0;
                        managerDispatchErrors_[i] = 0;
                    }
                }
                Logger::instance().logShare(
                    "[D-SUMMARY] total calls=" + std::to_string(totalCalls) +
                    " errors=" + std::to_string(totalErrors) +
                    " emitted=" + std::to_string(emitted) +
                    " anomaly_only=" + std::string(config_->data().smuDispatchSummaryAnomalyOnly ? "1" : "0") +
                    " peek=" + std::string(peekDispatchSummary ? "1" : "0"));
                if (forceDispatchSummaryTotal) {
                    Logger::instance().logShare(
                        "[D-SUMMARY] lifetime calls=" + std::to_string(totalCallsAll) +
                        " errors=" + std::to_string(totalErrorsAll));
                }
                Logger::instance().logShare("[D-SUMMARY] SMU dispatch window end");
            }
        }
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    Logger::instance().logShare("Loop...End");
    for (auto& shm : shmRegions_) {
        if (shm) {
            shm->detach(config_->data().removeShareMemOnExit);
        }
    }
    shmRegions_.clear();
    return true;
}

void ShareMemoryService::stop() {
    exiting_.store(true);
}
