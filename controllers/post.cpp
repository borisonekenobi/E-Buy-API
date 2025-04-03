#include "post.h"

#include <nlohmann/json.hpp>

#include "../authentication-functions.h"
#include "../utils.h"

#include "../database/client.h"

using namespace std;

namespace controllers::post
{
    http::response<http::string_body> create(http::request<http::string_body> const& req,
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
        if (!body.contains("title") || !body.contains("description") || !body.contains("price") ||
            !body.contains("type"))
        {
            res.result(http::status::bad_request);
            res.body() = nlohmann::json::parse(R"({"message": "Missing required fields."})").dump();
            res.prepare_payload();
            return res;
        }

        const string uuid = to_string(gen_uuid());
        const string user_id = data["id"].get<string>();
        const string title = body["title"].get<string>();
        const string description = body["description"].get<string>();
        const double price = body["price"].get<double>();
        const string type = body["type"].get<string>();
        const string status = "active";

        const auto result = database::client::query(
            "INSERT INTO posts (id, user_id, title, description, price, type, status) VALUES ($1, $2, $3, $4, $5, $6, $7);",
            {uuid, user_id, title, description, to_string(price), type, status}
        );

        nlohmann::json response;
        response["message"] = "Post created successfully";
        response["post"] = {
            {"id", uuid},
            {"user_id", user_id},
            {"title", title},
            {"description", description},
            {"price", price},
            {"type", type},
            {"status", status}
        };
        res.result(http::status::created);
        res.body() = response.dump();
        res.prepare_payload();
        return res;
    }

    http::response<http::string_body> find(http::request<http::string_body> const& req,
                                           http::response<http::string_body>& res)
    {
        const auto posts = database::client::query(
            "SELECT * FROM posts WHERE status = 'active' ORDER BY random() LIMIT 10;",
            {}
        );

        if (posts.empty())
        {
            res.result(http::status::not_found);
            res.body() = nlohmann::json::parse(R"({"message": "No posts found"})").dump();
            res.prepare_payload();
            return res;
        }

        auto response = nlohmann::json::array();
        for (const auto& post : posts)
            response.push_back({
                {"id", post[POST_ID_INDEX]},
                {"user_id", post[POST_USER_ID_INDEX]},
                {"title", post[POST_TITLE_INDEX]},
                {"description", post[POST_DESCRIPTION_INDEX]},
                {"price", post[POST_PRICE_INDEX]},
                {"type", post[POST_TYPE_INDEX]},
                {"status", post[POST_STATUS_INDEX]},
            });
        res.result(http::status::ok);
        res.body() = response.dump();
        res.prepare_payload();
        return res;
    }

    http::response<http::string_body> find_one(http::request<http::string_body> const& req,
                                               http::response<http::string_body>& res)
    {
        const string post_id = req.target().substr(11);

        if (post_id.empty())
        {
            res.result(http::status::bad_request);
            res.body() = nlohmann::json::parse(R"({"message": "Post ID is missing"})").dump();
            res.prepare_payload();
            return res;
        }

        boost::uuids::uuid uuid;
        if (!is_valid_uuid(post_id, uuid) || uuid.version() != UUIDv4)
        {
            res.result(http::status::bad_request);
            res.body() = nlohmann::json::parse(R"({"message": "Invalid Post ID format"})").dump();
            res.prepare_payload();
            return res;
        }

        const auto post = database::client::query("SELECT * FROM posts WHERE id = $1;", {to_string(uuid)})[0];
        if (post.empty())
        {
            res.result(http::status::not_found);
            res.body() = nlohmann::json::parse(R"({"message": "Post not found"})").dump();
            res.prepare_payload();
            return res;
        }

        nlohmann::json response = {
            {"id", post[POST_ID_INDEX]},
            {"user_id", post[POST_USER_ID_INDEX]},
            {"title", post[POST_TITLE_INDEX]},
            {"description", post[POST_DESCRIPTION_INDEX]},
            {"price", post[POST_PRICE_INDEX]},
            {"type", post[POST_TYPE_INDEX]},
            {"status", post[POST_STATUS_INDEX]},
        };

        if (response["type"] == "sale")
        {
            const auto transactions = database::client::query(
                "SELECT * FROM transactions WHERE post_id = $1;",
                {post_id}
            );
            if (transactions.empty())
                response["transaction"] = nlohmann::json();
            else
                response["transaction"] = {
                {"id", transactions[0][TRANSACTION_ID_INDEX]},
                {"user_id", transactions[0][TRANSACTION_USER_ID_INDEX]},
                {"price", transactions[0][TRANSACTION_PRICE_INDEX]},
            };
        }
        else
        {
            const auto bids = database::client::query(
                "SELECT * FROM bids WHERE post_id = $1 ORDER BY price DESC;",
                {post_id}
            );
            response["bids"] = nlohmann::json::array();
            for (const auto& bid : bids)
                response["bids"].push_back({
                    {"id", bid[BID_ID_INDEX]},
                    {"user_id", bid[BID_USER_ID_INDEX]},
                    {"price", bid[BID_PRICE_INDEX]},
                });
        }

        res.result(http::status::ok);
        res.body() = response.dump();
        res.prepare_payload();
        return res;
    }

    http::response<http::string_body> update(http::request<http::string_body> const& req,
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

        const string post_id = req.target().substr(11);
        boost::uuids::uuid uuid;
        if (!is_valid_uuid(post_id, uuid) || uuid.version() != UUIDv4)
        {
            res.result(http::status::bad_request);
            res.body() = nlohmann::json::parse(R"({"message": "Invalid Post ID format"})").dump();
            res.prepare_payload();
            return res;
        }

        auto body = nlohmann::json::parse(req.body());
        if (!body.contains("title") || !body.contains("description") || !body.contains("price") ||
            !body.contains("type"))
        {
            res.result(http::status::bad_request);
            res.body() = nlohmann::json::parse(R"({"message": "Missing required fields."})").dump();
            res.prepare_payload();
            return res;
        }

        const string user_id = data["id"].get<string>();
        const string title = body["title"].get<string>();
        const string description = body["description"].get<string>();
        const double price = body["price"].get<double>();
        const string type = body["type"].get<string>();

        const auto result = database::client::query(
            "UPDATE posts SET title = $1, description = $2, price = $3, type = $4 WHERE id = $5 AND user_id = $6;",
            {title, description, to_string(price), type, to_string(uuid), user_id}
        );

        nlohmann::json response;
        response["message"] = "Post updated successfully";
        res.result(http::status::ok);
        res.body() = response.dump();
        res.prepare_payload();
        return res;
    }

    http::response<http::string_body> delete_(http::request<http::string_body> const& req,
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

        const string post_id = req.target().substr(11);
        boost::uuids::uuid uuid;
        if (!is_valid_uuid(post_id, uuid) || uuid.version() != UUIDv4)
        {
            res.result(http::status::bad_request);
            res.body() = nlohmann::json::parse(R"({"message": "Invalid Post ID format"})").dump();
            res.prepare_payload();
            return res;
        }

        const auto result = database::client::query(
            "UPDATE posts SET status = 'inactive' WHERE id = $1 AND user_id = $2;",
            {to_string(uuid), data["id"]}
        );

        nlohmann::json response;
        response["message"] = "Post deleted successfully";
        res.result(http::status::ok);
        res.body() = response.dump();
        res.prepare_payload();
        return res;
    }
}
