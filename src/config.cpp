#include "config.hpp"
#include "logger.hpp"

#include <filesystem>
#include <fstream>
#include <initializer_list>
#include <unordered_map>

namespace {
std::string trim(const std::string& s) {
    const auto left = s.find_first_not_of(" \t\r\n");
    if (left == std::string::npos) {
        return {};
    }
    const auto right = s.find_last_not_of(" \t\r\n");
    return s.substr(left, right - left + 1);
}

std::string stripInlineComment(const std::string& s) {
    const auto pos = s.find(';');
    if (pos == std::string::npos) return trim(s);
    return trim(s.substr(0, pos));
}

std::vector<std::string> splitCsvLower(const std::string& raw) {
    auto lowerTrim = [](const std::string& v) {
        std::string t = trim(v);
        for (auto& ch : t) {
            if (ch >= 'A' && ch <= 'Z') ch = static_cast<char>(ch - 'A' + 'a');
        }
        return t;
    };
    std::vector<std::string> out;
    std::string cur;
    for (char ch : raw) {
        if (ch == ',') {
            const auto t = lowerTrim(cur);
            if (!t.empty()) out.push_back(t);
            cur.clear();
            continue;
        }
        cur.push_back(ch);
    }
    const auto t = lowerTrim(cur);
    if (!t.empty()) out.push_back(t);
    return out;
}

ShareMemType parseIdaTypeCode(int typeCode) {
    switch (typeCode) {
        case 1: return ShareMemType::Human;
        case 2: return ShareMemType::Guild;
        case 3: return ShareMemType::Mail;
        case 4: return ShareMemType::PlayerShop;
        case 5: return ShareMemType::GlobalData;
        case 6: return ShareMemType::CommissionShop;
        case 7: return ShareMemType::ItemSerial;
        case 8: return ShareMemType::PetProcreate;
        case 9: return ShareMemType::City;
        case 10: return ShareMemType::GuildLeague;
        case 11: return ShareMemType::Auction;
        case 12: return ShareMemType::TopList;
        default: return ShareMemType::Invalid;
    }
}

std::string typeNameFromCode(int typeCode) {
    switch (typeCode) {
        case 1: return "HumanSMU";
        case 2: return "GuildSMU";
        case 3: return "MailSMU";
        case 4: return "PlayerShopSM";
        case 5: return "GlobalDataSMU";
        case 6: return "CommisionShopSMU";
        case 7: return "ItemSerialKeySMU";
        case 8: return "PetProcreateItemSM";
        case 9: return "CitySMU";
        case 10: return "GuildLeagueSMU";
        case 11: return "AuctionSMU";
        case 12: return "TopListSMU";
        default: return "UnknownSMU";
    }
}

std::string toLowerCopy(std::string v) {
    for (auto& ch : v) {
        if (ch >= 'A' && ch <= 'Z') ch = static_cast<char>(ch - 'A' + 'a');
    }
    return v;
}

void stripUtf8Bom(std::string& line) {
    if (line.size() >= 3 && static_cast<unsigned char>(line[0]) == 0xEF && static_cast<unsigned char>(line[1]) == 0xBB &&
        static_cast<unsigned char>(line[2]) == 0xBF) {
        line.erase(0, 3);
    }
}

void applyFallbackServerIdIfNeeded(ServiceConfig& cfg) {
    // Only bump process server id; ZoneID stays from WorldInfo for DB Compare (legacy: Config::ZoneID often 0).
    if (cfg.currentServerId <= 0 && cfg.fallbackServerId > 0) {
        cfg.currentServerId = cfg.fallbackServerId;
    }
}

int parseIntFirstPositive(const std::unordered_map<std::string, std::string>& kv, std::initializer_list<const char*> keys) {
    for (const char* k : keys) {
        const auto it = kv.find(k);
        if (it == kv.end()) {
            continue;
        }
        try {
            const int v = std::stoi(it->second);
            if (v > 0) {
                return v;
            }
        } catch (...) {
        }
    }
    return 0;
}

int parseIntDef(const std::unordered_map<std::string, std::string>& kv, const std::string& key, int def) {
    const auto it = kv.find(key);
    if (it == kv.end()) return def;
    try {
        return std::stoi(it->second);
    } catch (...) {
        return def;
    }
}

std::string parseStringDef(const std::unordered_map<std::string, std::string>& kv, const std::string& key, const std::string& def) {
    const auto it = kv.find(key);
    if (it == kv.end()) return def;
    return it->second;
}

std::uint64_t parseUInt64Def(const std::unordered_map<std::string, std::string>& kv, const std::string& key, std::uint64_t def) {
    const auto it = kv.find(key);
    if (it == kv.end()) return def;
    try {
        return std::stoull(it->second, nullptr, 0);
    } catch (...) {
        return def;
    }
}

std::uint32_t parseUInt32MinDef(
    const std::unordered_map<std::string, std::string>& kv,
    const std::string& key,
    std::uint32_t def,
    std::uint32_t minValue
) {
    const int parsed = parseIntDef(kv, key, static_cast<int>(def));
    if (parsed < static_cast<int>(minValue)) {
        return def;
    }
    return static_cast<std::uint32_t>(parsed);
}

std::uint32_t defaultPoolSize(ShareMemType type) {
    switch (type) {
        case ShareMemType::Human: return 200000;
        case ShareMemType::Guild: return 20000;
        case ShareMemType::Mail: return 50000;
        case ShareMemType::ItemSerial: return 1;
        case ShareMemType::City: return 1024;
        case ShareMemType::GlobalData: return 512;
        case ShareMemType::Auction: return 50000;
        case ShareMemType::TopList: return 10000;
        case ShareMemType::CommissionShop: return 50000;
        case ShareMemType::PlayerShop: return 50000;
        case ShareMemType::PetProcreate: return 10000;
        case ShareMemType::GuildLeague: return 20000;
        default: return 0;
    }
}

bool parseIniFile(const std::string& path, std::unordered_map<std::string, std::string>& kv) {
    std::ifstream in(path);
    if (!in.is_open()) return false;

    std::string section;
    std::string line;
    while (std::getline(in, line)) {
        line = trim(line);
        stripUtf8Bom(line);
        if (line.empty() || line.starts_with(';') || line.starts_with('#')) {
            continue;
        }
        if (line.front() == '[' && line.back() == ']') {
            section = toLowerCopy(trim(line.substr(1, line.size() - 2)));
            continue;
        }
        const auto eq = line.find('=');
        if (eq == std::string::npos) {
            continue;
        }
        auto key = toLowerCopy(trim(line.substr(0, eq)));
        auto value = stripInlineComment(line.substr(eq + 1));
        if (value.empty()) continue;
        if (!section.empty()) {
            key = section + "." + key;
        }
        kv[key] = value;
    }
    return true;
}

void applyServerKeyOverrides(const std::unordered_map<std::string, std::string>& kv, ServiceConfig& cfg) {
    const int sid = cfg.currentServerId;
    const std::string sec = "server" + std::to_string(sid) + ".";
    const auto getKey = [&](const std::string& keyName) -> std::uint64_t {
        return parseUInt64Def(kv, sec + keyName, 0);
    };

    const bool enable = parseIntDef(kv, sec + "enablesharemem", 1) != 0;
    cfg.enableShareMem = enable;
    if (!enable) return;

    const auto humanKey = getKey("humansmkey");
    const auto pshopKey = getKey("playshopsmkey");
    const auto itemSerialKey = getKey("itemserialkey");
    const auto cshopKey = getKey("commisionshopkey");
    const auto auctionKey = getKey("auctionkey");

    for (auto& smu : cfg.shareMemObjects) {
        switch (smu.type) {
            case ShareMemType::Human:
                if (humanKey != 0) smu.key = humanKey;
                break;
            case ShareMemType::PlayerShop:
                if (pshopKey != 0) smu.key = pshopKey;
                break;
            case ShareMemType::ItemSerial:
                if (itemSerialKey != 0) smu.key = itemSerialKey;
                break;
            case ShareMemType::CommissionShop:
                if (cshopKey != 0) smu.key = cshopKey;
                break;
            case ShareMemType::Auction:
                if (auctionKey != 0) smu.key = auctionKey;
                break;
            default:
                break;
        }
    }
}

void buildServerKeyIndex(const std::unordered_map<std::string, std::string>& kv, ServiceConfig& cfg) {
    cfg.shareMemKeyToServerId.clear();
    constexpr const char* kNames[] = {
        "humansmkey",
        "playshopsmkey",
        "itemserialkey",
        "commisionshopkey",
        "auctionkey"
    };
    for (const auto& [k, v] : kv) {
        if (!k.starts_with("server")) {
            continue;
        }
        const auto dot = k.find('.');
        if (dot == std::string::npos || dot <= 6) {
            continue;
        }
        const auto sidStr = k.substr(6, dot - 6);
        int sid = 0;
        try {
            sid = std::stoi(sidStr);
        } catch (...) {
            continue;
        }
        const auto name = k.substr(dot + 1);
        bool matched = false;
        for (const char* n : kNames) {
            if (name == n) {
                matched = true;
                break;
            }
        }
        if (!matched) {
            continue;
        }
        std::uint64_t keyVal = 0;
        try {
            keyVal = std::stoull(v, nullptr, 0);
        } catch (...) {
            continue;
        }
        if (keyVal != 0) {
            cfg.shareMemKeyToServerId[keyVal] = sid;
        }
    }
}

/// Smallest N such that ServerInfo/ShareMem has a non-zero `serverN.(human|playshop|...)key` — matches legacy multi-world INI layout.
int inferSmallestServerIdWithKeys(const std::unordered_map<std::string, std::string>& kv) {
    int best = 0;
    constexpr const char* kNames[] = {
        "humansmkey",
        "playshopsmkey",
        "itemserialkey",
        "commisionshopkey",
        "auctionkey"
    };
    for (const auto& [k, v] : kv) {
        if (!k.starts_with("server")) {
            continue;
        }
        const auto dot = k.find('.');
        if (dot == std::string::npos || dot <= 6) {
            continue;
        }
        const auto sidStr = k.substr(6, dot - 6);
        int sid = 0;
        try {
            sid = std::stoi(sidStr);
        } catch (...) {
            continue;
        }
        if (sid <= 0) {
            continue;
        }
        const auto name = k.substr(dot + 1);
        bool matched = false;
        for (const char* n : kNames) {
            if (name == n) {
                matched = true;
                break;
            }
        }
        if (!matched) {
            continue;
        }
        std::uint64_t keyVal = 0;
        try {
            keyVal = std::stoull(v, nullptr, 0);
        } catch (...) {
            continue;
        }
        if (keyVal == 0) {
            continue;
        }
        if (best == 0 || sid < best) {
            best = sid;
        }
    }
    return best;
}

void parseProbePoolTypes(
    const std::unordered_map<std::string, std::string>& kv,
    const std::string& key,
    std::vector<ShareMemType>& out
) {
    const auto it = kv.find(key);
    if (it == kv.end()) return;
    out.clear();
    for (const auto& token : splitCsvLower(it->second)) {
        if (token == "all") {
            out.clear();
            return;
        }
        if (token == "human" || token == "humansmu") out.push_back(ShareMemType::Human);
        else if (token == "guild" || token == "guildsmu") out.push_back(ShareMemType::Guild);
        else if (token == "mail" || token == "mailsmu") out.push_back(ShareMemType::Mail);
        else if (token == "playershop" || token == "playershopsm") out.push_back(ShareMemType::PlayerShop);
        else if (token == "globaldata" || token == "globaldatasmu") out.push_back(ShareMemType::GlobalData);
        else if (token == "commissionshop" || token == "commisionshop" || token == "commisionshopsmu") out.push_back(ShareMemType::CommissionShop);
        else if (token == "itemserial" || token == "itemserialkeysmu") out.push_back(ShareMemType::ItemSerial);
        else if (token == "petprocreate" || token == "petprocreateitemsm") out.push_back(ShareMemType::PetProcreate);
        else if (token == "city" || token == "citysmu") out.push_back(ShareMemType::City);
        else if (token == "guildleague" || token == "guildleaguesmu") out.push_back(ShareMemType::GuildLeague);
        else if (token == "auction" || token == "auctionsmu") out.push_back(ShareMemType::Auction);
        else if (token == "toplist" || token == "toplistsmu") out.push_back(ShareMemType::TopList);
    }
}
}  // namespace

bool Config::load(const std::string& filePath) {
    data_ = {};
    std::unordered_map<std::string, std::string> kv;
    if (!parseIniFile(filePath, kv)) return false;

    data_.db.dsn = parseStringDef(kv, "system.dbname", "");
    data_.db.user = parseStringDef(kv, "system.dbuser", "");
    data_.db.password = parseStringDef(kv, "system.dbpassword", "");
    data_.currentServerId = parseIntDef(kv, "system.currentserverid", 0);
    data_.strictWorldZoneCheck = parseIntDef(kv, "system.strict_world_zone_check", 0) != 0;
    data_.strictStrideProbeCheck = parseIntDef(kv, "system.strict_stride_probe_check", 0) != 0;
    data_.humanNormalSaveLogEvery = parseUInt32MinDef(
        kv,
        "system.human_normal_save_log_every",
        data_.humanNormalSaveLogEvery,
        1);
    data_.humanSaveFailureWarnThreshold = parseUInt32MinDef(
        kv,
        "system.human_save_failure_warn_threshold",
        data_.humanSaveFailureWarnThreshold,
        1);
    data_.humanSaveFailureErrorThreshold = parseUInt32MinDef(
        kv,
        "system.human_save_failure_error_threshold",
        data_.humanSaveFailureErrorThreshold,
        1);
    data_.humanSaveFailureEscalateLogEvery = parseUInt32MinDef(
        kv,
        "system.human_save_failure_escalate_log_every",
        data_.humanSaveFailureEscalateLogEvery,
        1);
    data_.humanStageWindowLogAnomalyOnly =
        parseIntDef(kv, "system.human_stage_window_log_anomaly_only", data_.humanStageWindowLogAnomalyOnly ? 1 : 0) != 0;
    data_.smuDispatchSummaryIntervalSec = parseUInt32MinDef(
        kv,
        "system.smu_dispatch_summary_interval_sec",
        data_.smuDispatchSummaryIntervalSec,
        0);
    data_.smuDispatchSummaryAnomalyOnly =
        parseIntDef(kv, "system.smu_dispatch_summary_anomaly_only", data_.smuDispatchSummaryAnomalyOnly ? 1 : 0) != 0;
    data_.strictServerIdCheck =
        parseIntDef(kv, "system.strict_server_id_check", data_.strictServerIdCheck ? 1 : 0) != 0;
    data_.fallbackServerId = parseIntDef(kv, "system.fallback_server_id", 0);
    parseProbePoolTypes(kv, "system.probe_pool_types", data_.probePoolTypes);
    // B4: default safe policy is detach-only (no IPC_RMID).
    data_.removeShareMemOnExit = parseIntDef(kv, "sharemem.ipc_rmid_on_exit", 0) != 0;

    const int keyCount = parseIntDef(kv, "sharemem.keycount", 0);
    for (int i = 0; i < keyCount; ++i) {
        const auto typeCode = parseIntDef(kv, "sharemem.type" + std::to_string(i), -1);
        const auto mapped = parseIdaTypeCode(typeCode);
        if (mapped == ShareMemType::Invalid) {
            continue;
        }

        ShareMemData d;
        d.type = mapped;
        d.typeCode = typeCode;
        d.keyIndex = i;
        d.key = parseUInt64Def(kv, "sharemem.key" + std::to_string(i), 0);
        d.intervalMs = static_cast<std::uint32_t>(parseIntDef(kv, "sharemem.interval" + std::to_string(i), 0));
        const int poolOverride = parseIntDef(kv, "sharemem.poolsize" + std::to_string(i), 0);
        d.poolSize = poolOverride > 0 ? static_cast<std::uint32_t>(poolOverride) : defaultPoolSize(mapped);
        d.name = typeNameFromCode(typeCode);
        data_.shareMemObjects.push_back(d);
    }

    const auto shareMemPath = std::filesystem::path(filePath);
    const auto configDir = shareMemPath.parent_path();

    {
        std::unordered_map<std::string, std::string> worldKv;
        const auto worldPrimary = (configDir / "WorldInfo.ini").string();
        bool worldLoaded = parseIniFile(worldPrimary, worldKv);
        if (!worldLoaded) {
            const auto worldAlt = (configDir.parent_path() / "WorldInfo.ini").string();
            worldLoaded = parseIniFile(worldAlt, worldKv);
        }
        if (worldLoaded) {
            data_.worldId =
                parseIntDef(worldKv, "world.worldid", parseIntDef(worldKv, "system.worldid", 0));
            // 区服 ID：多组键名兼容不同发行版 INI（取第一个正数）
            data_.zoneId = parseIntFirstPositive(
                worldKv,
                {"world.serverid",
                 "system.serverid",
                 "world.zoneid",
                 "system.zoneid",
                 "worldinfo.serverid",
                 "worldinfo.zoneid",
                 "config.serverid",
                 "server.serverid",
                 "game.serverid",
                 "login.serverid"});
            if (data_.currentServerId <= 0) {
                data_.currentServerId = data_.zoneId;
            }
        }
    }

    auto logOnlyReload = [](const std::string& name, bool withDotSlashTopList = false) {
        const std::string prefix = (withDotSlashTopList ? "./Config/" : "");
        Logger::instance().logConfig("Load " + prefix + name + " ...Only OK! ");
        Logger::instance().logConfig("Load " + prefix + name + " ...ReLoad OK! ");
    };

    // Keep old config log style and order.
    logOnlyReload("BillingInfo.ini");
    logOnlyReload("ConfigInfo.ini");
    logOnlyReload("LoginInfo.ini");
    logOnlyReload("WorldInfo.ini");
    Logger::instance().logConfig("Load ShareMemInfo.ini ...Only OK! ");
    logOnlyReload("MachineInfo.ini");

    const auto serverInfoPath = (configDir / "ServerInfo.ini").string();
    std::unordered_map<std::string, std::string> serverKv;
    const bool serverInfoLoaded = parseIniFile(serverInfoPath, serverKv);
    if (serverInfoLoaded) {
        // Only accept explicit positive IDs; literal 0 must not wipe WorldInfo/zoneId fallback.
        if (const auto it = serverKv.find("system.currentserverid"); it != serverKv.end()) {
            try {
                const int v = std::stoi(it->second);
                if (v > 0) {
                    data_.currentServerId = v;
                }
            } catch (...) {
            }
        }
        if (data_.currentServerId <= 0 && data_.zoneId > 0) {
            data_.currentServerId = data_.zoneId;
        }
        data_.strictWorldZoneCheck =
            parseIntDef(serverKv, "system.strict_world_zone_check", data_.strictWorldZoneCheck ? 1 : 0) != 0;
        data_.strictStrideProbeCheck =
            parseIntDef(serverKv, "system.strict_stride_probe_check", data_.strictStrideProbeCheck ? 1 : 0) != 0;
        data_.humanNormalSaveLogEvery = parseUInt32MinDef(
            serverKv,
            "system.human_normal_save_log_every",
            data_.humanNormalSaveLogEvery,
            1);
        data_.humanSaveFailureWarnThreshold = parseUInt32MinDef(
            serverKv,
            "system.human_save_failure_warn_threshold",
            data_.humanSaveFailureWarnThreshold,
            1);
        data_.humanSaveFailureErrorThreshold = parseUInt32MinDef(
            serverKv,
            "system.human_save_failure_error_threshold",
            data_.humanSaveFailureErrorThreshold,
            1);
        data_.humanSaveFailureEscalateLogEvery = parseUInt32MinDef(
            serverKv,
            "system.human_save_failure_escalate_log_every",
            data_.humanSaveFailureEscalateLogEvery,
            1);
        data_.humanStageWindowLogAnomalyOnly =
            parseIntDef(
                serverKv,
                "system.human_stage_window_log_anomaly_only",
                data_.humanStageWindowLogAnomalyOnly ? 1 : 0) != 0;
        data_.smuDispatchSummaryIntervalSec = parseUInt32MinDef(
            serverKv,
            "system.smu_dispatch_summary_interval_sec",
            data_.smuDispatchSummaryIntervalSec,
            0);
        data_.smuDispatchSummaryAnomalyOnly =
            parseIntDef(
                serverKv,
                "system.smu_dispatch_summary_anomaly_only",
                data_.smuDispatchSummaryAnomalyOnly ? 1 : 0) != 0;
        data_.strictServerIdCheck =
            parseIntDef(
                serverKv,
                "system.strict_server_id_check",
                data_.strictServerIdCheck ? 1 : 0) != 0;
        data_.fallbackServerId = parseIntDef(serverKv, "system.fallback_server_id", data_.fallbackServerId);
        parseProbePoolTypes(serverKv, "system.probe_pool_types", data_.probePoolTypes);
        data_.removeShareMemOnExit =
            parseIntDef(serverKv, "system.ipc_rmid_on_exit", data_.removeShareMemOnExit ? 1 : 0) != 0;
        logOnlyReload("ServerInfo.ini");
    }
    if (data_.currentServerId <= 0 && data_.zoneId <= 0) {
        int inferred = serverInfoLoaded ? inferSmallestServerIdWithKeys(serverKv) : 0;
        if (inferred <= 0) {
            inferred = inferSmallestServerIdWithKeys(kv);
        }
        if (inferred > 0) {
            data_.currentServerId = inferred;
        }
    }
    applyFallbackServerIdIfNeeded(data_);
    if (data_.currentServerId <= 0 && data_.zoneId <= 0) {
        data_.currentServerId = 1;
    }
    if (serverInfoLoaded) {
        applyServerKeyOverrides(serverKv, data_);
        buildServerKeyIndex(serverKv, data_);
    }
    logOnlyReload("SceneInfo.ini");
    Logger::instance().logConfig("Load ./Config/TopListInfo.ini ...Only OK! ");

    return data_.enableShareMem && !data_.db.dsn.empty() && !data_.shareMemObjects.empty();
}
