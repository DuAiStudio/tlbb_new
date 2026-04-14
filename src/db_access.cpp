#include "db_access.hpp"

bool HumanDbRepo::loadCount(long long& outCount) {
    return db_.queryInt64("SELECT COUNT(*) FROM t_char", outCount);
}

bool HumanDbRepo::saveAll() {
    return db_.execute(
        "UPDATE t_char "
        "SET savetime=UNIX_TIMESTAMP() "
        "WHERE isvalid=1");
}

bool HumanDbRepo::saveLogoutFocused() {
    return db_.execute(
        "UPDATE t_char SET savetime=UNIX_TIMESTAMP() "
        "WHERE isvalid=1 AND logouttime>0 AND (savetime=0 OR savetime<logouttime) "
        "LIMIT 2000");
}

bool HumanDbRepo::saveAllExtra() {
    return db_.execute(
        "UPDATE t_charextra "
        "SET dbversion=dbversion+1 "
        "WHERE charguid IN ("
        "  SELECT charguid FROM t_char WHERE isvalid=1"
        ")");
}

bool GuildDbRepo::loadCount(long long& outCount) {
    return db_.queryInt64("SELECT COUNT(*) FROM t_guild_new", outCount);
}

bool GuildDbRepo::saveAll() {
    return db_.execute("UPDATE t_guild_new SET dataversion=dataversion+1 WHERE guildid>=0");
}

bool GuildDbRepo::saveAllUsers() {
    return db_.execute(
        "UPDATE t_guild_user "
        "SET gptime=UNIX_TIMESTAMP() "
        "WHERE guildid IN ("
        "  SELECT guildid FROM t_guild_new WHERE guildid>=0"
        ")");
}

bool GuildDbRepo::markGeneralSetDirty() {
    // t_general_set(sKey,nVal); matches tlbbdb.sql + procedure save_general_set(psKey,pnVal).
    return db_.execute(
        "INSERT INTO t_general_set (sKey, nVal) VALUES ('GUILD_NEW', 1) "
        "ON DUPLICATE KEY UPDATE nVal = IF(nVal = 0, 1, nVal)");
}

bool AuctionDbRepo::loadCounts(long long& auctionCount, long long& itemCount, long long& petCount) {
    return db_.queryInt64("SELECT COUNT(*) FROM t_auction", auctionCount) &&
           db_.queryInt64("SELECT COUNT(*) FROM t_auction_item", itemCount) &&
           db_.queryInt64("SELECT COUNT(*) FROM t_auction_pet", petCount);
}

bool AuctionDbRepo::saveAll() {
    return db_.execute("UPDATE t_auction SET dtime=dtime WHERE aid>0");
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

bool MailDbRepo::loadCount(long long& outCount) {
    return db_.queryInt64("SELECT COUNT(*) FROM t_mail", outCount);
}

bool MailDbRepo::saveAll() {
    return db_.execute("UPDATE t_mail SET isvalid=isvalid WHERE aid>0");
}

bool TopListDbRepo::loadCount(long long& outCount) {
    return db_.queryInt64("SELECT COUNT(*) FROM t_toplist", outCount);
}

bool TopListDbRepo::saveAll() {
    return db_.execute("UPDATE t_toplist SET keyvalue=keyvalue WHERE aid>0");
}

bool TopListDbRepo::saveAllByType() {
    return db_.execute(
               "UPDATE t_toplist SET keyvalue=keyvalue "
               "WHERE type>=0");
}

bool CommissionShopDbRepo::loadCounts(long long& shopCount, long long& itemCount) {
    return db_.queryInt64("SELECT COUNT(*) FROM t_cshop", shopCount) &&
           db_.queryInt64("SELECT COUNT(*) FROM t_cshopitem", itemCount);
}

bool CommissionShopDbRepo::saveAll() {
    return db_.execute("UPDATE t_cshopitem SET serial=serial WHERE aid>0") &&
           db_.execute("UPDATE t_cshop SET isvalid=isvalid WHERE aid>0");
}

bool ItemSerialDbRepo::loadCount(long long& outCount) {
    return db_.queryInt64("SELECT COUNT(*) FROM t_itemkey", outCount);
}

bool ItemSerialDbRepo::loadMaxSerial(long long& outMaxSerial) {
    return db_.queryInt64("SELECT COALESCE(MAX(serial),0) FROM t_itemkey", outMaxSerial);
}

bool ItemSerialDbRepo::saveAll() {
    return db_.execute("UPDATE t_itemkey SET serial=serial+1 WHERE aid>0");
}

bool CityDbRepo::loadCount(long long& outCount) {
    return db_.queryInt64("SELECT COUNT(*) FROM t_city_new", outCount);
}

bool CityDbRepo::saveAll() {
    return db_.execute("UPDATE t_city_new SET smoneyfix=smoneyfix WHERE poolid>=0");
}

bool CityDbRepo::markGeneralSetDirty() {
    return db_.execute(
        "INSERT INTO t_general_set (sKey, nVal) VALUES ('CITY_NEW', 1) "
        "ON DUPLICATE KEY UPDATE nVal = IF(nVal = 0, 1, nVal)");
}

bool GlobalDataDbRepo::loadCount(long long& outCount) {
    return db_.queryInt64("SELECT COUNT(*) FROM t_global", outCount);
}

bool GlobalDataDbRepo::saveAll() {
    return db_.execute("UPDATE t_global SET data1=data1 WHERE poolid>=0");
}

bool PlayerShopDbRepo::loadCounts(long long& shopCount, long long& stallCount) {
    return db_.queryInt64("SELECT COUNT(*) FROM t_pshop_new", shopCount) &&
           db_.queryInt64("SELECT COUNT(*) FROM t_pshop_stall", stallCount);
}

bool PlayerShopDbRepo::saveAll() {
    return db_.execute("UPDATE t_pshop_new SET dataversion=dataversion+1 WHERE aid>0") &&
           db_.execute("UPDATE t_pshop_stall SET Box_VldNum=Box_VldNum WHERE aid>0");
}

bool PlayerShopDbRepo::markGeneralSetDirty() {
    return db_.execute(
        "INSERT INTO t_general_set (sKey, nVal) VALUES ('PSHOP_NEW', 1) "
        "ON DUPLICATE KEY UPDATE nVal = IF(nVal = 0, 1, nVal)");
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
