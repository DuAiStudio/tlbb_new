#pragma once

#include <cstddef>
#include <cstdint>

/// B1 / IDA Linux LP64: `tlbb2007/Common/DB_Struct.h` + `BaseType.h` under `__LINUX__` uses
/// `typedef unsigned long ULONG` and `unsigned long m_Size` — on LP64, **unsigned long is 64-bit**.
/// So SMHead is **24 bytes** (8 + 8 + 4 + 4 tail padding), not three 32-bit fields.
/// If your IDA ELF used a different ABI (e.g. ILP32), re-export struct from IDA and adjust here.
struct SMHead {
    std::uint64_t m_Key{0};
    std::uint64_t m_Size{0};
    std::uint32_t m_HeadVer{0};
};

#if defined(__linux__)
static_assert(sizeof(SMHead) == 24U, "SMHead size must match Linux LP64 tlbb2007 layout");
#endif

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
