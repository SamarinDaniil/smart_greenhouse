#pragma once

#include <drogon/HttpFilter.h>
#include <string>
#include <algorithm>

namespace api
{

    /// JwtAuthFilter: проверяет JWT-токен для всех запросов, кроме /login
    class JwtAuthFilter : public drogon::HttpFilter<JwtAuthFilter>
    {
    public:
        JwtAuthFilter() = default;
        ~JwtAuthFilter() override = default;

        /**
         * @brief Основной метод фильтрации
         * @param req   Указатель на HTTP-запрос
         * @param fcb   Callback для немедленного ответа (например, при ошибке)
         * @param fccb  Callback для продолжения цепочки фильтров и контроллера
         */
        void doFilter(const drogon::HttpRequestPtr &req,
                      drogon::FilterCallback &&fcb,
                      drogon::FilterChainCallback &&fccb) override;
    };

} // namespace api