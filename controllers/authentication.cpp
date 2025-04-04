#include "authentication.h"

#include <nlohmann/json.hpp>

#include "../authentication-functions.h"
#include "../utils.h"

#include "../database/client.h"

#define INVALID_USERNAME_PASSWORD R"({"message": "Invalid username or password"})"

namespace controllers::authentication
{
    http::response<http::string_body> sign_up(http::request<http::string_body> const& req,
                                              http::response<http::string_body>& res)
    {
        nlohmann::json body;
        if (string message; is_malformed_body(req.body(), {"name", "username", "password"}, message, body))
        {
            res.result(http::status::bad_request);
            res.body() = message;
            res.prepare_payload();
            return res;
        }

        vector<vector<string>> users;
        if (!database::client::query("SELECT * FROM users WHERE username = $1", {body["username"].get<string>()},
                                     users))
            throw runtime_error(DATABASE_ERROR);

        if (!users.empty())
        {
            res.result(http::status::conflict);
            res.body() = R"({"message": "Username already exists"})";
            res.prepare_payload();
            return res;
        }

        string salt;
        const auto hashed = ::hash(body["password"].get<string>(), salt);
        if (vector<vector<string>> insert_result; !database::client::query(
            "INSERT INTO users (id, name, username, password, salt, type, status) VALUES ($1, $2, $3, $4, $5, $6, $7)",
            {
                to_string(gen_uuid()),
                body["name"].get<string>(),
                body["username"].get<string>(),
                hashed,
                salt,
                "standard",
                "active"
            },
            insert_result
        ))
            throw runtime_error(DATABASE_ERROR);

        res.result(http::status::created);
        res.body() = R"({"message": "User created successfully"})";
        res.prepare_payload();
        return res;
    }

    http::response<http::string_body> change_password(http::request<http::string_body> const& req,
                                                      http::response<http::string_body>& res)
    {
        nlohmann::json auth;
        if (string message; is_malformed_auth(req[http::field::authorization], message, auth))
        {
            res.result(http::status::unauthorized);
            res.body() = message;
            res.prepare_payload();
            return res;
        }

        nlohmann::json body;
        if (string message; is_malformed_body(req.body(), {"password", "new_password"}, message, body))
        {
            res.result(http::status::bad_request);
            res.body() = message;
            res.prepare_payload();
            return res;
        }

        vector<vector<string>> users;
        if (!database::client::query("SELECT * FROM users WHERE id = $1 AND status = 'active'",
                                     {auth["id"].get<string>()},
                                     users
        ))
            throw runtime_error(DATABASE_ERROR);

        if (users.empty())
        {
            res.result(http::status::not_found);
            res.body() = R"({"message": "User not found"})";
            res.prepare_payload();
            return res;
        }

        const auto& user = users[0];
        auto salt = user[USER_SALT_INDEX];
        const auto hashed = ::hash(body["password"].get<string>(), salt);
        const auto new_hashed = ::hash(body["new_password"].get<string>(), salt);
        if (hashed != user[USER_PASSWORD_INDEX])
        {
            res.result(http::status::unauthorized);
            res.body() = R"({"message": "Invalid password"})";
            res.prepare_payload();
            return res;
        }

        if (vector<vector<string>> update_result; !database::client::query(
            "UPDATE users SET password = $1 WHERE id = $2",
            {new_hashed, auth["id"].get<std::string>()},
            update_result
        ))
            throw runtime_error(DATABASE_ERROR);

        res.result(http::status::ok);
        res.body() = R"({"message": "Password changed successfully"})";
        res.prepare_payload();
        return res;
    }

    http::response<http::string_body> sign_in(http::request<http::string_body> const& req,
                                              http::response<http::string_body>& res)
    {
        nlohmann::json body;
        if (string message; is_malformed_body(req.body(), {"username", "password"}, message, body))
        {
            res.result(http::status::bad_request);
            res.body() = message;
            res.prepare_payload();
            return res;
        }

        vector<vector<string>> users;
        if (!database::client::query("SELECT * FROM users WHERE username = $1 AND status = 'active'",
                                     {body["username"].get<string>()},
                                     users
        ))
            throw runtime_error(DATABASE_ERROR);

        if (users.empty())
        {
            res.result(http::status::not_found);
            res.body() = INVALID_USERNAME_PASSWORD;
            res.prepare_payload();
            return res;
        }

        auto user = users[0];
        auto salt = user[USER_SALT_INDEX];
        if (const auto hashed = ::hash(body["password"].get<std::string>(), salt); hashed != user[USER_PASSWORD_INDEX])
        {
            res.result(http::status::not_found);
            res.body() = INVALID_USERNAME_PASSWORD;
            res.prepare_payload();
            return res;
        }

        res.result(http::status::ok);
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
        res.body() = response.dump();
        res.prepare_payload();
        return res;
    }

    http::response<http::string_body> renew_tokens(http::request<http::string_body> const& req,
                                                   http::response<http::string_body>& res)
    {
        nlohmann::json auth;
        if (string message; is_malformed_renew(req[http::field::authorization], message, auth))
        {
            res.result(http::status::bad_request);
            res.body() = message;
            res.prepare_payload();
            return res;
        }

        res.result(http::status::ok);
        nlohmann::json response;
        response["access"] = generate_access_token(auth["user"].get<nlohmann::json>());
        response["refresh"] = generate_refresh_token(auth);
        res.body() = response.dump();
        res.prepare_payload();
        return res;
    }
}
