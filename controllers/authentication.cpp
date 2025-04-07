#include "authentication.h"

#include <nlohmann/json.hpp>

#include "../authentication-functions.h"
#include "../utils.h"

#include "../database/client.h"

#define INVALID_USERNAME_PASSWORD R"({"message": "Invalid username or password"})"

namespace controllers::authentication
{
    void sign_up(http::request<http::string_body> const& req, http::response<http::string_body>& res)
    {
        nlohmann::json body;
        if (string message; is_malformed_body(req.body(), {"name", "username", "password"}, message, body))
            return prepare_response(res, http::status::bad_request, message);

        const auto users = database::client::query(
            "SELECT * FROM users WHERE username = $1",
            {body["username"].get<string>()}
        );
        if (!users.empty())
            return prepare_response(res, http::status::conflict, R"({"message": "Username already exists"})");

        string salt;
        const auto hashed = ::hash(body["password"].get<string>(), salt);
        auto insert_result = database::client::query(
            "INSERT INTO users (id, name, username, password, salt, type, status) VALUES ($1, $2, $3, $4, $5, $6, $7)",
            {
                to_string(gen_uuid()),
                body["name"].get<string>(),
                body["username"].get<string>(),
                hashed,
                salt,
                "standard",
                "active"
            }
        );

        prepare_response(res, http::status::created, R"({"message": "User created successfully"})");
    }

    void change_password(http::request<http::string_body> const& req, http::response<http::string_body>& res)
    {
        nlohmann::json auth;
        if (string message; is_malformed_auth(req[http::field::authorization], message, auth))
            return prepare_response(res, http::status::unauthorized, message);

        nlohmann::json body;
        if (string message; is_malformed_body(req.body(), {"password", "new_password"}, message, body))
            return prepare_response(res, http::status::bad_request, message);

        const auto users = database::client::query(
            "SELECT * FROM users WHERE id = $1 AND status = 'active'",
            {auth["id"].get<string>()}
        );

        if (users.empty())
            return prepare_response(res, http::status::not_found, R"({"message": "User not found"})");

        const auto& user = users[FIRST_OR_ONLY];
        auto salt = user[USER_SALT_INDEX];
        const auto hashed = ::hash(body["password"].get<string>(), salt);
        const auto new_hashed = ::hash(body["new_password"].get<string>(), salt);
        if (hashed != user[USER_PASSWORD_INDEX])
            return prepare_response(res, http::status::unauthorized, R"({"message": "Invalid password"})");

        auto update_result = database::client::query(
            "UPDATE users SET password = $1 WHERE id = $2",
            {new_hashed, auth["id"].get<std::string>()}
        );

        prepare_response(res, http::status::ok, R"({"message": "Password changed successfully"})");
    }

    void sign_in(http::request<http::string_body> const& req, http::response<http::string_body>& res)
    {
        nlohmann::json body;
        if (string message; is_malformed_body(req.body(), {"username", "password"}, message, body))
            return prepare_response(res, http::status::bad_request, message);

        const auto users = database::client::query(
            "SELECT * FROM users WHERE username = $1 AND status = 'active'",
            {body["username"].get<string>()}
        );
        if (users.empty())
            return prepare_response(res, http::status::not_found, INVALID_USERNAME_PASSWORD);

        auto user = users[FIRST_OR_ONLY];
        auto salt = user[USER_SALT_INDEX];
        if (const auto hashed = ::hash(body["password"].get<std::string>(), salt); hashed != user[USER_PASSWORD_INDEX])
            return prepare_response(res, http::status::not_found, INVALID_USERNAME_PASSWORD);

        nlohmann::json response;
        response["user"] = {
            {"id", user[USER_ID_INDEX]},
            {"name", user[USER_NAME_INDEX]},
            {"username", user[USER_USERNAME_INDEX]},
            {"type", user[USER_TYPE_INDEX]},
            {"status", user[USER_STATUS_INDEX]}
        };
        const nlohmann::json refresh = {{"type", "refresh"}, {"user", response["user"]}};
        response["access"] = generate_access_token(response["user"]);
        response["refresh"] = generate_refresh_token(refresh);
        prepare_response(res, http::status::ok, response.dump());
    }

    void renew_tokens(http::request<http::string_body> const& req, http::response<http::string_body>& res)
    {
        nlohmann::json auth;
        if (string message; is_malformed_renew(req[http::field::authorization], message, auth))
            return prepare_response(res, http::status::unauthorized, message);

        nlohmann::json response;
        response["access"] = generate_access_token(auth["user"].get<nlohmann::json>());
        response["refresh"] = generate_refresh_token(auth);
        prepare_response(res, http::status::ok, response.dump());
    }
}
