#pragma once
#include "entities/Component.hpp"
#include "db/Database.hpp"
#include <vector>
#include <optional>

/**
 * @class ComponentManager
 * @brief Менеджер компонентов для работы с таблицей components в базе данных
 *
 * Предоставляет методы для выполнения CRUD операций и выполнения запросов к таблице components.
 */
class ComponentManager
{
public:
    /**
     * @brief Конструктор
     * @param db Ссылка на объект базы данных
     */
    explicit ComponentManager(Database &db) : db_(db) {}

    // Основные CRUD операции

    /**
     * @brief Создает новый компонент в базе данных
     * @param component Объект Component с заполненными полями gh_id, name, role, subtype
     * @return true, если компонент успешно создан, иначе false
     * @note После успешного создания поле comp_id у объекта component будет заполнено
     */
    bool create(Component &component);

    /**
     * @brief Обновляет существующий компонент в базе данных
     * @param component Объект Component с заполненными полями для обновления
     * @return true, если компонент успешно обновлен, иначе false
     */
    bool update(const Component &component);

    /**
     * @brief Удаляет компонент из базы данных
     * @param comp_id Идентификатор компонента
     * @return true, если компонент успешно удален, иначе false
     */
    bool remove(int comp_id);

    /**
     * @brief Получает все компоненты из базы данных
     *
     * Выполняет SQL-запрос без фильтрации по теплице и возвращает
     * полный список записей из таблицы components.
     *
     * @return std::vector<Component> Вектор всех компонентов.
     */
    std::vector<Component> get_all();

    /**
     * @brief Получает компонент по его идентификатору
     * @param comp_id Идентификатор компонента
     * @return std::optional<Component> содержащий данные компонента или std::nullopt, если компонент не найден
     */
    std::optional<Component> get_by_id(int comp_id);

    // Получение компонентов по различным критериям

    /**
     * @brief Получает все компоненты для заданной теплицы
     * @param gh_id Идентификатор теплицы
     * @return Вектор компонентов, связанных с указанной теплицей
     */
    std::vector<Component> get_by_greenhouse(int gh_id);

    /**
     * @brief Получает все компоненты с заданной ролью
     * @param role Роль компонента
     * @return Вектор компонентов с указанной ролью
     */
    std::vector<Component> get_by_role(const std::string &role);

    /**
     * @brief Получает все компоненты с заданным подтипом
     * @param subtype Подтип компонента
     * @return Вектор компонентов с указанным подтипом
     */
    std::vector<Component> get_by_subtype(const std::string &subtype);

    /**
     * @brief Получает компоненты для заданной теплицы и роли
     * @param gh_id Идентификатор теплицы
     * @param role Роль компонента
     * @return Вектор компонентов, удовлетворяющих условиям запроса
     */
    std::vector<Component> get_by_greenhouse_and_role(int gh_id, const std::string &role);

    /**
     * @brief Получает компоненты для заданной теплицы и подтипа
     * @param gh_id Идентификатор теплицы
     * @param subtype Подтип компонента
     * @return Вектор компонентов, удовлетворяющих условиям запроса
     */
    std::vector<Component> get_by_greenhouse_and_subtype(int gh_id, const std::string &subtype);

    // Статистика и информация

    /**
     * @brief Подсчитывает количество компонентов в теплице
     * @param gh_id Идентификатор теплицы
     * @return Количество компонентов в указанной теплице
     */
    int count_by_greenhouse(int gh_id);

    /**
     * @brief Подсчитывает количество компонентов с заданной ролью
     * @param role Роль компонента
     * @return Количество компонентов с указанной ролью
     */
    int count_by_role(const std::string &role);

    /**
     * @brief Подсчитывает количество компонентов с заданным подтипом
     * @param subtype Подтип компонента
     * @return Количество компонентов с указанным подтипом
     */
    int count_by_subtype(const std::string &subtype);

private:
    Database &db_; ///< Ссылка на объект базы данных

    /**
     * @brief Парсит данные из SQL-запроса в объект Component
     * @param stmt Указатель на SQL-запрос с результатом
     * @return Заполненный объект Component
     */
    Component parse_component_from_db(sqlite3_stmt *stmt) const;
};