#pragma once

#include "api/BaseController.hpp"
#include "db/managers/GreenhouseManager.hpp"
#include "entities/Greenhouse.hpp"
#include "utils/Logger.hpp"

/**
 * @class GreenhouseController
 * @brief REST‑контроллер для управления теплицами
 *
 * Предоставляет API‑эндпоинты `/greenhouses` и `/greenhouses/:id`
 * для получения, создания, изменения и удаления сущностей
 * `Greenhouse`.  Все ответы отправляются в формате **JSON**
 * (кроме удаления, которое возвращает простой текст).
 *
 * <b>Аутентификация</b>
 * • Все методы используют JWT‑токен из заголовка
 *   `Authorization: Bearer &lt;token&gt;`.
 * • Методы <tt>create</tt>, <tt>update</tt> и <tt>remove</tt>
 *   дополнительно требуют роли **admin**.
 */
class GreenhouseController : public BaseController
{
public:
    /**
     * @brief Конструктор контроллера
     * @param db         Ссылка на объект базы данных
     * @param jwt_secret Секретный ключ для проверки JWT‑подписей
     */
    GreenhouseController(Database &db, const std::string &jwt_secret)
        : BaseController(db, jwt_secret), greenhouse_manager_(db)
    {
        LOG_DEBUG_SG("GreenhouseController initialized");
    }

    /**
     * @brief Регистрирует маршруты в роутере Pistache
     *
     * | HTTP | Путь                 | Метод‑обработчик |
     * |------|----------------------|------------------|
     * | GET  | /greenhouses         | get_all          |
     * | GET  | /greenhouses/:id     | get_by_id        |
     * | POST | /greenhouses         | create           |
     * | PUT  | /greenhouses/:id     | update           |
     * | DELETE | /greenhouses/:id   | remove           |
     */
    void setup_routes(Pistache::Rest::Router &router) override;
    ~GreenhouseController() = default;

private:
    GreenhouseManager greenhouse_manager_; ///<
    /**
     * @brief Возвращает список всех теплиц
     *
     * **Запрос**
     * ‑ Заголовок `Authorization` с валидным токеном.
     *
     * **Ответ 200 OK (application/json)**
     * ```json
     * [
     *   { "greenhouse_id": 1, "name": "Main GH", "location": "North wing" },
     *   { "greenhouse_id": 2, "name": "Test GH", "location": null }
     * ]
     * ```
     */
    void get_all(const Pistache::Rest::Request &request,
                 Pistache::Http::ResponseWriter response);

    /**
     * @brief Возвращает сведения о конкретной теплице по ID
     *
     * **Запрос**
     * ‑ Параметр `:id` (целое) в URL
     * ‑ Заголовок `Authorization` с валидным токеном.
     *
     * **Ответ 200 OK (application/json)**
     * ```json
     * { "greenhouse_id": 1, "name": "Main GH", "location": "North wing" }
     * ```
     *
     * **Ошибки**
     * ‑ 404 Not Found — теплица не найдена
     * ‑ 401 Unauthorized — токен отсутствует или некорректен
     */
    void get_by_id(const Pistache::Rest::Request &request,
                   Pistache::Http::ResponseWriter response);

    /**
     * @brief Создаёт новую теплицу (role = admin)
     *
     * **Запрос**
     * ‑ Заголовок `Authorization` — токен администратора
     * ‑ Тело JSON:
     * ```json
     * { "name": "Main GH", "location": "North wing" }   // location необязателен
     * ```
     *
     * **Ответ 201 Created (application/json)**
     * ```json
     * { "greenhouse_id": 7, "name": "Main GH", "location": "North wing" }
     * ```
     *
     * **Ошибки**
     * ‑ 400 Bad Request — неверный JSON формат
     * ‑ 403 Forbidden — пользователь не администратор
     */
    void create(const Pistache::Rest::Request &request,
                Pistache::Http::ResponseWriter response);

    /**
     * @brief Обновляет существующую теплицу по ID (role = admin)
     *
     * **Запрос**
     * ‑ URL‑параметр `:id`
     * ‑ Заголовок `Authorization` — токен администратора
     * ‑ Тело JSON:
     * ```json
     * { "name": "Renamed GH", "location": "South wing" } // location опционален
     * ```
     *
     * **Ответ 200 OK (application/json)**
     * ```json
     * { "greenhouse_id": 1, "name": "Renamed GH", "location": "South wing" }
     * ```
     *
     * **Ошибки**
     * ‑ 404 Not Found — теплица не найдена
     * ‑ 400 Bad Request — неверный JSON формат
     */
    void update(const Pistache::Rest::Request &request,
                Pistache::Http::ResponseWriter response);

    /**
     * @brief Удаляет теплицу по ID (role = admin)
     *
     * **Запрос**
     * ‑ URL‑параметр `:id`
     * ‑ Заголовок `Authorization` — токен администратора
     *
     * **Ответ 200 OK (text/plain)**
     * ```
     * Greenhouse deleted
     * ```
     *
     * **Ошибки**
     * ‑ 404 Not Found — теплица не найдена
     * ‑ 403 Forbidden — пользователь не администратор
     */
    void remove(const Pistache::Rest::Request &request,
                Pistache::Http::ResponseWriter response);
};