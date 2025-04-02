#pragma once

#include <boost/beast/http.hpp>

namespace http = boost::beast::http;

namespace controllers::authentication
{
    http::response<http::string_body> sign_up(http::request<http::string_body> const& req,
                                              http::response<http::string_body>& res);
    http::response<http::string_body> change_password(http::request<http::string_body> const& req,
                                                      http::response<http::string_body>& res);
    http::response<http::string_body> renew_tokens(http::request<http::string_body> const& req,
                                                   http::response<http::string_body>& res);
    http::response<http::string_body> sign_in(http::request<http::string_body> const& req,
                                              http::response<http::string_body>& res);
}
