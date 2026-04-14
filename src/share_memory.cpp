#include "share_memory.hpp"
#include "db_access.hpp"
#include "logger.hpp"

#include <chrono>
#include <cstddef>
#include <filesystem>
#include <memory>
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
    explicit HumanSmuLogicManager(OdbcInterface& db) : repo_(db) {}

    bool init(const ShareMemData& data) override {
        data_ = data;
        if (!repo_.loadCount(charCount_)) {
            return false;
        }
        return true;
    }

    void doDefault() override {}

    void doNormalSave() override {
        persistHuman(false);
    }

    void doSaveAll() override {
        persistHuman(true);
    }

    void persistHuman(bool logOkLines) {
        if (!repo_.saveAll() || !repo_.saveAllExtra()) {
            Logger::instance().logShare("End HumanSMU_" + std::to_string(data_.key) + " SaveAll...Get Errors!");
            return;
        }
        if (logOkLines) {
            Logger::instance().logShare("End HumanSMU_" + std::to_string(data_.key) + " SaveAll...OK!");
            Logger::instance().logShare("SMUCount = 0");
            Logger::instance().logShare("TotalSMUSize = 0");
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
            if (!repo_.saveLogoutFocused()) {
                Logger::instance().logShare("Human SaveLogout...Get Errors!");
            }
        }
    }


private:
    HumanDbRepo repo_;
    ShareMemData data_;
    long long charCount_{0};
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
        if (!repo_.saveAll() || !repo_.saveAllUsers()) {
            Logger::instance().logShare("End GuildSMU_" + std::to_string(data_.key) + " SaveAll...Get Errors!");
            return;
        }
        Logger::instance().logShare("End GuildSMU_" + std::to_string(data_.key) + " SaveAll...OK!");
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
        if (!repo_.saveAll()) {
        }
        Logger::instance().logShare("End MailSMU_" + std::to_string(data_.key) + " SaveAll...OK!");
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
        if (!repo_.loadMaxSerial(maxSerial_)) {
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
        if (!repo_.saveAll()) {
            Logger::instance().logShare(
                "ItemSerial Save...Error! Serial = " + std::to_string(maxSerial_) +
                " ,ServerID = " + std::to_string(serverId_));
            return;
        }
        long long serial = 0;
        if (!repo_.loadMaxSerial(serial)) {
            serial = maxSerial_;
        } else {
            maxSerial_ = serial;
        }
        Logger::instance().logShare(
            "ItemSerial Save...OK! Serial = " + std::to_string(serial) +
            " ,ServerID = " + std::to_string(serverId_));
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
        if (!repo_.saveAll()) {
            lastSaveAllOk_ = false;
            Logger::instance().logShare("End CityInfoSM_" + std::to_string(data_.key) + " SaveAll...Get Errors!");
            return;
        }
        lastSaveAllOk_ = true;
        Logger::instance().logShare("End CityInfoSM_" + std::to_string(data_.key) + " SaveAll...OK!");
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
        if (!repo_.saveAll()) {
            Logger::instance().logShare("GlobalData Save...Error!");
            return;
        }
        Logger::instance().logShare("GlobalData Save...OK!");
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
        if (!repo_.saveAll() || !repo_.saveAllUnits()) {
            Logger::instance().logShare("CommisionShop Save...Error!");
            return;
        }
        Logger::instance().logShare("Auction Save...OK!");
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
        if (!repo_.saveAll() || !repo_.saveAllByType()) {
            Logger::instance().logShare("TopList[ID:0] Save...Error!");
            Logger::instance().logShare("TopList[ID:1] Save...Error!");
            Logger::instance().logShare("TopList[ID:2] Save...Error!");
            return;
        }
        Logger::instance().logShare("TopList[ID:0] Save...OK!");
        Logger::instance().logShare("TopList[ID:1] Save...OK!");
        Logger::instance().logShare("TopList[ID:2] Save...OK!");
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
        if (!repo_.saveAll()) {
            Logger::instance().logShare("CommisionShop Save...Error!");
            return;
        }
        Logger::instance().logShare("CommisionShop Save...OK!");
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
        if (!repo_.saveAll()) {
            Logger::instance().logShare("End PlayerShopSM_" + std::to_string(data_.key) + " SaveAll...Get Errors!");
            return;
        }
        Logger::instance().logShare("End PlayerShopSM_" + std::to_string(data_.key) + " SaveAll...OK!");
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

// Calibrated to legacy ShareMemory attach logs (total bytes at default pool sizes from ShareMemInfo.ini).
std::uint64_t attachSizeBytes(ShareMemType type, std::uint32_t poolSlots) {
    struct Row {
        ShareMemType t;
        std::uint64_t totalAtDefault;
        std::uint32_t defaultSlots;
    };
    static constexpr Row kTable[] = {
        {ShareMemType::Human, 478722520ULL, 200000},
        {ShareMemType::Guild, 75622420ULL, 20000},
        {ShareMemType::Mail, 77209620ULL, 50000},
        {ShareMemType::PlayerShop, 267534356ULL, 50000},
        {ShareMemType::GlobalData, 21524ULL, 512},
        {ShareMemType::CommissionShop, 69830ULL, 50000},
        {ShareMemType::ItemSerial, 41ULL, 1},
        {ShareMemType::PetProcreate, 1172500ULL, 10000},
        {ShareMemType::City, 9780035ULL, 1024},
        {ShareMemType::GuildLeague, 188948ULL, 20000},
        {ShareMemType::Auction, 93075020ULL, 50000},
        {ShareMemType::TopList, 13717ULL, 10000},
    };
    for (const auto& r : kTable) {
        if (r.t == type && r.defaultSlots != 0) {
            return (r.totalAtDefault * static_cast<std::uint64_t>(poolSlots)) /
                   static_cast<std::uint64_t>(r.defaultSlots);
        }
    }
    return static_cast<std::uint64_t>(poolSlots) * 1024ULL;
}

std::unique_ptr<ISmuLogicManager> makeManagerFor(
    const ServiceConfig& cfg,
    OdbcInterface& odbc,
    const ShareMemData& smu
) {
    switch (smu.type) {
        case ShareMemType::Human:
            return std::make_unique<HumanSmuLogicManager>(odbc);
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
    Logger::instance().logShare("Start Read Config Files...");
    Logger::instance().logShare("Read Config Files...OK!");
    Logger::instance().logShare("Start New Managers...");
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

    for (const auto& smu : smus) {
        auto mgr = makeManagerFor(config.data(), odbc_, smu);
        const auto legacyToken = legacyTypeToken(smu.type);
        Logger::instance().logShare("new SMUPool<" + legacyToken + ">()...OK");
        Logger::instance().logShare("new SMULogicManager<" + legacyToken + ">()...OK");
        managers_.push_back({smu.type, std::move(mgr)});
        managerIntervalsMs_.push_back(smu.intervalMs);
        managerElapsedMs_.push_back(0);
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
        "Config::WorldID=" + std::to_string(config.data().worldId) +
        ",Config::ZoneID=" + std::to_string(config.data().zoneId) +
        "......DB::WorldID=" + std::to_string(dbWorldId) +
        ",DB::ZoneID=" + std::to_string(dbZoneId) + "...Compare");
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

    if (hasSaveAll) fs::remove("saveall.cmd");
    if (hasStartSave) fs::remove("startsave.cmd");
    if (hasStopSave) fs::remove("stopsave.cmd");
    if (hasExit) fs::remove("exit.cmd");
    if (hasCleanBattleGuild) fs::remove("cleanbattleguild.cmd");
    if (hasCrash) fs::remove("crash.cmd");

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
        pendingCrashSave_.store(true);
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
        auto driveManager = [&](std::size_t i) {
            auto& slot = managers_[i];
            managerElapsedMs_[i] += 1000;
            const bool heartbeatDue = managerElapsedMs_[i] >= kHeartbeatIntervalMs;
            if (crashSave) {
                slot.second->heartbeat(false, saveLogoutMode, true);
            } else if (heartbeatDue || forceSave) {
                slot.second->heartbeat(forceSave, saveLogoutMode, false);
                if (heartbeatDue) {
                    managerElapsedMs_[i] = 0;
                }
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
        if (pendingExitAfterSave_.load()) {
            pendingExitAfterSave_.store(false);
            exiting_.store(true);
        }
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    Logger::instance().logShare("Loop...End");
    return true;
}

void ShareMemoryService::stop() {
    exiting_.store(true);
}
