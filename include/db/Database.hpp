#pragma once

#include <string>
#include <sqlite3.h>
#include <stdexcept>

namespace db
{

    /// Управление подключением к SQLite и инициализация схемы
    class Database
    {
    public:
        /// Открывает (или создаёт) файл базы данных по пути dbPath
        /// и применяет основные PRAGMA.
        explicit Database(const std::string &dbPath);

        /// Закрывает подключение
        ~Database();

        Database(const Database &) = delete;
        Database &operator=(const Database &) = delete;
        Database(Database &&) = delete;
        Database &operator=(Database &&) = delete;

        /// Возвращает сырый sqlite3* для прямых вызовов, например prepare
        sqlite3 *handle() const noexcept { return db_; }

        /// Выполнить произвольный SQL-скрипт
        void executeScript(const std::string &sql);

        /// Начать транзакцию
        void beginTransaction();

        /// Зафиксировать транзакцию
        void commit();

        /// Откатить транзакцию
        void rollback();

    private:
        sqlite3 *db_;
    };

}
