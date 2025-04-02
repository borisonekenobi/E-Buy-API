#include <nlohmann/json.hpp>

#include "authentication.h"

#include "../authentication-functions.h"
#include "../utils.h"
#include "../database/client.h"

namespace controllers::authentication
{
    http::response<http::string_body> sign_up(http::request<http::string_body> const& req,
                                              http::response<http::string_body>& res)
    {
        nlohmann::json body = nlohmann::json::parse(req.body());
        if (!body.contains("name") || !body.contains("username") || !body.contains("password"))
        {
            res.result(http::status::bad_request);
            nlohmann::json response;
            response["message"] = "Missing required fields: name, username, password";
            res.body() = response.dump();
            res.prepare_payload();
            return res;
        }

        const auto user = database::client::query("SELECT * FROM users WHERE username = $1",
                                                  {body["username"].get<std::string>()});
        if (!user.empty())
        {
            res.result(http::status::conflict);
            nlohmann::json response;
            response["message"] = "Username already exists";
            res.body() = response.dump();
            res.prepare_payload();
            return res;
        }

        string salt;
        const auto hashed = ::hash(body["password"].get<std::string>(), salt);
        const auto insert_result = database::client::query(
            "INSERT INTO users (id, name, username, password, salt, type, status) VALUES ($1, $2, $3, $4, $5, $6, $7)",
            {
                to_string(gen_uuid()),
                body["name"].get<std::string>(),
                body["username"].get<std::string>(),
                hashed,
                salt,
                "standard",
                "active"
            }
        );

        res.result(http::status::created);
        nlohmann::json response;
        response["message"] = "User created successfully";
        res.body() = response.dump();
        res.prepare_payload();
        return res;
    }

    http::response<http::string_body> change_password(http::request<http::string_body> const& req,
                                                      http::response<http::string_body>& res)
    {
        verifyToken()

        res.result(http::status::not_implemented);
        nlohmann::json response;
        response["message"] = "Not implemented";
        res.body() = response.dump();
        res.prepare_payload();
        return res;
    }

    http::response<http::string_body> sign_in(http::request<http::string_body> const& req,
                                              http::response<http::string_body>& res)
    {
        nlohmann::json body = nlohmann::json::parse(req.body());
        if (!body.contains("username") || !body.contains("password"))
        {
            res.result(http::status::bad_request);
            nlohmann::json response;
            response["message"] = "Missing required fields: username, password";
            res.body() = response.dump();
            res.prepare_payload();
            return res;
        }

        auto users = database::client::query("SELECT * FROM users WHERE username = $1",
                                             {body["username"].get<std::string>()});
        if (users.empty())
        {
            res.result(http::status::not_found);
            nlohmann::json response;
            response["message"] = "Invalid username or password";
            res.body() = response.dump();
            res.prepare_payload();
            return res;
        }

        auto user = users[0];
        auto salt = user[4];
        if (const auto hashed = ::hash(body["password"].get<std::string>(), salt); hashed != user[3])
        {
            res.result(http::status::not_found);
            nlohmann::json response;
            response["message"] = "Invalid username or password";
            res.body() = response.dump();
            res.prepare_payload();
            return res;
        }

        res.result(http::status::ok);
        nlohmann::json response;
        response["user"] = {
            {"id", user[0]},
            {"name", user[1]},
            {"username", user[2]},
            {"type", user[5]},
            {"status", user[6]}
        };
        response["access"] = generate_access_token(response["user"]);
        response["refresh"] = generate_refresh_token(response["user"]);
        res.body() = response.dump();
        res.prepare_payload();
        return res;
    }

    http::response<http::string_body> renew_tokens(http::request<http::string_body> const& req,
                                                   http::response<http::string_body>& res)
    {
        res.result(http::status::not_implemented);
        nlohmann::json response;
        response["message"] = "Not implemented";
        res.body() = response.dump();
        res.prepare_payload();
        return res;
    }
}
