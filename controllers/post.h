#pragma once

#include <boost/beast/http.hpp>

namespace http = boost::beast::http;

namespace controllers::post
{
    http::response<http::string_body> create(http::request<http::string_body> const& req,
                                             http::response<http::string_body>& res);
    http::response<http::string_body> find(http::request<http::string_body> const& req,
                                           http::response<http::string_body>& res);
    http::response<http::string_body> find_one(http::request<http::string_body> const& req,
                                               http::response<http::string_body>& res);
    http::response<http::string_body> edit(http::request<http::string_body> const& req,
                                           http::response<http::string_body>& res);
    http::response<http::string_body> delete_(http::request<http::string_body> const& req,
                                              http::response<http::string_body>& res);

    http::response<http::string_body> bid(http::request<http::string_body> const& req,
                                          http::response<http::string_body>& res);
    http::response<http::string_body> buy(http::request<http::string_body> const& req,
                                          http::response<http::string_body>& res);
}
