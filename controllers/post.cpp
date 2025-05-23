#include "post.h"

#include <jwt-cpp/jwt.h>

#include <nlohmann/json.hpp>

#include "../utils.h"

#include "../database/client.h"

using namespace std;

namespace controllers::post
{
    void create(http::request<http::string_body> const& req, http::response<http::string_body>& res)
    {
        nlohmann::json auth;
        if (string message; is_malformed_auth(req[http::field::authorization], message, auth))
            return prepare_response(res, http::status::unauthorized, message);

        nlohmann::json body;
        if (string message; is_malformed_body(req.body(), {"title", "description", "price", "type"}, message, body))
            return prepare_response(res, http::status::bad_request, message);

        double price;
        if (string message; !is_valid_price(body["price"], message, price))
            return prepare_response(res, http::status::bad_request, message);

        const string uuid = to_string(gen_uuid());
        const string user_id = auth["id"].get<string>();
        const string title = body["title"].get<string>();
        const string description = body["description"].get<string>();
        const string type = body["type"].get<string>();
        const string status = "active";

        auto result = database::client::query(
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

        prepare_response(res, http::status::created, response.dump());
    }

    void find(http::request<http::string_body> const& req, http::response<http::string_body>& res)
    {
        const auto type = req.target().substr(11);
        auto posts = database::client::query(
            "SELECT * FROM posts WHERE status = 'active' AND type = $1 ORDER BY random() LIMIT 10;",
            {type}
        );

        if (posts.empty())
            return prepare_response(res, http::status::not_found, nlohmann::json::array().dump());

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

        prepare_response(res, http::status::ok, response.dump());
    }

    void find_one(http::request<http::string_body> const& req, http::response<http::string_body>& res)
    {
        const string post_id = req.target().substr(11);
        if (post_id.empty())
            return prepare_response(res, http::status::bad_request, R"({"message": "Post ID is missing"})");

        boost::uuids::uuid uuid;
        if (!is_valid_uuid(post_id, uuid))
            return prepare_response(res, http::status::bad_request, R"({"message": "Invalid Post ID format"})");

        auto posts = database::client::query("SELECT * FROM posts WHERE id = $1;", {to_string(uuid)});
        if (posts.empty())
            return prepare_response(res, http::status::not_found, R"({"message": "Post not found"})");

        const auto post = posts[FIRST_OR_ONLY];
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
            auto transactions = database::client::query(
                "SELECT * FROM transactions WHERE post_id = $1;",
                {post_id}
            );
            if (transactions.empty())
                response["transaction"] = nlohmann::json();
            else
                response["transaction"] = {
                    {"id", transactions[FIRST_OR_ONLY][TRANSACTION_ID_INDEX]},
                    {"user_id", transactions[FIRST_OR_ONLY][TRANSACTION_USER_ID_INDEX]},
                    {"price", transactions[FIRST_OR_ONLY][TRANSACTION_PRICE_INDEX]},
                };
        }
        else
        {
            auto bids = database::client::query(
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

        prepare_response(res, http::status::ok, response.dump());
    }

    void update(http::request<http::string_body> const& req, http::response<http::string_body>& res)
    {
        nlohmann::json auth;
        if (string message; is_malformed_auth(req[http::field::authorization], message, auth))
            return prepare_response(res, http::status::unauthorized, message);

        const string post_id = req.target().substr(11);
        boost::uuids::uuid uuid;
        if (!is_valid_uuid(post_id, uuid))
            return prepare_response(res, http::status::bad_request, R"({"message": "Invalid Post ID format"})");

        auto posts = database::client::query(
            "SELECT * FROM posts WHERE id = $1;",
            {to_string(uuid)}
        );
        if (posts.empty())
            return prepare_response(res, http::status::not_found, R"({"message": "Post not found"})");

        if (const auto& post = posts[FIRST_OR_ONLY]; post[POST_USER_ID_INDEX] != auth["id"].get<string>())
            return prepare_response(res, http::status::forbidden, R"({"message": "You are not authorized to update this post"})");

        nlohmann::json body;
        if (string message; is_malformed_body(req.body(), {"title", "description", "price", "type"}, message, body))
            return prepare_response(res, http::status::bad_request, message);

        double price;
        if (string message; !is_valid_price(body["price"], message, price))
            return prepare_response(res, http::status::bad_request, message);

        const string user_id = auth["id"].get<string>();
        const string title = body["title"].get<string>();
        const string description = body["description"].get<string>();
        const string type = body["type"].get<string>();

        auto update_result = database::client::query(
            "UPDATE posts SET title = $1, description = $2, price = $3, type = $4 WHERE id = $5 AND user_id = $6;",
            {title, description, to_string(price), type, to_string(uuid), user_id}
        );

        prepare_response(res, http::status::ok, R"({"message": "Post updated successfully"})");
    }

    void delete_(http::request<http::string_body> const& req, http::response<http::string_body>& res)
    {
        nlohmann::json auth;
        if (string message; is_malformed_auth(req[http::field::authorization], message, auth))
            return prepare_response(res, http::status::unauthorized, message);

        const string post_id = req.target().substr(11);
        boost::uuids::uuid uuid;
        if (!is_valid_uuid(post_id, uuid))
            return prepare_response(res, http::status::bad_request, R"({"message": "Invalid Post ID format"})");

        const auto posts = database::client::query(
            "SELECT * FROM posts WHERE id = $1;",
            {to_string(uuid)}
        );
        if (posts.empty())
            return prepare_response(res, http::status::not_found, R"({"message": "Post not found"})");

        if (const auto& post = posts[FIRST_OR_ONLY]; post[POST_USER_ID_INDEX] != auth["id"].get<string>())
            return prepare_response(res, http::status::forbidden, R"({"message": "You are not authorized to update this post"})");

        auto update_result = database::client::query(
            "UPDATE posts SET status = 'inactive' WHERE id = $1 AND user_id = $2;",
            {to_string(uuid), auth["id"].get<string>()}
        );

        prepare_response(res, http::status::ok, R"({"message": "Post deleted successfully"})");
    }
}
