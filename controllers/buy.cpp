#include <nlohmann/json.hpp>

#include "post.h"

using namespace std;

namespace controllers::post
{
    http::response<http::string_body> buy(http::request<http::string_body> const& req,
                                          http::response<http::string_body>& res)
    {
        res.result(http::status::not_implemented);
        res.body() = R"({"message": "Not Implemented"})";
        res.prepare_payload();
        return res;
    }
}
