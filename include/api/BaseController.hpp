#pragma once
#include <pistache/endpoint.h>
#include <pistache/router.h>
#include <chrono>
#include "jwt/jwt.hpp"
#include "db/managers/UserManager.hpp"
#include "utils/PasswordHasher.hpp"
#include "utils/Logger.hpp"
#include "entities/User.hpp"
#include <nlohmann/json.hpp>
#include "utils/TimeParserISO8601.hpp"

/**
 * @struct AuthResult
 * @brief Результат аутентификации пользователя
 */
struct AuthResult
{
    bool valid = false;         ///< true, если аутентификация успешна
    int user_id = -1;          ///< ID пользователя (если аутентифицирован)
    std::string role;          ///< Роль пользователя (если аутентифицирован)
    std::string error;         ///< Сообщение об ошибке (если аутентификация не удалась)
    
    /**
     * @brief Проверяет, является ли результат валидным
     */
    bool is_valid() const { return valid && user_id > 0; }
};

/**
 * @class BaseController
 * @brief Базовый класс для обработки аутентификации и маршрутов API
 *
 * Содержит общие методы для работы с JWT-токенами (HS256), аутентификации пользователей
 * и регистрации маршрутов. Предоставляет функционал для проверки прав доступа.
 * Pistache/0.4.25
 * cpp-jwt/1.4.0
 * nlohmann_json/3.11.3
 */
class BaseController
{
public:
    /**
     * @brief Конструктор
     * @param db Ссылка на объект базы данных
     * @param jwt_secret Секретный ключ для подписи JWT-токенов
     */
    BaseController(Database &db, const std::string &jwt_secret)
        : user_manager_(db), jwt_secret_(jwt_secret) {}

    /**
     * @brief Виртуальный деструктор
     */
    virtual ~BaseController() = default;

    /**
     * @brief Регистрирует маршруты API
     * @param router Роутер Pistache
     */
    virtual void setup_routes(Pistache::Rest::Router &router);

protected:
    UserManager user_manager_;
    std::string jwt_secret_;

    /**
     * @brief Обрабатывает запрос на вход пользователя
     * @param request HTTP-запрос с телом {"username": "...", "password": "..."}
     * @param response HTTP-ответ с токеном или сообщением об ошибке
     *
     * Возможные ответы:
     * - 200 OK: {"token": "JWT_TOKEN"}
     * - 401 Unauthorized: "User not found" или "Invalid password"
     * - 400 Bad Request: ошибка парсинга JSON
     */
    void login(const Pistache::Rest::Request &request,
               Pistache::Http::ResponseWriter response);

    /**
     * @brief Генерирует JWT-токен для пользователя
     * @param user_id ID пользователя
     * @param role Роль пользователя
     * @param expires_in_hours Время жизни токена в часах (по умолчанию 24)
     * @return Строка JWT-токена
     */
    std::string generate_token(int user_id, const std::string &role, int expires_in_hours = 24);

    /**
     * @brief Извлекает и проверяет JWT-токен из заголовка Authorization
     * @param request HTTP-запрос с заголовком Authorization: Bearer <token>
     * @return Результат аутентификации (AuthResult)
     */
    AuthResult authenticate_request(const Pistache::Rest::Request &request);

    /**
     * @brief Проверяет, является ли пользователь администратором
     * @param auth Результат аутентификации (AuthResult)
     * @param response HTTP-ответ для отправки ошибки
     * @return true, если пользователь - администратор
     *
     * Возможные ответы:
     * - 401 Unauthorized: ошибка аутентификации
     * - 403 Forbidden: пользователь не является администратором
     */
    bool require_admin_role(const AuthResult &auth,
                           Pistache::Http::ResponseWriter &response);

    /**
     * @brief Проверяет, имеет ли пользователь указанную роль
     * @param auth Результат аутентификации
     * @param required_role Требуемая роль
     * @param response HTTP-ответ для отправки ошибки
     * @return true, если пользователь имеет требуемую роль
     */
    bool require_role(const AuthResult &auth, const std::string &required_role,
                     Pistache::Http::ResponseWriter &response);

    /**
     * @brief Отправляет JSON-ответ
     * @param response HTTP-ответ
     * @param json JSON-объект
     * @param status_code HTTP-статус (по умолчанию 200)
     */
    void send_json_response(Pistache::Http::ResponseWriter &response,
                           const nlohmann::json &json,
                           Pistache::Http::Code status_code = Pistache::Http::Code::Ok);

    /**
     * @brief Отправляет ответ с ошибкой
     * @param response HTTP-ответ
     * @param message Сообщение об ошибке
     * @param status_code HTTP-статус
     */
    void send_error_response(Pistache::Http::ResponseWriter &response,
                            const std::string &message,
                            Pistache::Http::Code status_code);

    /**
     * @brief Парсит JSON из тела запроса
     * @param request HTTP-запрос
     * @param json_out Выходной JSON-объект
     * @return true, если парсинг успешен
     */
    bool parse_json_body(const Pistache::Rest::Request &request, nlohmann::json &json_out);

    /**
     * @brief Извлекает токен из заголовка Authorization
     * @param request HTTP-запрос
     * @return Токен или пустая строка, если не найден
     */
    std::string extract_bearer_token(const Pistache::Rest::Request &request);

    /**
     * @brief Валидирует JWT-токен
     * @param token JWT-токен
     * @return Результат аутентификации
     */
    AuthResult validate_jwt_token(const std::string &token);
};