#include "db/SQLiteStatement.hpp"

SQLiteStatement::SQLiteStatement(sqlite3* db, const std::string& sql) {
    int rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt_, nullptr);
    if (rc != SQLITE_OK) {
        throw std::runtime_error("SQLite prepare failed: " + std::string(sqlite3_errmsg(db)));
    }
}

SQLiteStatement::~SQLiteStatement() {
    if (stmt_) {
        sqlite3_finalize(stmt_);
        stmt_ = nullptr;
    }
}

SQLiteStatement::SQLiteStatement(SQLiteStatement&& other) noexcept {
    stmt_ = other.stmt_;
    other.stmt_ = nullptr;
}

SQLiteStatement& SQLiteStatement::operator=(SQLiteStatement&& other) noexcept {
    if (this != &other) {
        if (stmt_) sqlite3_finalize(stmt_);
        stmt_ = other.stmt_;
        other.stmt_ = nullptr;
    }
    return *this;
}

void SQLiteStatement::bind(int index, int value) {
    if (sqlite3_bind_int(stmt_, index, value) != SQLITE_OK)
        throw std::runtime_error("SQLite bind int failed");
}

void SQLiteStatement::bind(int index, double value) {
    if (sqlite3_bind_double(stmt_, index, value) != SQLITE_OK)
        throw std::runtime_error("SQLite bind double failed");
}

void SQLiteStatement::bind(int index, const std::string& value) {
    if (sqlite3_bind_text(stmt_, index, value.c_str(), -1, SQLITE_TRANSIENT) != SQLITE_OK)
        throw std::runtime_error("SQLite bind text failed");
}

void SQLiteStatement::bind(int index, const char* value) {
    if (sqlite3_bind_text(stmt_, index, value, -1, SQLITE_TRANSIENT) != SQLITE_OK)
        throw std::runtime_error("SQLite bind text failed");
}

void SQLiteStatement::bindNull(int index) {
    if (sqlite3_bind_null(stmt_, index) != SQLITE_OK)
        throw std::runtime_error("SQLite bind null failed");
}

bool SQLiteStatement::step() {
    int rc = sqlite3_step(stmt_);
    if (rc == SQLITE_ROW) {
        return true;
    }
    if (rc == SQLITE_DONE) {
        return false;
    }
    throw std::runtime_error("SQLite step failed: " + std::string(sqlite3_errmsg(sqlite3_db_handle(stmt_))));
}

void SQLiteStatement::reset() {
    if (sqlite3_reset(stmt_) != SQLITE_OK)
        throw std::runtime_error("SQLite reset failed");
    if (sqlite3_clear_bindings(stmt_) != SQLITE_OK)
        throw std::runtime_error("SQLite clear bindings failed");
}

int SQLiteStatement::columnInt(int col) const {
    return sqlite3_column_int(stmt_, col);
}

double SQLiteStatement::columnDouble(int col) const {
    return sqlite3_column_double(stmt_, col);
}

std::string SQLiteStatement::columnText(int col) const {
    const unsigned char* text = sqlite3_column_text(stmt_, col);
    return text ? reinterpret_cast<const char*>(text) : std::string{};
}

bool SQLiteStatement::columnBool(int col) const {
    return sqlite3_column_int(stmt_, col) != 0;
}
