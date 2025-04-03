#include "authentication.h"

#include <nlohmann/json.hpp>

#include "../authentication-functions.h"
#include "../utils.h"

#include "../database/client.h"

namespace controllers::authentication
{
    http::response<http::string_body> sign_up(http::request<http::string_body> const& req,
                                              http::response<http::string_body>& res)
    {
        auto body = nlohmann::json::parse(req.body());
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
        const auto auth_header = req[http::field::authorization];
        if (auth_header.empty())
        {
            res.result(http::status::unauthorized);
            res.body() = nlohmann::json::parse(R"({"message": "Authorization header is missing."})").dump();
            res.prepare_payload();
            return res;
        }

        const auto space_pos = auth_header.find(' ');
        if (space_pos == string::npos)
        {
            res.result(http::status::unauthorized);
            res.body() = nlohmann::json::parse(R"({"message": "Invalid authorization header format."})").dump();
            res.prepare_payload();
            return res;
        }

        const auto data = verify_token(auth_header.substr(space_pos + 1));
        if (!is_valid_token_data(data))
        {
            res.result(http::status::unauthorized);
            res.body() = nlohmann::json::parse(R"({"message": "Invalid or expired token."})").dump();
            res.prepare_payload();
            return res;
        }

        auto body = nlohmann::json::parse(req.body());
        if (!body.contains("password") || !body.contains("new_password"))
        {
            res.result(http::status::bad_request);
            nlohmann::json response;
            response["message"] = "Missing required fields: password, new_password";
            res.body() = response.dump();
            res.prepare_payload();
            return res;
        }

        const auto users = database::client::query(
            "SELECT * FROM users WHERE id = $1 AND status = 'active'",
            {data["id"].get<std::string>()}
        );
        const auto user = users[0];
        auto salt = user[4];
        const auto hashed = ::hash(body["password"].get<std::string>(), salt);
        const auto new_hashed = ::hash(body["new_password"].get<std::string>(), salt);
        if (hashed != user[3])
        {
            res.result(http::status::not_found);
            nlohmann::json response;
            response["message"] = "Invalid username or password";
            res.body() = response.dump();
            res.prepare_payload();
            return res;
        }

        const auto update_result = database::client::query(
            "UPDATE users SET password = $1 WHERE id = $2",
            {new_hashed, data["id"].get<std::string>()}
        );

        res.result(http::status::ok);
        nlohmann::json response;
        response["message"] = "Password changed successfully";
        res.body() = response.dump();
        res.prepare_payload();
        return res;
    }

    http::response<http::string_body> sign_in(http::request<http::string_body> const& req,
                                              http::response<http::string_body>& res)
    {
        auto body = nlohmann::json::parse(req.body());
        if (!body.contains("username") || !body.contains("password"))
        {
            res.result(http::status::bad_request);
            nlohmann::json response;
            response["message"] = "Missing required fields: username, password";
            res.body() = response.dump();
            res.prepare_payload();
            return res;
        }

        const auto users = database::client::query(
            "SELECT * FROM users WHERE username = $1 AND status = 'active'",
            {body["username"].get<std::string>()}
        );
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
        nlohmann::json refresh = {{"type", "refresh"}, {"user", response["user"]}};
        response["access"] = generate_access_token(response["user"]);
        response["refresh"] = generate_refresh_token(refresh);
        res.body() = response.dump();
        res.prepare_payload();
        return res;
    }

    http::response<http::string_body> renew_tokens(http::request<http::string_body> const& req,
                                                   http::response<http::string_body>& res)
    {
        const auto auth_header = req[http::field::authorization];
        if (auth_header.empty())
        {
            res.result(http::status::unauthorized);
            res.body() = nlohmann::json::parse(R"({"message": "Authorization header is missing."})").dump();
            res.prepare_payload();
            return res;
        }

        if (!auth_header.starts_with("Bearer "))
        {
            res.result(http::status::unauthorized);
            res.body() = nlohmann::json::parse(R"({"message": "Invalid authorization header format."})").dump();
            res.prepare_payload();
            return res;
        }

        const auto token = auth_header.substr(7);
        const auto data = verify_token(token);
        const auto user = data["user"];
        if (data.empty() || !data.contains("type") || data["type"] != "refresh" || !is_valid_token_data(user))
        {
            res.result(http::status::unauthorized);
            res.body() = nlohmann::json::parse(R"({"message": "Invalid or expired token."})").dump();
            res.prepare_payload();
            return res;
        }

        res.result(http::status::ok);
        nlohmann::json response;
        response["access"] = generate_access_token(user);
        response["refresh"] = generate_refresh_token(data);
        res.body() = response.dump();
        res.prepare_payload();
        return res;
    }
}
