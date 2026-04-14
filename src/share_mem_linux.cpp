#include "share_mem_linux.hpp"

#if defined(__linux__)

#include "logger.hpp"

#include <cerrno>
#include <cstring>
#include <sys/ipc.h>
#include <sys/shm.h>

namespace {

void logShmErr(const char* what, key_t ipcKey, std::size_t totalBytes) {
    const int e = errno;
    Logger::instance().logShare(
        std::string(what) + " key=" + std::to_string(static_cast<long long>(ipcKey)) +
        " size=" + std::to_string(totalBytes) + " errno=" + std::to_string(e) + " (" + std::strerror(e) + ")");
}

}  // namespace

void ShareMemLinux::unmapOnly() {
    if (header_ != nullptr && header_ != reinterpret_cast<void*>(-1)) {
        shmdt(header_);
    }
    header_ = nullptr;
    shmid_ = -1;
    size_ = 0;
}

ShareMemLinux::~ShareMemLinux() {
    unmapOnly();
}

bool ShareMemLinux::attachOrCreate(
    std::uint32_t key,
    std::size_t totalBytes,
    bool& attachOpenFailed,
    bool& createdNew
) {
    attachOpenFailed = false;
    createdNew = false;
    unmapOnly();
    if (totalBytes < sizeof(SMHead)) {
        Logger::instance().logShare("ShareMem totalBytes too small for SMHead");
        return false;
    }

    const key_t ipcKey = static_cast<key_t>(key);

    int id = shmget(ipcKey, totalBytes, 0);
    if (id < 0) {
        attachOpenFailed = true;
        id = shmget(ipcKey, totalBytes, IPC_CREAT | IPC_EXCL | 0666);
        if (id < 0) {
            const int cre = errno;
            if (cre == EEXIST) {
                id = shmget(ipcKey, totalBytes, 0);
                createdNew = false;
            }
            if (id < 0) {
                logShmErr("shmget(create/exist)", ipcKey, totalBytes);
                return false;
            }
        } else {
            createdNew = true;
        }
    }

    void* p = shmat(id, nullptr, 0);
    if (p == reinterpret_cast<void*>(-1) || p == nullptr) {
        logShmErr("shmat", ipcKey, totalBytes);
        return false;
    }

    shmid_ = id;
    header_ = p;

    struct shmid_ds ds {};
    if (shmctl(id, IPC_STAT, &ds) == 0) {
        size_ = static_cast<std::size_t>(ds.shm_segsz);
        if (size_ != totalBytes) {
            Logger::instance().logShare(
                "ShareMem IPC_STAT shm_segsz=" + std::to_string(size_) + " requested_totalBytes=" +
                std::to_string(totalBytes));
        }
    } else {
        size_ = totalBytes;
    }

    auto* head = static_cast<SMHead*>(p);
    const auto key64 = static_cast<std::uint64_t>(key);
    const auto size64 = static_cast<std::uint64_t>(totalBytes);
    if (createdNew) {
        head->m_Key = key64;
        head->m_Size = size64;
        head->m_HeadVer = 0;
    } else {
        // `ShareMemAO::Attach`: Assert m_Key and m_Size match (LP64).
        if (head->m_Key != key64) {
            Logger::instance().logShare(
                "ShareMem SMHead m_Key mismatch: in_shm=" + std::to_string(head->m_Key) + " expected=" +
                std::to_string(key64) + " (check SMHead vs IDA Linux ELF)");
            unmapOnly();
            return false;
        }
        if (head->m_Size != size64) {
            Logger::instance().logShare(
                "ShareMem SMHead m_Size=" + std::to_string(head->m_Size) + " calc=" +
                std::to_string(size64) + " kernel_seg=" + std::to_string(size_) + " (continuing; key ok)");
        }
    }

    return true;
}

#else

ShareMemLinux::~ShareMemLinux() = default;

bool ShareMemLinux::attachOrCreate(
    std::uint32_t,
    std::size_t,
    bool& attachOpenFailed,
    bool& createdNew
) {
    attachOpenFailed = false;
    createdNew = false;
    return true;
}

#endif
