#pragma once

#include "odbc_interface.hpp"

class HumanDbRepo {
public:
    explicit HumanDbRepo(OdbcInterface& db) : db_(db) {}

    bool loadCount(long long& outCount);
    bool saveAll();
    bool saveLogoutFocused();
    bool saveAllExtra();

private:
    OdbcInterface& db_;
};

class GuildDbRepo {
public:
    explicit GuildDbRepo(OdbcInterface& db) : db_(db) {}

    bool loadCount(long long& outCount);
    bool saveAll();
    bool saveAllUsers();
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

private:
    OdbcInterface& db_;
};

class MailDbRepo {
public:
    explicit MailDbRepo(OdbcInterface& db) : db_(db) {}

    bool loadCount(long long& outCount);
    bool saveAll();

private:
    OdbcInterface& db_;
};

class TopListDbRepo {
public:
    explicit TopListDbRepo(OdbcInterface& db) : db_(db) {}

    bool loadCount(long long& outCount);
    bool saveAll();
    bool saveAllByType();

private:
    OdbcInterface& db_;
};

class CommissionShopDbRepo {
public:
    explicit CommissionShopDbRepo(OdbcInterface& db) : db_(db) {}

    bool loadCounts(long long& shopCount, long long& itemCount);
    bool saveAll();

private:
    OdbcInterface& db_;
};

class ItemSerialDbRepo {
public:
    explicit ItemSerialDbRepo(OdbcInterface& db) : db_(db) {}

    bool loadCount(long long& outCount);
    bool loadMaxSerial(long long& outMaxSerial);
    bool saveAll();

private:
    OdbcInterface& db_;
};

class CityDbRepo {
public:
    explicit CityDbRepo(OdbcInterface& db) : db_(db) {}

    bool loadCount(long long& outCount);
    bool saveAll();
    bool markGeneralSetDirty();

private:
    OdbcInterface& db_;
};

class GlobalDataDbRepo {
public:
    explicit GlobalDataDbRepo(OdbcInterface& db) : db_(db) {}

    bool loadCount(long long& outCount);
    bool saveAll();

private:
    OdbcInterface& db_;
};

class PlayerShopDbRepo {
public:
    explicit PlayerShopDbRepo(OdbcInterface& db) : db_(db) {}

    bool loadCounts(long long& shopCount, long long& stallCount);
    bool saveAll();
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
