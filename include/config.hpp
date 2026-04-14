#pragma once

#include "types.hpp"

#include <string>
#include <unordered_map>
#include <vector>

struct DbConfig {
    std::string dsn;
    std::string user;
    std::string password;
};

struct ServiceConfig {
    std::vector<ShareMemData> shareMemObjects;
    std::unordered_map<std::uint64_t, int> shareMemKeyToServerId;
    DbConfig db;
    std::string sqlDir;
    int currentServerId{0};
    bool enableShareMem{true};
};

class Config {
public:
    bool load(const std::string& filePath);
    const ServiceConfig& data() const { return data_; }

private:
    ServiceConfig data_;
};
