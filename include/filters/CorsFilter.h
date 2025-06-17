#pragma once
#include <drogon/HttpFilter.h>
using namespace drogon;

class CorsFilter : public HttpFilter<CorsFilter>
{
public:
    void doFilter(const HttpRequestPtr &req,
                  FilterCallback &&fcb,
                  FilterChainCallback &&fccb) override;
    {
        auto resp = HttpResponse::newHttpResponse();
        resp->addHeader("Access-Control-Allow-Origin", "*");
        resp->addHeader("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
        resp->addHeader("Access-Control-Allow-Headers", "Content-Type, Authorization");
        resp->addHeader("Access-Control-Max-Age", "86400");

        // Preflight request
        if (req->getMethod() == Options)
        {
            fcb(resp);
            return;
        }
    }
};