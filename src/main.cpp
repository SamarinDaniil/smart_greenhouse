#include <drogon/drogon.h>
#include "plugins/DbPlugin.hpp"
#include "plugins/JwtPlugin.hpp"
#include <chrono>
#include <cassert>
#include <iostream>
#include <memory>
#include "utils/PasswordHasher.hpp"
#include "db/Database.hpp"
#include "db/managers/UserManager.hpp"
#include "entities/User.hpp"
#include <optional>
#include <drogon/HttpClient.h>
#include <thread>
#include <json/json.h>

using namespace drogon;
using namespace api;

int main()
{
   // 1. Тестирование хеширования паролей
   auto pas_hash = utils_sg::PasswordHasher::generate_hash("23s1dfSamarin");
   std::cout << "Generated hash: " << pas_hash << std::endl;
   if (utils_sg::PasswordHasher::validate_password("23s1dfSamarin", pas_hash))
   {
      std::cout << "Password validation: ok" << std::endl;
   }

   // 2. Тестирование базы данных
   auto dbPtr = std::make_unique<db::Database>();
   if (dbPtr->initialize())
   {
      std::cout << "Database initialized: ok!" << std::endl;
   }

   auto user_dbPtr = std::make_unique<db::UserManager>();
   std::optional<User> userOpt = user_dbPtr->get_by_id(1);

   if (userOpt.has_value())
   {
      const User &user = userOpt.value();
      std::cout << "\nUser found:\n";
      std::cout << "ID: " << user.user_id << "\n";
      std::cout << "Username: " << user.username << "\n";
      std::cout << "Role: " << user.role << "\n";

      if (utils_sg::PasswordHasher::validate_password("23s1dfSamarin", user.password_hash))
      {
         std::cout << "User password validation: ok" << std::endl;
      }
   }
   else
   {
      std::cout << "User with ID 1 not found.\n";
   }

   // 3. Загрузка конфигурации
   std::cout << "\n===== Loading configuration =====" << std::endl;

   // Загружаем конфиг из файла
   try
   {
      drogon::app().loadConfigFile("config/config.json");
      std::cout << "Configuration loaded successfully" << std::endl;
   }
   catch (const std::exception &e)
   {
      std::cerr << "Failed to load configuration: " << e.what() << std::endl;
      return 1;
   }

   // 4. Тестирование аутентификации
   std::cout << "\n===== Testing AuthController =====" << std::endl;

   // Запускаем HTTP-сервер в отдельном потоке
   std::thread serverThread([]()
                            {
        // Устанавливаем уровень логгирования из конфига
        app().setLogLevel(trantor::Logger::kDebug);
        app().run(); });

   // Ждем запуска сервера
   std::this_thread::sleep_for(std::chrono::seconds(1));

   // Получаем информацию о слушателях
   auto listeners = app().getListeners();
   if (listeners.empty())
   {
      std::cerr << "No listeners configured!" << std::endl;
      return 1;
   }

   int port = listeners.front().toPort();
   std::string address = "127.0.0.1";
   std::cout << "HTTP server started at " << address << ":" << port << std::endl;

   // Создаем HTTP-клиент
   auto client = HttpClient::newHttpClient(
       "http://" + address + ":" + std::to_string(port),
       trantor::EventLoop::getEventLoopOfCurrentThread(),
       5.0, // Таймаут подключения
       60.0 // Таймаут запроса
   );

   // Формируем запрос на авторизацию
   Json::Value loginRequest;
   loginRequest["username"] = "SamarinDaniil"; // Замените на реальное имя пользователя
   loginRequest["password"] = "23s1dfSamarin"; // Замените на реальный пароль

   Json::StreamWriterBuilder writer;
   std::string requestBody = Json::writeString(writer, loginRequest);

   auto req = HttpRequest::newHttpRequest();
   req->setPath("/api/login");
   req->setMethod(Post);
   req->setContentTypeCode(CT_APPLICATION_JSON);
   req->setBody(requestBody);

   std::cout << "\nSending login request to /api/login:\n"
             << requestBody << std::endl;

   // Отправляем запрос
   client->sendRequest(req, [](ReqResult result, const HttpResponsePtr &response)
                       {
        if (result != ReqResult::Ok) {
            std::cerr << "Request failed: " << result;
            
            // Проверяем наличие деталей ошибки
            if (result == ReqResult::BadServerAddress) {
                std::cerr << "Check server address and port" << std::endl;
            }
            else if (result == ReqResult::Timeout) {
                std::cerr << "Request timed out" << std::endl;
            }
            else if (result == ReqResult::NetworkFailure) {
                std::cerr << "Network failure" << std::endl;
            }
            
            app().quit();
            return;
        }
        
        std::cout << "\nReceived response:\n"
                  << "Status: " << response->getStatusCode() << "\n"
                  << "Body: " << response->getBody() << std::endl;
        
        // Останавливаем сервер после получения ответа
        app().quit(); });

   // Ждем завершения работы сервера
   serverThread.join();
   std::cout << "HTTP server stopped" << std::endl;

   return 0;
}