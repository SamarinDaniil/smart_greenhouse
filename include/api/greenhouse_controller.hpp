#pragma once

#include "base_controller.hpp"
#include "managers/GreenhouseManager.hpp"
#include "entities/Greenhouse.hpp"

/**
 * @class GreenhouseController
 * @brief Контроллер для обработки запросов, связанных с теплицами.
 *
 * Предоставляет методы для создания, чтения, обновления и удаления записей о теплицах.
 */
class GreenhouseController : public BaseController
{
public:
    /**
     * @brief Конструктор контроллера.
     *
     * @param db Ссылка на объект базы данных.
     * @param jwt_secret Секретный ключ для проверки JWT токенов.
     */
    GreenhouseController(Database &db,
                         const std::string &jwt_secret)
        : BaseController(db, jwt_secret),
          greenhouse_manager_(db) {}

    /**
     * @brief Регистрация маршрутов для работы с теплицами.
     *
     * @param router Роутер Pistache.
     */
    void setup_routes(Pistache::Rest::Router &router) override
    {
        LOG_DEBUG("Setting up greenhouse routes");
        BaseController::setup_routes(router);

        using namespace Pistache::Rest;
        Routes::Get(router, "/greenhouses", Routes::bind(&GreenhouseController::get_all, this));
        Routes::Get(router, "/greenhouses/:id", Routes::bind(&GreenhouseController::get_by_id, this));
        Routes::Post(router, "/greenhouses", Routes::bind(&GreenhouseController::create, this));
        Routes::Put(router, "/greenhouses/:id", Routes::bind(&GreenhouseController::update, this));
        Routes::Delete(router, "/greenhouses/:id", Routes::bind(&GreenhouseController::remove, this));
    }

private:
    GreenhouseManager greenhouse_manager_;

    /**
     * @brief Получает список всех теплиц.
     *
     * @param request Объект HTTP-запроса.
     * @param response Объект для отправки HTTP-ответа.
     *
     * @return JSON-массив со списком теплиц или ошибка авторизации/сервера.
     *
     * @response 200 OK - Успешно. Возвращается массив теплиц.
     * @response 401 Unauthorized - Неавторизованный доступ.
     * @response 500 Internal Server Error - Ошибка сервера.
     */
    void get_all(const Pistache::Rest::Request &request,
                 Pistache::Http::ResponseWriter response)
    {
        LOG_TRACE("Entering get_all method");
        auto auth = authenticate_request(request);
        if (!auth.valid)
        {
            LOG_WARN("Authentication failed: %s", auth.error.c_str());
            response.send(Pistache::Http::Code::Unauthorized, auth.error);
            return;
        }

        try
        {
            LOG_DEBUG("Fetching all greenhouses for user: %d", auth.user_id);
            auto greenhouses = greenhouse_manager_.get_all();
            LOG_DEBUG("Retrieved %d greenhouses", greenhouses.size());
            response.send(Pistache::Http::Code::Ok, nlohmann::json(greenhouses).dump());
        }
        catch (const std::exception &e)
        {
            LOG_ERROR("Get all greenhouses error: %s", e.what());
            response.send(Pistache::Http::Code::Internal_Server_Error, e.what());
        }
    }
    /**
     * @brief Получает данные по конкретной теплице по ID.
     *
     * @param request Объект HTTP-запроса.
     * @param response Объект для отправки HTTP-ответа.
     *
     * @return JSON-объект с данными теплицы или ошибка.
     *
     * @response 200 OK - Успешно. Возвращены данные теплицы.
     * @response 401 Unauthorized - Неавторизованный доступ.
     * @response 404 Not Found - Теплица не найдена.
     * @response 400 Bad Request - Неверный формат ID.
     * @response 500 Internal Server Error - Ошибка сервера.
     */
    void get_by_id(const Pistache::Rest::Request &request,
                   Pistache::Http::ResponseWriter response)
    {
        LOG_TRACE("Entering get_by_id method");
        auto auth = authenticate_request(request);
        if (!auth.valid)
        {
            LOG_WARN("Authentication failed: %s", auth.error.c_str());
            response.send(Pistache::Http::Code::Unauthorized, auth.error);
            return;
        }

        try
        {
            int gh_id = std::stoi(request.param(":id").as<std::string>());
            LOG_DEBUG("Fetching greenhouse details for ID: %d", gh_id);

            auto greenhouse_opt = greenhouse_manager_.get_by_id(gh_id);

            if (!greenhouse_opt)
            {
                LOG_WARN("Greenhouse not found: %d", gh_id);
                response.send(Pistache::Http::Code::Not_Found, "Greenhouse not found");
                return;
            }

            response.send(Pistache::Http::Code::Ok, nlohmann::json(*greenhouse_opt).dump());
            LOG_DEBUG("Successfully retrieved greenhouse: %d", gh_id);
        }
        catch (const std::exception &e)
        {
            LOG_ERROR("Get greenhouse by ID error: %s", e.what());
            response.send(Pistache::Http::Code::Bad_Request, e.what());
        }
    }

    /**
     * @brief Создаёт новую теплицу.
     *
     * @param request Объект HTTP-запроса.
     * @param response Объект для отправки HTTP-ответа.
     *
     * @return JSON-объект созданной теплицы или ошибка.
     *
     * @response 201 Created - Успешно. Теплица создана.
     * @response 401 Unauthorized - Неавторизованный доступ.
     * @response 403 Forbidden - Пользователь не является администратором.
     * @response 400 Bad Request - Неверный формат данных.
     * @response 500 Internal Server Error - Ошибка при сохранении.
     */
    void create(const Pistache::Rest::Request &request,
                Pistache::Http::ResponseWriter response)
    {
        LOG_TRACE("Entering create method");
        auto auth = authenticate_request(request);
        if (!require_admin_role(auth, response))
            return;

        try
        {
            LOG_DEBUG("Creating new greenhouse for admin user: %d", auth.user_id);
            nlohmann::json json_body = nlohmann::json::parse(request.body());
            Greenhouse greenhouse;
            greenhouse.name = json_body["name"];

            if (json_body.contains("location"))
            {
                greenhouse.location = json_body["location"];
                LOG_DEBUG("Location specified: %s", greenhouse.location.c_str());
            }

            if (greenhouse_manager_.create(greenhouse))
            {
                LOG_INFO("Greenhouse created by user %d: %s (ID: %d)",
                         auth.user_id, greenhouse.name.c_str(), greenhouse.greenhouse_id);
                response.send(Pistache::Http::Code::Created, nlohmann::json(greenhouse).dump());
            }
            else
            {
                LOG_ERROR("Failed to create greenhouse: %s", greenhouse.name.c_str());
                response.send(Pistache::Http::Code::Internal_Server_Error, "Failed to create greenhouse");
            }
        }
        catch (const std::exception &e)
        {
            LOG_ERROR("Create greenhouse error: %s", e.what());
            response.send(Pistache::Http::Code::Bad_Request, e.what());
        }
    }

    /**
     * @brief Обновляет информацию о существующей теплице.
     *
     * @param request Объект HTTP-запроса.
     * @param response Объект для отправки HTTP-ответа.
     *
     * @return JSON-объект обновлённой теплицы или ошибка.
     *
     * @response 200 OK - Успешно. Данные обновлены.
     * @response 401 Unauthorized - Неавторизованный доступ.
     * @response 403 Forbidden - Пользователь не является администратором.
     * @response 404 Not Found - Теплица не найдена.
     * @response 400 Bad Request - Неверный формат данных.
     * @response 500 Internal Server Error - Ошибка при обновлении.
     */
    void update(const Pistache::Rest::Request &request,
                Pistache::Http::ResponseWriter response)
    {
        LOG_TRACE("Entering update method");
        auto auth = authenticate_request(request);
        if (!require_admin_role(auth, response))
            return;

        try
        {
            int gh_id = std::stoi(request.param(":id").as<std::string>());
            LOG_DEBUG("Updating greenhouse ID: %d by user: %d", gh_id, auth.user_id);

            auto existing_opt = greenhouse_manager_.get_by_id(gh_id);

            if (!existing_opt)
            {
                LOG_WARN("Attempt to update non-existent greenhouse: %d", gh_id);
                response.send(Pistache::Http::Code::Not_Found, "Greenhouse not found");
                return;
            }

            Greenhouse existing = *existing_opt;
            nlohmann::json json_body = nlohmann::json::parse(request.body());
            existing.name = json_body["name"];

            if (json_body.contains("location"))
            {
                existing.location = json_body["location"];
                LOG_DEBUG("Updating location to: %s", existing.location.c_str());
            }

            if (greenhouse_manager_.update(existing))
            {
                LOG_INFO("Greenhouse updated by user %d: %s (ID: %d)",
                         auth.user_id, existing.name.c_str(), existing.greenhouse_id);
                response.send(Pistache::Http::Code::Ok, nlohmann::json(existing).dump());
            }
            else
            {
                LOG_ERROR("Failed to update greenhouse: %s (ID: %d)", existing.name.c_str(), existing.greenhouse_id);
                response.send(Pistache::Http::Code::Internal_Server_Error, "Failed to update greenhouse");
            }
        }
        catch (const std::exception &e)
        {
            LOG_ERROR("Update greenhouse error: %s", e.what());
            response.send(Pistache::Http::Code::Bad_Request, e.what());
        }
    }

    /**
     * @brief Удаляет теплицу по её ID.
     *
     * @param request Объект HTTP-запроса.
     * @param response Объект для отправки HTTP-ответа.
     *
     * @return Сообщение об успешном удалении или ошибка.
     *
     * @response 200 OK - Успешно. Теплица удалена.
     * @response 401 Unauthorized - Неавторизованный доступ.
     * @response 403 Forbidden - Пользователь не является администратором.
     * @response 404 Not Found - Теплица не найдена.
     * @response 400 Bad Request - Неверный формат ID.
     * @response 500 Internal Server Error - Ошибка при удалении.
     */
    void remove(const Pistache::Rest::Request &request,
                Pistache::Http::ResponseWriter response)
    {
        LOG_TRACE("Entering remove method");
        auto auth = authenticate_request(request);
        if (!require_admin_role(auth, response))
            return;

        try
        {
            int gh_id = std::stoi(request.param(":id").as<std::string>());
            LOG_DEBUG("Deleting greenhouse ID: %d by user: %d", gh_id, auth.user_id);

            if (greenhouse_manager_.remove(gh_id))
            {
                LOG_INFO("Greenhouse deleted successfully: %d by user: %d", gh_id, auth.user_id);
                response.send(Pistache::Http::Code::Ok, "Greenhouse deleted");
            }
            else
            {
                LOG_ERROR("Failed to delete greenhouse: %d", gh_id);
                response.send(Pistache::Http::Code::Internal_Server_Error, "Failed to delete greenhouse");
            }
        }
        catch (const std::exception &e)
        {
            LOG_ERROR("Delete greenhouse error: %s", e.what());
            response.send(Pistache::Http::Code::Bad_Request, e.what());
        }
    }
};