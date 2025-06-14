#ifndef RULECONTROLLER_HPP
#define RULECONTROLLER_HPP

#include "api/BaseController.hpp"
#include "db/managers/RuleManager.hpp"
#include "entities/Rule.hpp"
#include <pistache/router.h>
#include <pistache/http.h>
#include "utils/Logger.hpp"

/**
 * @class RuleController
 * @brief Controller для управления правилами (Rules) через REST API.
 *
 * Обеспечивает CRUD-операции и дополнительные действия с правилами:
 * - Создание
 * - Получение по ID
 * - Обновление
 * - Удаление
 * - Получение списка правил по теплице
 * - Переключение состояния правила (включить/выключить)
 */
class RuleController : public BaseController
{
public:
    /**
     * @brief Конструктор контроллера правил.
     * @param db Ссылка на объект базы данных.
     * @param jwt_secret Секрет для валидации JWT.
     */
    RuleController(Database &db, const std::string &jwt_secret)
        : BaseController(db, jwt_secret), rule_manager_(db)
    {
        LOG_INFO_SG("RuleController initialized");
    }

    /**
     * @brief Настройка маршрутов REST API в роутере.
     * @param router Объект Pistache::Rest::Router.
     */
    void setup_routes(Pistache::Rest::Router &router) override;
    /// @brief
    ~RuleController() = default;

private:
    RuleManager rule_manager_;

    /**
     * @brief Обрабатывает POST /rules
     * @param request HTTP-запрос с JSON телом, содержащим данные правила.
     *        Ожидаемые поля:
     *        - gh_id (int)
     *        - name (string)
     *        - from_comp_id (int)
     *        - to_comp_id (int)
     *        - kind (string)
     *        - operator (string, optional)
     *        - threshold (double, optional)
     *        - time_spec (string, optional)
     * @param response HTTP-ответ.
     * @return 201 Created с JSON {"rule_id": <new_rule_id>} в случае успеха;
     *         400 Bad Request при некорректном JSON или отсутствии обязательных полей;
     *         500 Internal Server Error при ошибке БД.
     */
    void create_rule(const Pistache::Rest::Request &request,
                     Pistache::Http::ResponseWriter response);

    /**
     * @brief Обрабатывает GET /rules/:id
     * @param request HTTP-запрос с параметром :id (int)
     * @param response HTTP-ответ.
     * @return 200 OK с JSON-представлением Rule;
     *         400 Bad Request при некорректном ID;
     *         401 Unauthorized при отсутствии или неверном токене;
     *         404 Not Found если правило не найдено.
     */
    void get_rule(const Pistache::Rest::Request &request,
                  Pistache::Http::ResponseWriter response);

    /**
     * @brief Обрабатывает PUT /rules/:id
     * @param request HTTP-запрос с параметром :id (int) и JSON телом с полями для обновления.
     *        Поддерживаемые поля:
     *        - name (string)
     *        - operator (string)
     *        - threshold (double)
     *        - time_spec (string)
     *        - enabled (bool)
     * @param response HTTP-ответ.
     * @return 204 No Content при успешном обновлении;
     *         400 Bad Request при некорректном JSON или ID;
     *         401 Unauthorized при отсутствии админских прав;
     *         404 Not Found если правило не найдено;
     *         500 Internal Server Error при ошибке БД.
     */
    void update_rule(const Pistache::Rest::Request &request,
                     Pistache::Http::ResponseWriter response);

    /**
     * @brief Обрабатывает DELETE /rules/:id
     * @param request HTTP-запрос с параметром :id (int)
     * @param response HTTP-ответ.
     * @return 204 No Content при успешном удалении;
     *         400 Bad Request при некорректном ID;
     *         401 Unauthorized при отсутствии админских прав;
     *         404 Not Found если правило не найдено.
     */
    void delete_rule(const Pistache::Rest::Request &request,
                     Pistache::Http::ResponseWriter response);

    /**
     * @brief Обрабатывает GET /greenhouses/:gh_id/rules
     * @param request HTTP-запрос с параметром :gh_id (int)
     * @param response HTTP-ответ.
     * @return 200 OK с JSON-массивом правил для указанной теплицы;
     *         400 Bad Request при некорректном ID;
     *         401 Unauthorized при отсутствии токена.
     */
    void get_rules_by_greenhouse(const Pistache::Rest::Request &request,
                                 Pistache::Http::ResponseWriter response);

    /**
     * @brief Обрабатывает POST /rules/:id/toggle
     * @param request HTTP-запрос с параметром :id (int) и JSON телом {"enabled": bool}.
     * @param response HTTP-ответ.
     * @return 204 No Content при успешном переключении;
     *         400 Bad Request при некорректном JSON или ID;
     *         401 Unauthorized при отсутствии админских прав;
     *         500 Internal Server Error при ошибке БД.
     */
    void toggle_rule(const Pistache::Rest::Request &request,
                     Pistache::Http::ResponseWriter response);
};

#endif // RULECONTROLLER_HPP
