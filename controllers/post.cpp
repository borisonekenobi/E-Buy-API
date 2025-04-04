#include "post.h"

#include <nlohmann/json.hpp>

#include "../utils.h"

#include "../database/client.h"

using namespace std;

namespace controllers::post
{
    http::response<http::string_body> create(http::request<http::string_body> const& req,
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
        if (string message; is_malformed_body(req.body(), {"title", "description", "price", "type"}, message, body))
        {
            res.result(http::status::bad_request);
            res.body() = message;
            res.prepare_payload();
            return res;
        }

        double price;
        if (string message; !is_valid_price(body["price"], message, price))
        {
            res.result(http::status::bad_request);
            res.body() = message;
            res.prepare_payload();
            return res;
        }

        const string uuid = to_string(gen_uuid());
        const string user_id = auth["id"].get<string>();
        const string title = body["title"].get<string>();
        const string description = body["description"].get<string>();
        const string type = body["type"].get<string>();
        const string status = "active";

        if (vector<vector<string>> result; !database::client::query(
            "INSERT INTO posts (id, user_id, title, description, price, type, status) VALUES ($1, $2, $3, $4, $5, $6, $7);",
            {uuid, user_id, title, description, to_string(price), type, status},
            result
        ))
            throw runtime_error(DATABASE_ERROR);

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
        const auto type = req.target().substr(11);
        vector<vector<string>> posts;
        if (!database::client::query(
            "SELECT * FROM posts WHERE status = 'active' AND type = $1 ORDER BY random() LIMIT 10;",
            {type},
            posts
        ))
            throw runtime_error(DATABASE_ERROR);

        if (posts.empty())
        {
            res.result(http::status::not_found);
            res.body() = nlohmann::json::array().dump();
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
        if (!is_valid_uuid(post_id, uuid))
        {
            res.result(http::status::bad_request);
            res.body() = nlohmann::json::parse(R"({"message": "Invalid Post ID format"})").dump();
            res.prepare_payload();
            return res;
        }

        vector<vector<string>> posts;
        if (!database::client::query("SELECT * FROM posts WHERE id = $1;", {to_string(uuid)}, posts))
            throw runtime_error(DATABASE_ERROR);

        if (posts.empty())
        {
            res.result(http::status::not_found);
            res.body() = R"({"message": "Post not found"})";
            res.prepare_payload();
            return res;
        }

        const auto post = posts[0];
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
            vector<vector<string>> transactions;
            if (!database::client::query(
                "SELECT * FROM transactions WHERE post_id = $1;",
                {post_id}, transactions
            ))
                throw runtime_error(DATABASE_ERROR);

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
            vector<vector<string>> bids;
            if (!database::client::query(
                "SELECT * FROM bids WHERE post_id = $1 ORDER BY price DESC;",
                {post_id}, bids
            ))
                throw runtime_error(DATABASE_ERROR);

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
        nlohmann::json auth;
        if (string message; is_malformed_auth(req[http::field::authorization], message, auth))
        {
            res.result(http::status::unauthorized);
            res.body() = message;
            res.prepare_payload();
            return res;
        }

        const string post_id = req.target().substr(11);
        boost::uuids::uuid uuid;
        if (!is_valid_uuid(post_id, uuid))
        {
            res.result(http::status::bad_request);
            res.body() = R"({"message": "Invalid Post ID format"})";
            res.prepare_payload();
            return res;
        }

        nlohmann::json body;
        if (string message; is_malformed_body(req.body(), {"title", "description", "price", "type"}, message, body))
        {
            res.result(http::status::bad_request);
            res.body() = message;
            res.prepare_payload();
            return res;
        }

        double price;
        if (string message; !is_valid_price(body["price"], message, price))
        {
            res.result(http::status::bad_request);
            res.body() = message;
            res.prepare_payload();
            return res;
        }

        const string user_id = auth["id"].get<string>();
        const string title = body["title"].get<string>();
        const string description = body["description"].get<string>();
        const string type = body["type"].get<string>();

        if (vector<vector<string>> update_result; !database::client::query(
            "UPDATE posts SET title = $1, description = $2, price = $3, type = $4 WHERE id = $5 AND user_id = $6;",
            {title, description, to_string(price), type, to_string(uuid), user_id},
            update_result
        ))
            throw runtime_error(DATABASE_ERROR);

        res.result(http::status::ok);
        res.body() = R"({"message": "Post updated successfully"})";
        res.prepare_payload();
        return res;
    }

    http::response<http::string_body> delete_(http::request<http::string_body> const& req,
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

        const string post_id = req.target().substr(11);
        boost::uuids::uuid uuid;
        if (!is_valid_uuid(post_id, uuid))
        {
            res.result(http::status::bad_request);
            res.body() = R"({"message": "Invalid Post ID format"})";
            res.prepare_payload();
            return res;
        }

        if (vector<vector<string>> update_result; !database::client::query(
            "UPDATE posts SET status = 'inactive' WHERE id = $1 AND user_id = $2;",
            {to_string(uuid), auth["id"].get<string>()}, update_result
        ))
            throw runtime_error(DATABASE_ERROR);

        res.result(http::status::ok);
        res.body() = R"({"message": "Post deleted successfully"})";
        res.prepare_payload();
        return res;
    }
}
