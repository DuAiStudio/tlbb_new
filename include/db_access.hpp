#pragma once

#include "odbc_interface.hpp"

class HumanDbRepo {
public:
    explicit HumanDbRepo(OdbcInterface& db) : db_(db) {}

    bool loadCount(long long& outCount);
    bool saveAll();
    bool saveAllWithRows(long long& affectedRows);
    bool saveNormalWithRows(long long& affectedRows);
    bool saveLogoutFocused();
    bool saveLogoutFocusedWithRows(long long& affectedRows);
    bool saveAllExtra();
    bool saveAllExtraWithRows(long long& affectedRows);
    bool saveNormalExtraWithRows(long long& affectedRows);
    bool saveAllTransactional(long long& affectedMain, long long& affectedExtra);
    bool saveNormalTransactional(long long& affectedMain, long long& affectedExtra);
    bool saveLogoutFocusedTransactional(long long& affectedMain, long long& affectedExtra);

private:
    OdbcInterface& db_;
};

class GuildDbRepo {
public:
    explicit GuildDbRepo(OdbcInterface& db) : db_(db) {}

    bool loadCount(long long& outCount);
    bool saveAll();
    bool saveAllUsers();
    bool saveAllWithRows(long long& affectedRows);
    bool saveAllUsersWithRows(long long& affectedRows);
    bool saveAllTransactional(long long& affectedGuild, long long& affectedUser);
    bool markGeneralSetDirty();

private:
    OdbcInterface& db_;
};

class AuctionDbRepo {
public:
    explicit AuctionDbRepo(OdbcInterface& db) : db_(db) {}

    bool loadCounts(long long& auctionCount, long long& itemCount, long long& petCount);
    bool saveAll();
    bool saveAllUnits();
    bool saveAllWithRows(long long& affectedRows);
    bool saveAllUnitsWithRows(long long& affectedRows);
    bool saveAllTransactional(long long& affectedAuction, long long& affectedItem, long long& affectedPet);

private:
    OdbcInterface& db_;
};

class MailDbRepo {
public:
    explicit MailDbRepo(OdbcInterface& db) : db_(db) {}

    bool loadCount(long long& outCount);
    bool saveAll();
    bool saveAllWithRows(long long& affectedRows);

private:
    OdbcInterface& db_;
};

class TopListDbRepo {
public:
    explicit TopListDbRepo(OdbcInterface& db) : db_(db) {}

    bool loadCount(long long& outCount);
    bool saveAll();
    bool saveAllByType();
    bool saveAllWithRows(long long& affectedRows);
    bool saveAllByTypeWithRows(long long& affectedRows);
    bool saveAllTransactional(long long& affectedAid, long long& affectedType);

private:
    OdbcInterface& db_;
};

class CommissionShopDbRepo {
public:
    explicit CommissionShopDbRepo(OdbcInterface& db) : db_(db) {}

    bool loadCounts(long long& shopCount, long long& itemCount);
    bool saveAll();
    bool saveAllWithRows(long long& affectedRows);
    bool saveAllTransactional(long long& affectedItem, long long& affectedShop);

private:
    OdbcInterface& db_;
};

class ItemSerialDbRepo {
public:
    explicit ItemSerialDbRepo(OdbcInterface& db) : db_(db) {}

    bool loadCount(long long& outCount);
    bool loadMaxSerial(long long& outMaxSerial, int serverId);
    bool ensureServerRow(int serverId, long long smKey, long long& affectedRows);
    bool saveAll();
    bool saveAllWithRows(long long& affectedRows, int serverId);

private:
    OdbcInterface& db_;
};

class CityDbRepo {
public:
    explicit CityDbRepo(OdbcInterface& db) : db_(db) {}

    bool loadCount(long long& outCount);
    bool saveAll();
    bool saveAllWithRows(long long& affectedRows);
    bool saveAllTransactional(long long& affectedCity);
    bool markGeneralSetDirty();

private:
    OdbcInterface& db_;
};

class GlobalDataDbRepo {
public:
    explicit GlobalDataDbRepo(OdbcInterface& db) : db_(db) {}

    bool loadCount(long long& outCount);
    bool saveAll();
    bool saveAllWithRows(long long& affectedRows);
    bool saveAllTransactional(long long& affectedGlobal);

private:
    OdbcInterface& db_;
};

class PlayerShopDbRepo {
public:
    explicit PlayerShopDbRepo(OdbcInterface& db) : db_(db) {}

    bool loadCounts(long long& shopCount, long long& stallCount);
    bool saveAll();
    bool saveAllWithRows(long long& affectedRows);
    bool saveAllTransactional(long long& affectedShop, long long& affectedStall);
    bool markGeneralSetDirty();

private:
    OdbcInterface& db_;
};

class PetProcreateDbRepo {
public:
    explicit PetProcreateDbRepo(OdbcInterface& db) : db_(db) {}

    bool loadCount(long long& outCount);
    bool saveAll();

private:
    OdbcInterface& db_;
};

class GuildLeagueDbRepo {
public:
    explicit GuildLeagueDbRepo(OdbcInterface& db) : db_(db) {}

    bool loadCounts(long long& leagueCount, long long& userCount);
    bool saveAll();

private:
    OdbcInterface& db_;
};
