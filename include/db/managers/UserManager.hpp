#pragma once
#include "entities/User.hpp"
#include "db/Database.hpp"

class UserManager
{
public:
    explicit UserManager(Database &db) : db_(db) {}

    bool create(User &user, const std::string &password);
    bool authenticate(const std::string &username, const std::string &password);
    bool update_role(int user_id, const std::string &new_role);
    User get_by_id(int user_id);
    std::vector<User> get_all();

private:
    Database &db_;
};