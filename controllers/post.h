#pragma once

#include <boost/beast/http.hpp>

namespace http = boost::beast::http;

namespace controllers
{
    class post
    {
    public:
        static http::response<http::string_body> get_post(http::request<http::string_body> const& req,
                                                          http::response<http::string_body>& res);
        static http::response<http::string_body> create_post(http::request<http::string_body> const& req,
                                                           http::response<http::string_body>& res);
    };
}
