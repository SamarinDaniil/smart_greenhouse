#ifndef USERCONTROLLER_HPP
#define USERCONTROLLER_HPP

#include "api/BaseController.hpp"
#include "db/managers/UserManager.hpp"
#include <pistache/router.h>
#include <pistache/http.h>

/**
 * @brief Контроллер для обработки пользовательских операций.
 *
 * Обеспечивает маршруты для аутентификации и управления пользователями.
 * Все методы, кроме /login, требуют наличия токена администратора.
 */
class UserController : public BaseController
{
public:
    /**
     * @brief Конструктор контроллера пользователей.
     * @param db Ссылка на объект базы данных.
     * @param jwt_secret Секретный ключ для генерации JWT-токенов.
     */
    UserController(Database& db, const std::string& jwt_secret)
        : BaseController(db, jwt_secret), user_manager_(db) {}

    /**
     * @brief Настраивает HTTP маршруты для пользователя.
     * @param router Роутер Pistache, в который добавляются маршруты.
     */
    void setup_routes(Pistache::Rest::Router& router) override;
    /// @brief 
    ~UserController() = default;
private:
    UserManager user_manager_;

    /**
     * @brief Регистрирует нового пользователя.
     *
     * POST /register
     *
     * Требует: JSON-объект с полями:
     * - username (string)
     * - password (string)
     * - role (string)
     *
     * Ответ:
     * - 201 Created: Успешно создан пользователь
     * - 400 Bad Request: Невалидный JSON или отсутствующие поля
     * - 401 Unauthorized: Невалидный JWT-токен
     * - 403 Forbidden: Недостаточно прав (не администратор)
     * - 500 Internal Server Error: Ошибка создания пользователя
     *
     * @param request HTTP-запрос.
     * @param response HTTP-ответ.
     */
    void register_user(const Pistache::Rest::Request& request,
                       Pistache::Http::ResponseWriter response);

    /**
     * @brief Получает список всех пользователей.
     *
     * GET /users
     *
     * Ответ:
     * - 200 OK: Массив пользователей (см. get_user)
     * - 401 Unauthorized: Невалидный JWT-токен
     * - 403 Forbidden: Недостаточно прав (не администратор)
     *
     * @param request HTTP-запрос.
     * @param response HTTP-ответ.
     */
    void get_all_users(const Pistache::Rest::Request& request,
                       Pistache::Http::ResponseWriter response);

    /**
     * @brief Получает данные пользователя по ID.
     *
     * GET /users/:id
     *
     * Ответ:
     * - 200 OK: JSON-объект с полями:
     *   - user_id (int)
     *   - username (string)
     *   - role (string)
     *   - created_at (string, ISO 8601 дата)
     * - 400 Bad Request: Невалидный ID пользователя
     * - 401 Unauthorized: Невалидный JWT-токен
     * - 403 Forbidden: Недостаточно прав (не администратор)
     * - 404 Not Found: Пользователь не найден
     *
     * @param request HTTP-запрос.
     * @param response HTTP-ответ.
     */
    void get_user(const Pistache::Rest::Request& request,
                  Pistache::Http::ResponseWriter response);

    /**
     * @brief Обновляет данные пользователя (в настоящее время — только роль).
     *
     * PUT /users/:id
     *
     * Принимает: JSON-объект с полем:
     * - role (string)
     *
     * Ответ:
     * - 204 No Content: Успешное обновление
     * - 400 Bad Request: Невалидный ID или JSON
     * - 401 Unauthorized: Невалидный JWT-токен
     * - 403 Forbidden: Недостаточно прав (не администратор)
     * - 404 Not Found: Пользователь не найден
     * - 500 Internal Server Error: Ошибка обновления роли
     *
     * @param request HTTP-запрос.
     * @param response HTTP-ответ.
     */
    void update_user(const Pistache::Rest::Request& request,
                     Pistache::Http::ResponseWriter response);
};

#endif // USERCONTROLLER_HPP