#include "db_access.hpp"

namespace {
bool saveGeneralSet(OdbcInterface& db, const char* key, int value) {
    // C1: align with tlbbdb.sql procedure signature: save_general_set(psKey, pnVal).
    const std::string callSql =
        "CALL save_general_set('" + std::string(key) + "', " + std::to_string(value) + ")";
    if (db.execute(callSql)) {
        return true;
    }

    // Fallback for environments where stored procedures are not deployed.
    return db.execute(
        "INSERT INTO t_general_set (sKey, nVal) VALUES ('" + std::string(key) + "', " + std::to_string(value) +
        ") ON DUPLICATE KEY UPDATE nVal = IF(nVal = 0, VALUES(nVal), nVal)");
}
}  // namespace

bool HumanDbRepo::loadCount(long long& outCount) {
    return db_.queryInt64("SELECT COUNT(*) FROM t_char", outCount);
}

bool HumanDbRepo::saveAll() {
    return db_.execute(
        "UPDATE t_char "
        "SET savetime=UNIX_TIMESTAMP() "
        "WHERE isvalid=1");
}

bool HumanDbRepo::saveAllWithRows(long long& affectedRows) {
    return db_.execute(
        "UPDATE t_char "
        "SET savetime=UNIX_TIMESTAMP() "
        "WHERE isvalid=1",
        affectedRows);
}

bool HumanDbRepo::saveNormalWithRows(long long& affectedRows) {
    return db_.execute(
        "UPDATE t_char "
        "SET savetime=UNIX_TIMESTAMP() "
        "WHERE isvalid=1 "
        "ORDER BY logouttime DESC "
        "LIMIT 2000",
        affectedRows);
}

bool HumanDbRepo::saveLogoutFocused() {
    return db_.execute(
        "UPDATE t_char SET savetime=UNIX_TIMESTAMP() "
        "WHERE isvalid=1 AND logouttime>0 AND (savetime=0 OR savetime<logouttime) "
        "LIMIT 2000");
}

bool HumanDbRepo::saveLogoutFocusedWithRows(long long& affectedRows) {
    return db_.execute(
        "UPDATE t_char SET savetime=UNIX_TIMESTAMP() "
        "WHERE isvalid=1 AND logouttime>0 AND (savetime=0 OR savetime<logouttime) "
        "LIMIT 2000",
        affectedRows);
}

bool HumanDbRepo::saveAllExtra() {
    return db_.execute(
        "UPDATE t_charextra "
        "SET dbversion=dbversion+1 "
        "WHERE charguid IN ("
        "  SELECT charguid FROM t_char WHERE isvalid=1"
        ")");
}

bool HumanDbRepo::saveAllExtraWithRows(long long& affectedRows) {
    return db_.execute(
        "UPDATE t_charextra "
        "SET dbversion=dbversion+1 "
        "WHERE charguid IN ("
        "  SELECT charguid FROM t_char WHERE isvalid=1"
        ")",
        affectedRows);
}

bool HumanDbRepo::saveNormalExtraWithRows(long long& affectedRows) {
    return db_.execute(
        "UPDATE t_charextra "
        "SET dbversion=dbversion+1 "
        "WHERE charguid IN ("
        "  SELECT charguid FROM t_char WHERE isvalid=1 ORDER BY logouttime DESC LIMIT 2000"
        ")",
        affectedRows);
}

bool HumanDbRepo::saveAllTransactional(long long& affectedMain, long long& affectedExtra) {
    affectedMain = 0;
    affectedExtra = 0;
    if (!db_.beginTransaction()) {
        return false;
    }
    if (!saveAllWithRows(affectedMain)) {
        (void)db_.rollbackTransaction();
        return false;
    }
    if (!saveAllExtraWithRows(affectedExtra)) {
        (void)db_.rollbackTransaction();
        return false;
    }
    if (!db_.commitTransaction()) {
        (void)db_.rollbackTransaction();
        return false;
    }
    return true;
}

bool HumanDbRepo::saveNormalTransactional(long long& affectedMain, long long& affectedExtra) {
    affectedMain = 0;
    affectedExtra = 0;
    if (!db_.beginTransaction()) {
        return false;
    }
    if (!saveNormalWithRows(affectedMain)) {
        (void)db_.rollbackTransaction();
        return false;
    }
    if (!saveNormalExtraWithRows(affectedExtra)) {
        (void)db_.rollbackTransaction();
        return false;
    }
    if (!db_.commitTransaction()) {
        (void)db_.rollbackTransaction();
        return false;
    }
    return true;
}

bool HumanDbRepo::saveLogoutFocusedTransactional(long long& affectedMain, long long& affectedExtra) {
    affectedMain = 0;
    affectedExtra = 0;
    if (!db_.beginTransaction()) {
        return false;
    }
    if (!saveLogoutFocusedWithRows(affectedMain)) {
        (void)db_.rollbackTransaction();
        return false;
    }
    if (!db_.execute(
            "UPDATE t_charextra "
            "SET dbversion=dbversion+1 "
            "WHERE charguid IN ("
            "  SELECT charguid FROM t_char WHERE isvalid=1 AND logouttime>0 "
            "  ORDER BY logouttime DESC LIMIT 2000"
            ")",
            affectedExtra)) {
        (void)db_.rollbackTransaction();
        return false;
    }
    if (!db_.commitTransaction()) {
        (void)db_.rollbackTransaction();
        return false;
    }
    return true;
}

bool GuildDbRepo::loadCount(long long& outCount) {
    return db_.queryInt64("SELECT COUNT(*) FROM t_guild_new", outCount);
}

bool GuildDbRepo::saveAll() {
    return db_.execute("UPDATE t_guild_new SET dataversion=dataversion+1 WHERE guildid>=0");
}

bool GuildDbRepo::saveAllWithRows(long long& affectedRows) {
    return db_.execute("UPDATE t_guild_new SET dataversion=dataversion+1 WHERE guildid>=0", affectedRows);
}

bool GuildDbRepo::saveAllUsers() {
    return db_.execute(
        "UPDATE t_guild_user "
        "SET gptime=UNIX_TIMESTAMP() "
        "WHERE guildid IN ("
        "  SELECT guildid FROM t_guild_new WHERE guildid>=0"
        ")");
}

bool GuildDbRepo::saveAllUsersWithRows(long long& affectedRows) {
    return db_.execute(
        "UPDATE t_guild_user "
        "SET gptime=UNIX_TIMESTAMP() "
        "WHERE guildid IN ("
        "  SELECT guildid FROM t_guild_new WHERE guildid>=0"
        ")",
        affectedRows);
}

bool GuildDbRepo::saveAllTransactional(long long& affectedGuild, long long& affectedUser) {
    affectedGuild = 0;
    affectedUser = 0;
    if (!db_.beginTransaction()) {
        return false;
    }
    if (!db_.execute("UPDATE t_guild_new SET dataversion=dataversion+1 WHERE guildid>=0", affectedGuild)) {
        (void)db_.rollbackTransaction();
        return false;
    }
    if (!db_.execute(
            "UPDATE t_guild_user "
            "SET gptime=UNIX_TIMESTAMP() "
            "WHERE guildid IN ("
            "  SELECT guildid FROM t_guild_new WHERE guildid>=0"
            ")",
            affectedUser)) {
        (void)db_.rollbackTransaction();
        return false;
    }
    if (!db_.commitTransaction()) {
        (void)db_.rollbackTransaction();
        return false;
    }
    return true;
}

bool GuildDbRepo::markGeneralSetDirty() {
    return saveGeneralSet(db_, "GUILD_NEW", 1);
}

bool AuctionDbRepo::loadCounts(long long& auctionCount, long long& itemCount, long long& petCount) {
    return db_.queryInt64("SELECT COUNT(*) FROM t_auction", auctionCount) &&
           db_.queryInt64("SELECT COUNT(*) FROM t_auction_item", itemCount) &&
           db_.queryInt64("SELECT COUNT(*) FROM t_auction_pet", petCount);
}

bool AuctionDbRepo::saveAll() {
    return db_.execute("UPDATE t_auction SET dtime=dtime WHERE aid>0");
}

bool AuctionDbRepo::saveAllWithRows(long long& affectedRows) {
    return db_.execute("UPDATE t_auction SET dtime=dtime WHERE aid>0", affectedRows);
}

bool AuctionDbRepo::saveAllUnits() {
    return db_.execute(
               "UPDATE t_auction_item "
               "SET Itm_dbversion=Itm_dbversion+1 "
               "WHERE poolid IN ("
               "  SELECT poolid FROM t_auction WHERE aid>0"
               ")") &&
           db_.execute(
               "UPDATE t_auction_pet "
               "SET Pet_dbversion=Pet_dbversion+1 "
               "WHERE poolid IN ("
               "  SELECT poolid FROM t_auction WHERE aid>0"
               ")");
}

bool AuctionDbRepo::saveAllUnitsWithRows(long long& affectedRows) {
    long long r1 = 0;
    long long r2 = 0;
    const bool ok1 = db_.execute(
        "UPDATE t_auction_item "
        "SET Itm_dbversion=Itm_dbversion+1 "
        "WHERE poolid IN ("
        "  SELECT poolid FROM t_auction WHERE aid>0"
        ")",
        r1);
    const bool ok2 = db_.execute(
        "UPDATE t_auction_pet "
        "SET Pet_dbversion=Pet_dbversion+1 "
        "WHERE poolid IN ("
        "  SELECT poolid FROM t_auction WHERE aid>0"
        ")",
        r2);
    affectedRows = (r1 >= 0 ? r1 : 0) + (r2 >= 0 ? r2 : 0);
    return ok1 && ok2;
}

bool AuctionDbRepo::saveAllTransactional(long long& affectedAuction, long long& affectedItem, long long& affectedPet) {
    affectedAuction = 0;
    affectedItem = 0;
    affectedPet = 0;
    if (!db_.beginTransaction()) {
        return false;
    }
    if (!db_.execute("UPDATE t_auction SET dtime=dtime WHERE aid>0", affectedAuction)) {
        (void)db_.rollbackTransaction();
        return false;
    }
    if (!db_.execute(
            "UPDATE t_auction_item "
            "SET Itm_dbversion=Itm_dbversion+1 "
            "WHERE poolid IN ("
            "  SELECT poolid FROM t_auction WHERE aid>0"
            ")",
            affectedItem)) {
        (void)db_.rollbackTransaction();
        return false;
    }
    if (!db_.execute(
            "UPDATE t_auction_pet "
            "SET Pet_dbversion=Pet_dbversion+1 "
            "WHERE poolid IN ("
            "  SELECT poolid FROM t_auction WHERE aid>0"
            ")",
            affectedPet)) {
        (void)db_.rollbackTransaction();
        return false;
    }
    if (!db_.commitTransaction()) {
        (void)db_.rollbackTransaction();
        return false;
    }
    return true;
}

bool MailDbRepo::loadCount(long long& outCount) {
    return db_.queryInt64("SELECT COUNT(*) FROM t_mail", outCount);
}

bool MailDbRepo::saveAll() {
    return db_.execute("UPDATE t_mail SET isvalid=isvalid WHERE aid>0");
}

bool MailDbRepo::saveAllWithRows(long long& affectedRows) {
    return db_.execute("UPDATE t_mail SET isvalid=isvalid WHERE aid>0", affectedRows);
}

bool TopListDbRepo::loadCount(long long& outCount) {
    return db_.queryInt64("SELECT COUNT(*) FROM t_toplist", outCount);
}

bool TopListDbRepo::saveAll() {
    return db_.execute("UPDATE t_toplist SET keyvalue=keyvalue WHERE aid>0");
}

bool TopListDbRepo::saveAllWithRows(long long& affectedRows) {
    return db_.execute("UPDATE t_toplist SET keyvalue=keyvalue WHERE aid>0", affectedRows);
}

bool TopListDbRepo::saveAllByType() {
    return db_.execute(
               "UPDATE t_toplist SET keyvalue=keyvalue "
               "WHERE type>=0");
}

bool TopListDbRepo::saveAllByTypeWithRows(long long& affectedRows) {
    return db_.execute(
        "UPDATE t_toplist SET keyvalue=keyvalue "
        "WHERE type>=0",
        affectedRows);
}

bool TopListDbRepo::saveAllTransactional(long long& affectedAid, long long& affectedType) {
    affectedAid = 0;
    affectedType = 0;
    if (!db_.beginTransaction()) {
        return false;
    }
    if (!db_.execute("UPDATE t_toplist SET keyvalue=keyvalue WHERE aid>0", affectedAid)) {
        (void)db_.rollbackTransaction();
        return false;
    }
    if (!db_.execute(
            "UPDATE t_toplist SET keyvalue=keyvalue "
            "WHERE type>=0",
            affectedType)) {
        (void)db_.rollbackTransaction();
        return false;
    }
    if (!db_.commitTransaction()) {
        (void)db_.rollbackTransaction();
        return false;
    }
    return true;
}

bool CommissionShopDbRepo::loadCounts(long long& shopCount, long long& itemCount) {
    return db_.queryInt64("SELECT COUNT(*) FROM t_cshop", shopCount) &&
           db_.queryInt64("SELECT COUNT(*) FROM t_cshopitem", itemCount);
}

bool CommissionShopDbRepo::saveAll() {
    return db_.execute("UPDATE t_cshopitem SET serial=serial WHERE aid>0") &&
           db_.execute("UPDATE t_cshop SET isvalid=isvalid WHERE aid>0");
}

bool CommissionShopDbRepo::saveAllWithRows(long long& affectedRows) {
    long long r1 = 0;
    long long r2 = 0;
    const bool ok1 = db_.execute("UPDATE t_cshopitem SET serial=serial WHERE aid>0", r1);
    const bool ok2 = db_.execute("UPDATE t_cshop SET isvalid=isvalid WHERE aid>0", r2);
    affectedRows = (r1 >= 0 ? r1 : 0) + (r2 >= 0 ? r2 : 0);
    return ok1 && ok2;
}

bool CommissionShopDbRepo::saveAllTransactional(long long& affectedItem, long long& affectedShop) {
    affectedItem = 0;
    affectedShop = 0;
    if (!db_.beginTransaction()) {
        return false;
    }
    if (!db_.execute("UPDATE t_cshopitem SET serial=serial WHERE aid>0", affectedItem)) {
        (void)db_.rollbackTransaction();
        return false;
    }
    if (!db_.execute("UPDATE t_cshop SET isvalid=isvalid WHERE aid>0", affectedShop)) {
        (void)db_.rollbackTransaction();
        return false;
    }
    if (!db_.commitTransaction()) {
        (void)db_.rollbackTransaction();
        return false;
    }
    return true;
}

bool ItemSerialDbRepo::loadCount(long long& outCount) {
    return db_.queryInt64("SELECT COUNT(*) FROM t_itemkey", outCount);
}

bool ItemSerialDbRepo::loadMaxSerial(long long& outMaxSerial, int serverId) {
    return db_.queryInt64(
        "SELECT COALESCE(MAX(serial),0) FROM t_itemkey WHERE sid=" + std::to_string(serverId),
        outMaxSerial);
}

bool ItemSerialDbRepo::ensureServerRow(int serverId, long long smKey, long long& affectedRows) {
    const std::string sql =
        "INSERT INTO t_itemkey (sid, smkey, serial) "
        "SELECT " + std::to_string(serverId) + "," + std::to_string(smKey) + ",0 "
        "WHERE NOT EXISTS (SELECT 1 FROM t_itemkey WHERE sid=" + std::to_string(serverId) + ")";
    return db_.execute(sql, affectedRows);
}

bool ItemSerialDbRepo::saveAll() {
    return db_.execute("UPDATE t_itemkey SET serial=serial+1 WHERE aid>0");
}

bool ItemSerialDbRepo::saveAllWithRows(long long& affectedRows, int serverId) {
    const std::string sql =
        "UPDATE t_itemkey "
        "SET serial=serial+1 "
        "WHERE aid=("
        "  SELECT aid2 FROM ("
        "    SELECT aid AS aid2 FROM t_itemkey WHERE sid=" + std::to_string(serverId) + " ORDER BY aid DESC LIMIT 1"
        "  ) AS t"
        ")";
    return db_.execute(sql, affectedRows);
}

bool CityDbRepo::loadCount(long long& outCount) {
    return db_.queryInt64("SELECT COUNT(*) FROM t_city_new", outCount);
}

bool CityDbRepo::saveAll() {
    return db_.execute("UPDATE t_city_new SET smoneyfix=smoneyfix WHERE poolid>=0");
}

bool CityDbRepo::saveAllWithRows(long long& affectedRows) {
    return db_.execute("UPDATE t_city_new SET smoneyfix=smoneyfix WHERE poolid>=0", affectedRows);
}

bool CityDbRepo::saveAllTransactional(long long& affectedCity) {
    affectedCity = 0;
    if (!db_.beginTransaction()) {
        return false;
    }
    if (!db_.execute("UPDATE t_city_new SET smoneyfix=smoneyfix WHERE poolid>=0", affectedCity)) {
        (void)db_.rollbackTransaction();
        return false;
    }
    if (!db_.commitTransaction()) {
        (void)db_.rollbackTransaction();
        return false;
    }
    return true;
}

bool CityDbRepo::markGeneralSetDirty() {
    return saveGeneralSet(db_, "CITY_NEW", 1);
}

bool GlobalDataDbRepo::loadCount(long long& outCount) {
    return db_.queryInt64("SELECT COUNT(*) FROM t_global", outCount);
}

bool GlobalDataDbRepo::saveAll() {
    return db_.execute("UPDATE t_global SET data1=data1 WHERE poolid>=0");
}

bool GlobalDataDbRepo::saveAllWithRows(long long& affectedRows) {
    return db_.execute("UPDATE t_global SET data1=data1 WHERE poolid>=0", affectedRows);
}

bool GlobalDataDbRepo::saveAllTransactional(long long& affectedGlobal) {
    affectedGlobal = 0;
    if (!db_.beginTransaction()) {
        return false;
    }
    if (!db_.execute("UPDATE t_global SET data1=data1 WHERE poolid>=0", affectedGlobal)) {
        (void)db_.rollbackTransaction();
        return false;
    }
    if (!db_.commitTransaction()) {
        (void)db_.rollbackTransaction();
        return false;
    }
    return true;
}

bool PlayerShopDbRepo::loadCounts(long long& shopCount, long long& stallCount) {
    return db_.queryInt64("SELECT COUNT(*) FROM t_pshop_new", shopCount) &&
           db_.queryInt64("SELECT COUNT(*) FROM t_pshop_stall", stallCount);
}

bool PlayerShopDbRepo::saveAll() {
    return db_.execute("UPDATE t_pshop_new SET dataversion=dataversion+1 WHERE aid>0") &&
           db_.execute("UPDATE t_pshop_stall SET Box_VldNum=Box_VldNum WHERE aid>0");
}

bool PlayerShopDbRepo::saveAllWithRows(long long& affectedRows) {
    long long r1 = 0;
    long long r2 = 0;
    const bool ok1 = db_.execute("UPDATE t_pshop_new SET dataversion=dataversion+1 WHERE aid>0", r1);
    const bool ok2 = db_.execute("UPDATE t_pshop_stall SET Box_VldNum=Box_VldNum WHERE aid>0", r2);
    affectedRows = (r1 >= 0 ? r1 : 0) + (r2 >= 0 ? r2 : 0);
    return ok1 && ok2;
}

bool PlayerShopDbRepo::saveAllTransactional(long long& affectedShop, long long& affectedStall) {
    affectedShop = 0;
    affectedStall = 0;
    if (!db_.beginTransaction()) {
        return false;
    }
    if (!db_.execute("UPDATE t_pshop_new SET dataversion=dataversion+1 WHERE aid>0", affectedShop)) {
        (void)db_.rollbackTransaction();
        return false;
    }
    if (!db_.execute("UPDATE t_pshop_stall SET Box_VldNum=Box_VldNum WHERE aid>0", affectedStall)) {
        (void)db_.rollbackTransaction();
        return false;
    }
    if (!db_.commitTransaction()) {
        (void)db_.rollbackTransaction();
        return false;
    }
    return true;
}

bool PlayerShopDbRepo::markGeneralSetDirty() {
    return saveGeneralSet(db_, "PSHOP_NEW", 1);
}

bool PetProcreateDbRepo::loadCount(long long& outCount) {
    return db_.queryInt64("SELECT COUNT(*) FROM t_petcreate", outCount);
}

bool PetProcreateDbRepo::saveAll() {
    return db_.execute("UPDATE t_petcreate SET dbversion=dbversion+1 WHERE aid>0");
}

bool GuildLeagueDbRepo::loadCounts(long long& leagueCount, long long& userCount) {
    return db_.queryInt64("SELECT COUNT(*) FROM t_league", leagueCount) &&
           db_.queryInt64("SELECT COUNT(*) FROM t_league_usr", userCount);
}

bool GuildLeagueDbRepo::saveAll() {
    return db_.execute("UPDATE t_league SET applynum=applynum WHERE leagueid>=0") &&
           db_.execute("UPDATE t_league_usr SET jointime=jointime WHERE leagueid>=0");
}
