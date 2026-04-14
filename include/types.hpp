#pragma once

#include <cstdint>
#include <string>

enum class ShareMemType : std::uint32_t {
    Invalid = 0,
    Human = 1,
    Guild = 2,
    Mail = 3,
    ItemSerial = 4,
    City = 5,
    GlobalData = 6,
    Auction = 7,
    TopList = 8,
    CommissionShop = 9,
    PlayerShop = 10,
    PetProcreate = 11,
    GuildLeague = 12
};

enum class CommandType : std::uint32_t {
    Unknown = 0,
    SaveAll = 1,
    Exit = 2,
    ClearAll = 3,
    StartSaveLogout = 4,
    StopSaveLogout = 5
};

struct ShareMemData {
    ShareMemType type{ShareMemType::Invalid};
    int typeCode{0};
    int keyIndex{-1};
    std::uint64_t key{0};
    std::uint32_t poolSize{0};
    std::uint32_t intervalMs{0};
    std::string name;
};
