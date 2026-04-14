#include "share_mem_linux.hpp"

#if defined(__linux__)

#include <cerrno>
#include <sys/ipc.h>
#include <sys/shm.h>

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
        return false;
    }

    const key_t ipcKey = static_cast<key_t>(key);

    int id = shmget(ipcKey, totalBytes, 0);
    if (id < 0) {
        attachOpenFailed = true;
        id = shmget(ipcKey, totalBytes, IPC_CREAT | IPC_EXCL | 0666);
        if (id < 0) {
            if (errno == EEXIST) {
                id = shmget(ipcKey, totalBytes, 0);
                createdNew = false;
            }
            if (id < 0) {
                return false;
            }
        } else {
            createdNew = true;
        }
    }

    void* p = shmat(id, nullptr, 0);
    if (p == reinterpret_cast<void*>(-1) || p == nullptr) {
        return false;
    }

    shmid_ = id;
    header_ = p;
    size_ = totalBytes;

    auto* head = static_cast<SMHead*>(p);
    if (createdNew) {
        head->m_Key = key;
        head->m_Size = static_cast<std::uint32_t>(totalBytes);
        head->m_HeadVer = 0;
    } else {
        if (head->m_Key != key || head->m_Size != static_cast<std::uint32_t>(totalBytes)) {
            unmapOnly();
            return false;
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
