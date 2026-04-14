#pragma once

#include <cstddef>
#include <cstdint>

/// Matches `Common/DB_Struct.h` SMHead layout (32-bit fields; pool sizes fit uint32).
struct SMHead {
    std::uint32_t m_Key{0};
    std::uint32_t m_Size{0};
    std::uint32_t m_HeadVer{0};
};

/// Linux SysV shared memory, same strategy as `Server/SMU/ShareMemAO.cpp` +
/// `Common/ShareMemAPI.cpp` (__LINUX__): OpenShareMem then CreateShareMem if needed
/// (see `SMUManager::Init` when SMPT_SHAREMEM).
class ShareMemLinux {
public:
    ShareMemLinux() = default;
    ~ShareMemLinux();
    ShareMemLinux(const ShareMemLinux&) = delete;
    ShareMemLinux& operator=(const ShareMemLinux&) = delete;

    /// Same order as `SMUManager::Init` + `ShareMemAO`: try open, then create if needed.
    /// `attachOpenFailed` true if first `shmget(...,0)` failed (log "Attach ShareMem Error").
    /// `createdNew` true if this process created the segment (`CreateShareMem Ok`).
    bool attachOrCreate(
        std::uint32_t key,
        std::size_t totalBytes,
        bool& attachOpenFailed,
        bool& createdNew
    );

    [[nodiscard]] void* headerPtr() const { return header_; }
    [[nodiscard]] std::size_t byteSize() const { return size_; }

private:
    void unmapOnly();
    int shmid_{-1};
    void* header_{nullptr};
    std::size_t size_{0};
};
