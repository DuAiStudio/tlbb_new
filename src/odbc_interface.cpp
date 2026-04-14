#include "odbc_interface.hpp"

#include <iostream>
#include <string>

OdbcInterface::OdbcInterface() {
    SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &env_);
    SQLSetEnvAttr(env_, SQL_ATTR_ODBC_VERSION, reinterpret_cast<void*>(SQL_OV_ODBC3), 0);
    SQLAllocHandle(SQL_HANDLE_DBC, env_, &dbc_);
}

OdbcInterface::~OdbcInterface() {
    disconnect();
    if (dbc_ != SQL_NULL_HDBC) SQLFreeHandle(SQL_HANDLE_DBC, dbc_);
    if (env_ != SQL_NULL_HENV) SQLFreeHandle(SQL_HANDLE_ENV, env_);
}

bool OdbcInterface::connect(const DbConfig& cfg) {
    if (connected_) return true;

    const std::string connStr = "DSN=" + cfg.dsn + ";UID=" + cfg.user + ";PWD=" + cfg.password + ";";
    SQLCHAR outConn[1024]{};
    SQLSMALLINT outConnLen = 0;

    const auto rc = SQLDriverConnect(
        dbc_,
        nullptr,
        reinterpret_cast<SQLCHAR*>(const_cast<char*>(connStr.c_str())),
        SQL_NTS,
        outConn,
        sizeof(outConn),
        &outConnLen,
        SQL_DRIVER_NOPROMPT);

    connected_ = SQL_SUCCEEDED(rc);
    if (!connected_) {
        std::cerr << "ODBC connect failed for DSN: " << cfg.dsn << '\n';
    }
    return connected_;
}

bool OdbcInterface::isConnected() const {
    return connected_;
}

void OdbcInterface::disconnect() {
    if (!connected_) return;
    SQLDisconnect(dbc_);
    connected_ = false;
}

bool OdbcInterface::execute(const std::string& sql) {
    SQLHSTMT stmt = SQL_NULL_HSTMT;
    if (!SQL_SUCCEEDED(SQLAllocHandle(SQL_HANDLE_STMT, dbc_, &stmt))) {
        return false;
    }
    const auto rc = SQLExecDirect(stmt, reinterpret_cast<SQLCHAR*>(const_cast<char*>(sql.c_str())), SQL_NTS);
    SQLFreeHandle(SQL_HANDLE_STMT, stmt);
    return SQL_SUCCEEDED(rc);
}

bool OdbcInterface::queryInt64(const std::string& sql, long long& value) {
    value = 0;
    SQLHSTMT stmt = SQL_NULL_HSTMT;
    if (!SQL_SUCCEEDED(SQLAllocHandle(SQL_HANDLE_STMT, dbc_, &stmt))) {
        return false;
    }

    const auto execRc = SQLExecDirect(stmt, reinterpret_cast<SQLCHAR*>(const_cast<char*>(sql.c_str())), SQL_NTS);
    if (!SQL_SUCCEEDED(execRc)) {
        SQLFreeHandle(SQL_HANDLE_STMT, stmt);
        return false;
    }

    if (!SQL_SUCCEEDED(SQLFetch(stmt))) {
        SQLFreeHandle(SQL_HANDLE_STMT, stmt);
        return false;
    }

    SQLLEN ind = 0;
    const auto getRc = SQLGetData(stmt, 1, SQL_C_SBIGINT, &value, sizeof(value), &ind);
    SQLFreeHandle(SQL_HANDLE_STMT, stmt);
    return SQL_SUCCEEDED(getRc) && ind != SQL_NULL_DATA;
}
