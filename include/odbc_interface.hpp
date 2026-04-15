#pragma once

#include "config.hpp"

#include <sqlext.h>
#include <string>

class OdbcInterface {
public:
    OdbcInterface();
    ~OdbcInterface();

    bool connect(const DbConfig& cfg);
    bool isConnected() const;
    void disconnect();
    bool execute(const std::string& sql);
    bool execute(const std::string& sql, long long& affectedRows);
    bool queryInt64(const std::string& sql, long long& value);
    bool beginTransaction();
    bool commitTransaction();
    bool rollbackTransaction();

private:
    SQLHENV env_{SQL_NULL_HENV};
    SQLHDBC dbc_{SQL_NULL_HDBC};
    bool connected_{false};
    bool inTransaction_{false};
};
