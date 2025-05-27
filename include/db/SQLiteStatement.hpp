#pragma once

#include <sqlite3>
#include <string>
#include <stdexcept>
#include <vector>
#include <type_traits>

class SQLiteStatement
{
public:
    SQLiteStatement(sqlite3 *db, const std::string &sql);
    ~SQLiteStatement();

    SQLiteStatement(const SQLiteStatement &) = delete;
    SQLiteStatement &operator=(SQLiteStatement &&other) noexcept;

    void bind(int index, int value);
    void bind(int index, double value);
    void bind(int index, const std::string &value);
    void bind(int index, const char *value);
    void bindNull(int index);

    bool step();

    void reset();

    int columnInt(int col) const;
    double columnDouble(int col) const;
    std::string columnText(int col) const;
    bool columnBool(int col) const;

private:
    sqlite3_stmt *stmt_ = nullptr;
}