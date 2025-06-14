#pragma once
#include "entities/Greenhouse.hpp"
#include "db/Database.hpp"
#include "utils/Logger.hpp"

#include <optional>
#include <vector>

namespace db
{
    /**
     * @class GreenhouseManager
     * @brief Управляет операциями с теплицами в базе данных.
     *
     * Класс предоставляет методы для создания, обновления, удаления и извлечения
     * информации о теплицах из базы данных SQLite. Использует RAII-обертку для
     * автоматического освобождения ресурсов при работе с SQL-запросами.
     */
    class GreenhouseManager
    {
    public:
        /**
         * @brief Конструктор с передачей ссылки на базу данных.
         *
         * @param db Ссылка на объект базы данных, который будет использоваться для операций.
         * @pre База данных должна быть корректно инициализирована и открыта.
         */
        explicit GreenhouseManager(Database &db) : db_(db) {}

        /**
         * @brief Создает новую запись о теплице в базе данных.
         *
         * @param greenhouse Объект теплицы, содержащий имя и местоположение.
         *                   Поле gh_id будет заполнено после успешного создания.
         * @return true, если операция успешна, иначе false.
         * @post В случае успеха объект greenhouse будет дополнен gh_id и временными метками.
         */
        bool create(Greenhouse &greenhouse);

        /**
         * @brief Обновляет существующую запись о теплице в базе данных.
         *
         * @param greenhouse Объект теплицы с новыми данными (имя, местоположение, gh_id).
         * @return true, если операция успешна, иначе false.
         * @pre gh_id должен соответствовать существующей записи в базе данных.
         */
        bool update(const Greenhouse &greenhouse);

        /**
         * @brief Удаляет запись о теплице из базы данных по ее идентификатору.
         *
         * @param gh_id Идентификатор теплицы.
         * @return true, если операция успешна, иначе false.
         * @pre gh_id должен существовать в базе данных.
         */
        bool remove(int gh_id);

        /**
         * @brief Получает данные о теплице по ее идентификатору.
         *
         * @param gh_id Идентификатор теплицы.
         * @return std::optional<Greenhouse> Объект теплицы, если запись найдена,
         *         иначе std::nullopt.
         * @details Возвращаемый объект содержит все поля: gh_id, name, location,
         *          created_at, updated_at.
         */
        std::optional<Greenhouse> get_by_id(int gh_id);

        /**
         * @brief Получает список всех теплиц из базы данных.
         *
         * @return std::vector<Greenhouse> Вектор объектов теплиц.
         * @details Каждый объект содержит все поля: gh_id, name, location,
         *          created_at, updated_at. При ошибке возвращает пустой вектор.
         */
        std::vector<Greenhouse> get_all();

    private:
        Database &db_; ///< Ссылка на объект базы данных
    };
}
