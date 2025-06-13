#pragma once
#include <string>
#include <cstring>
#include <stdexcept>
#include <crypt.h>

class PasswordHasher {
public:
    static std::string generate_hash(const std::string& password) ;

    static bool validate_password(const std::string& password, const std::string& hash);
};