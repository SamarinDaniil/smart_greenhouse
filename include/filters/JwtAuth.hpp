#pragma once
#include <drogon/HttpFilter.h>
#include <vector>
#include <string>
#include <regex>

class JwtAuth : public drogon::HttpFilter<JwtAuth>
{
public:
    static void setWhitelist(const std::vector<std::string>& patterns);
    
    void doFilter(const drogon::HttpRequestPtr& req,
                  drogon::FilterCallback&& fcb,
                  drogon::FilterChainCallback&& fccb) override;

private:
    static inline std::vector<std::regex> whitelistPatterns;
};