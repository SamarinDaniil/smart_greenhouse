#ifndef USERMANAGER_HPP
#define USERMANAGER_HPP

#include "entities/User.hpp"
#include "db/Database.hpp"

#include <optional>
#include <vector>

/// @brief Менеджер для управления пользователями (CRUD, аутентификация)
class UserManager
{
public:
    /// @brief Конструктор
    /// @param db Ссылка на объект базы данных
    explicit UserManager(Database &db) : db_(db) {}

    /// @brief Создает нового пользователя
    /// @param[in,out] user Объект пользователя (при успехе обновляется ID и датами)
    /// @param[in] password Пароль в открытом виде
    /// @return true при успешном создании, false при ошибке
    bool create(User &user, const std::string &password);

    /// @brief Проверяет аутентификационные данные
    /// @param[in] username Логин пользователя
    /// @param[in] password Пароль в открытом виде
    /// @return true если пароль верный, false если ошибка или пользователь не найден
    bool authenticate(const std::string &username, const std::string &password);

    /// @brief Изменяет роль пользователя
    /// @param[in] user_id ID целевого пользователя
    /// @param[in] new_role Новая роль ('observer' или 'admin')
    /// @return true при успешном обновлении, false при ошибке
    bool update_role(int user_id, const std::string &new_role);

    /// @brief Ищет пользователя по ID
    /// @param[in] user_id ID искомого пользователя
    /// @return Объект User или std::nullopt если не найден
    std::optional<User> get_by_id(int user_id); 
    
    /// @brief Ищет пользователя по логину
    /// @param[in] username Логин искомого пользователя
    /// @return Объект User или std::nullopt если не найден
    std::optional<User> get_by_username(const std::string& username);
    
    /// @brief Получает всех пользователей системы
    /// @return Вектор объектов User (без хешей паролей)
    std::vector<User> get_all();

private:
    Database &db_; ///< Ссылка на менеджер базы данных
};

#endif // USERMANAGER_HPP