#pragma once

#include <boost/beast/http.hpp>

#define USER_ID_INDEX 0
#define USER_NAME_INDEX 1
#define USER_USERNAME_INDEX 2
#define USER_PASSWORD_INDEX 3
#define USER_SALT_INDEX 4
#define USER_TYPE_INDEX 5
#define USER_STATUS_INDEX 6

namespace http = boost::beast::http;

namespace controllers::authentication
{
    void sign_up(http::request<http::string_body> const& req, http::response<http::string_body>& res);
    void change_password(http::request<http::string_body> const& req, http::response<http::string_body>& res);
    void renew_tokens(http::request<http::string_body> const& req, http::response<http::string_body>& res);
    void sign_in(http::request<http::string_body> const& req, http::response<http::string_body>& res);
}
