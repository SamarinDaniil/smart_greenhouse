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

int main()
{
   auto pas_hash = utils::PasswordHasher::generate_hash("23s1dfSamarin");
   std::cout << pas_hash << std::endl;
   if (utils::PasswordHasher::validate_password("23s1dfSamarin", pas_hash) == true)
      std::cout << "ok" << std::endl;

   auto dbPtr = std::make_unique<db::Database>();
   if (dbPtr->initialize())
      std::cout << "ok!" << std::endl;
   auto user_dbPtr = std::make_unique<db::UserManager>();
   std::optional<User> userOpt = user_dbPtr->get_by_id(1);

   if (userOpt.has_value())
   {
      // Есть значение — пользователь найден
      const User &user = userOpt.value();

      std::cout << "Пользователь найден:\n";
      std::cout << "ID: " << user.user_id << "\n";
      std::cout << "Имя: " << user.username << "\n";
      std::cout << "password: " << user.password_hash << "\n";
      std::cout << "Роль: " << user.role << "\n";
      std::cout << "Дата создания: " << user.created_at << "\n";
      if (utils::PasswordHasher::validate_password("23s1dfSamarin", user.password_hash) == true)
         std::cout << "ok" << std::endl;
   }
   else
   {
      std::cout << "Пользователь с ID 1 не найден.\n";
   }

   return 0;
}