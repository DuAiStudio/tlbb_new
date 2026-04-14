#pragma once

#include "types.hpp"

#include <cstdint>

/// B2: Total SysV segment size used by ShareMemory / World for each SMU pool.
///
/// Old code (`tlbb2007/Server/SMU/SMUManager.h`):
///   `Attach(key, sizeof(T) * nMaxCount + sizeof(SMHead))`
/// i.e. **totalBytes = poolSlots * sizeof(element) + sizeof(SMHead)**.
///
/// We do not compile `HumanSMU` etc. here; instead we use **observed segment totals**
/// from Linux ShareMemory logs at **default** `poolSlots` (same defaults as
/// `defaultPoolSize()` in `config.cpp`), and **scale linearly** when `poolSlots` differs:
///
///   totalBytes(poolSlots) = totalAtDefault * poolSlots / defaultSlots
///
/// The table values are **full segment sizes** (payload + SMHead) as logged by the
/// reference binary. After B1, `SMHead` is 24 bytes on LP64; the table remains valid
/// as a proportional baseline — if your IDA/World build uses different pool counts or
/// struct sizes, set `sharemem.poolsizeN` in `ShareMemInfo.ini` and/or adjust the table
/// from `sizeof` in IDA.

inline std::uint64_t attachSizeBytes(ShareMemType type, std::uint32_t poolSlots) {
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
