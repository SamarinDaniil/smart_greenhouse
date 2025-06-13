#ifndef USERMANAGER_HPP
#define USERMANAGER_HPP
#include "entities/User.hpp"
#include "db/Database.hpp"
#include <optional>
#include <vector>

/// @brief 
class UserManager
{
public:
    /// @brief 
    /// @param db 
    explicit UserManager(Database &db) : db_(db) {}

    /// @brief 
    /// @param user 
    /// @param password 
    /// @return 
    bool create(User &user, const std::string &password);
    /// @brief 
    /// @param username 
    /// @param password 
    /// @return 
    bool authenticate(const std::string &username, const std::string &password);
    /// @brief 
    /// @param user_id 
    /// @param new_role 
    /// @return 
    bool update_role(int user_id, const std::string &new_role);
    /// @brief 
    /// @param user_id 
    /// @return 
    std::optional<User> get_by_id(int user_id); 
    /// @brief 
    /// @param username 
    /// @return 
    std::optional<User> get_by_username(const std::string& username);
    /// @brief 
    /// @return 
    std::vector<User> get_all();

private:
    /// @brief 
    Database &db_;
};

#endif // USERMANAGER_HPP