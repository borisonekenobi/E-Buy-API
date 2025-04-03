#pragma once

#include <boost/beast/http.hpp>

namespace http = boost::beast::http;

namespace controllers::post
{
    http::response<http::string_body> create_post(http::request<http::string_body> const& req,
                                                  http::response<http::string_body>& res);
    http::response<http::string_body> buy_post(http::request<http::string_body> const& req,
                                                      http::response<http::string_body>& res);
    http::response<http::string_body> get_post(http::request<http::string_body> const& req,
                                               http::response<http::string_body>& res);
    http::response<http::string_body> change_post(http::request<http::string_body> const& req,
                                           http::response<http::string_body>& res);
    http::response<http::string_body> bid_post(http::request<http::string_body> const& req,
                                           http::response<http::string_body>& res);
}
