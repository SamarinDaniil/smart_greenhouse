#pragma once

#include "api/BaseController.hpp"
#include "db/managers/ComponentManager.hpp"
#include "entities/Component.hpp"
#include "db/managers/GreenhouseManager.hpp"
#include "utils/Logger.hpp"

/**
 * @class ComponentController
 * @brief Контроллер для управления компонентами теплицы
 *
 * Обрабатывает запросы CRUD для сущности Component,
 * обеспечивает аутентификацию и проверку прав.
 */
class ComponentController : public BaseController
{
public:
    /**
     * @brief Конструктор ComponentController
     * @param db Ссылка на объект базы данных
     * @param jwt_secret Секретный ключ для JWT-аутентификации
     */
    ComponentController(Database &db,
                        const std::string &jwt_secret)
        : BaseController(db, jwt_secret),
          component_manager_(db),
          greenhouse_manager_(db)
    {
        LOG_INFO_SG("ComponentController initialized");
    }

    /**
     * @brief Настраивает маршруты API для компонентов
     * @param router Объект роутера Pistache
     */
    void setup_routes(Pistache::Rest::Router &router) override;

    ~ComponentController() = default;

private:
    ComponentManager component_manager_;
    GreenhouseManager greenhouse_manager_;

    /**
     * @brief Получает список компонентов по заданным фильтрам
     * @param request HTTP-запрос с опциональными параметрами:
     *   - gh_id (int) — идентификатор теплицы
     *   - role (string) — роль компонента: "sensor" или "actuator"
     *   - subtype (string) — подтип компонента
     * @param response HTTP-ответ со списком компонентов в формате JSON
     *
     * Возможные ответы:
     * - 200 OK: JSON-массив объектов Component
     * - 401 Unauthorized: строка с сообщением об ошибке аутентификации
     * - 400 Bad Request: текст ошибки (например, неверный формат параметра)
     */
    void get_components(const Pistache::Rest::Request &request,
                        Pistache::Http::ResponseWriter response);

    /**
     * @brief Получает компонент по его идентификатору
     * @param request HTTP-запрос с параметром :id
     * @param response HTTP-ответ с объектом Component в формате JSON
     *
     * Возможные ответы:
     * - 200 OK: JSON-объект Component
     * - 401 Unauthorized: строка с сообщением об ошибке аутентификации
     * - 404 Not Found: "Component not found"
     * - 400 Bad Request: текст ошибки (например, неверный формат id)
     */
    void get_by_id(const Pistache::Rest::Request &request,
                   Pistache::Http::ResponseWriter response);

    /**
     * @brief Создаёт новый компонент
     * @param request HTTP-запрос с телом JSON:
     *   {
     *     "gh_id": int,
     *     "name": string,
     *     "role": "sensor" | "actuator",
     *     "subtype": string
     *   }
     * @param response HTTP-ответ с созданным компонентом в формате JSON
     *
     * Возможные ответы:
     * - 201 Created: JSON-объект созданного Component
     * - 401 Unauthorized: строка с сообщением об ошибке аутентификации
     * - 403 Forbidden: строка с сообщением об ошибке прав доступа
     * - 400 Bad Request: "Greenhouse does not exist" или "Invalid role. Must be 'sensor' or 'actuator'" или текст парсинга JSON
     * - 500 Internal Server Error: "Failed to create component"
     */
    void create(const Pistache::Rest::Request &request,
                Pistache::Http::ResponseWriter response);

    /**
     * @brief Обновляет существующий компонент
     * @param request HTTP-запрос с параметром :id и телом JSON с полями для обновления:
     *   {
     *     "name"?: string,
     *     "role"?: "sensor" | "actuator",
     *     "subtype"?: string
     *   }
     * @param response HTTP-ответ с обновлённым компонентом в формате JSON
     *
     * Возможные ответы:
     * - 200 OK: JSON-объект обновлённого Component
     * - 401 Unauthorized: строка с сообщением об ошибке аутентификации
     * - 403 Forbidden: строка с сообщением об ошибке прав доступа
     * - 404 Not Found: "Component not found"
     * - 400 Bad Request: "Invalid role. Must be 'sensor' or 'actuator'" или текст парсинга JSON
     * - 500 Internal Server Error: "Failed to update component"
     */
    void update(const Pistache::Rest::Request &request,
                Pistache::Http::ResponseWriter response);

    /**
     * @brief Удаляет компонент по его идентификатору
     * @param request HTTP-запрос с параметром :id
     * @param response HTTP-ответ с результатом удаления
     *
     * Возможные ответы:
     * - 200 OK: "Component deleted"
     * - 401 Unauthorized: строка с сообщением об ошибке аутентификации
     * - 403 Forbidden: строка с сообщением об ошибке прав доступа
     * - 404 Not Found: "Component not found"
     * - 500 Internal Server Error: "Failed to delete component"
     */
    void remove(const Pistache::Rest::Request &request,
                Pistache::Http::ResponseWriter response);
};