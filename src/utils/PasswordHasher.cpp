#include "utils/PasswordHasher.hpp"

std::string PasswordHasher::generate_hash(const std::string &password)
{
    // Генерация случайной соли для алгоритма SHA-512
    const std::string salt_prefix = "$6$";
    char salt[32];
    if (!crypt_gensalt_rn(salt_prefix.c_str(), 0, nullptr, 0, salt, sizeof(salt)))
    {
        throw std::runtime_error("Salt generation failed");
    }

    // Хеширование пароля с солью
    struct crypt_data data{};
    data.initialized = 0;

    const char *result = crypt_rn(password.c_str(), salt, &data, sizeof(data));
    if (!result)
    {
        throw std::runtime_error("Password hashing failed");
    }

    return result;
}

bool PasswordHasher::validate_password(const std::string &password, const std::string &hash)
{
    // Проверка пароля против существующего хеша
    struct crypt_data data{};
    data.initialized = 0;

    const char *result = crypt_rn(password.c_str(), hash.c_str(), &data, sizeof(data));
    if (!result)
    {
        return false;
    }

    return std::strcmp(result, hash.c_str()) == 0;
}
