#include "config.hpp"
#include "logger.hpp"

#include <filesystem>
#include <fstream>
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
}  // namespace

bool Config::load(const std::string& filePath) {
    data_ = {};
    std::unordered_map<std::string, std::string> kv;
    if (!parseIniFile(filePath, kv)) return false;

    data_.db.dsn = parseStringDef(kv, "system.dbname", "");
    data_.db.user = parseStringDef(kv, "system.dbuser", "");
    data_.db.password = parseStringDef(kv, "system.dbpassword", "");
    data_.currentServerId = parseIntDef(kv, "system.currentserverid", 0);

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
        d.poolSize = defaultPoolSize(mapped);
        d.name = typeNameFromCode(typeCode);
        data_.shareMemObjects.push_back(d);
    }

    const auto shareMemPath = std::filesystem::path(filePath);
    const auto configDir = shareMemPath.parent_path();

    {
        std::unordered_map<std::string, std::string> worldKv;
        if (parseIniFile((configDir / "WorldInfo.ini").string(), worldKv)) {
            data_.worldId =
                parseIntDef(worldKv, "world.worldid", parseIntDef(worldKv, "system.worldid", 0));
            // 服务器 ID：优先 serverid；若无则用 zoneid（日志仍为 Config::ZoneID=… 与原版一致）
            data_.zoneId = parseIntDef(
                worldKv,
                "world.serverid",
                parseIntDef(
                    worldKv,
                    "system.serverid",
                    parseIntDef(
                        worldKv,
                        "world.zoneid",
                        parseIntDef(worldKv, "system.zoneid", 0))));
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
    if (parseIniFile(serverInfoPath, serverKv)) {
        data_.currentServerId = parseIntDef(serverKv, "system.currentserverid", data_.currentServerId);
        applyServerKeyOverrides(serverKv, data_);
        buildServerKeyIndex(serverKv, data_);
        logOnlyReload("ServerInfo.ini");
    }
    logOnlyReload("SceneInfo.ini");
    Logger::instance().logConfig("Load ./Config/TopListInfo.ini ...Only OK! ");

    return data_.enableShareMem && !data_.db.dsn.empty() && !data_.shareMemObjects.empty();
}
