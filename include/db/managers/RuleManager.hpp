#ifndef RULEMANAGER_HPP
#define RULEMANAGER_HPP
#include "entities/Rule.hpp"
#include "db/Database.hpp"
#include <vector>
#include <optional>

namespace db
{
    /**
     * @class RuleManager
     * @brief Класс для управления правилами в базе данных
     *
     * Предоставляет методы для CRUD-операций с правилами,
     * управления состоянием правил и поиска по различным критериям
     */
    class RuleManager
    {
    public:
        /**
         * @brief Конструктор с параметром
         * @param db Ссылка на объект базы данных
         */
        explicit RuleManager(Database &db) : db_(db) {}

        // Основные CRUD операции

        /**
         * @brief Создает новое правило в базе данных
         *
         * @param rule Объект правила, который будет сохранен в БД
         *             После успешного выполнения заполняется rule_id
         * @return true, если правило успешно создано
         */
        bool create(Rule &rule);

        /**
         * @brief Обновляет существующее правило в базе данных
         *
         * @param rule Объект правила с новыми данными
         * @return true, если правило успешно обновлено
         */
        bool update(const Rule &rule);

        /**
         * @brief Удаляет правило из базы данных
         *
         * @param rule_id Идентификатор удаляемого правила
         * @return true, если правило успешно удалено
         */
        bool remove(int rule_id);

        /**
         * @brief Получает правило по его ID
         *
         * @param rule_id Идентификатор правила
         * @return Объект Rule в случае успеха, std::nullopt при ошибке
         */
        std::optional<Rule> get_by_id(int rule_id);

        // Получение правил по различным критериям

        /**
         * @brief Получает все правила для конкретной теплицы
         *
         * @param gh_id Идентификатор теплицы
         * @return Вектор правил (может быть пустым)
         */
        std::vector<Rule> get_by_greenhouse(int gh_id);

        /**
         * @brief Получает все активные правила
         *
         * @return Вектор активных правил (может быть пустым)
         */
        std::vector<Rule> get_active_rules();

        /**
         * @brief Получает правила для компонента
         *
         * @param comp_id Идентификатор компонента
         * @param as_source true - как источник, false - как получатель
         * @return Вектор правил (может быть пустым)
         */
        std::vector<Rule> get_rules_for_component(int comp_id, bool as_source = true);

        // Управление состоянием правил

        /**
         * @brief Включает/выключает правило
         *
         * @param rule_id Идентификатор правила
         * @param enabled true - включить, false - выключить
         * @return true, если операция выполнена успешно
         */
        bool toggle_rule(int rule_id, bool enabled);

        /**
         * @brief Проверяет активность правила
         *
         * @param rule_id Идентификатор правила
         * @return true, если правило активно
         */
        bool is_rule_active(int rule_id);

    private:
        Database &db_;

        /**
         * @brief Парсит результат SQL-запроса в объект Rule
         *
         * @param stmt Указатель на SQL-статемент SQLite
         * @return Заполненный объект Rule
         */
        Rule parse_rule_from_db(sqlite3_stmt *stmt) const;
    };
}

#endif // RULEMANAGER_HPP