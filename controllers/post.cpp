#include <nlohmann/json.hpp>
#include <iostream>

#include "post.h"
#include "../utils.h"
#include "bid.h"
#include "../database/client.h"

using namespace std;

namespace controllers::post {
    http::response<http::string_body> create_post(http::request<http::string_body> const &req,
                                                  http::response<http::string_body> &res)
    {
        nlohmann::json request_body = nlohmann::json::parse(req.body());
        const string user_id = request_body["user_id"];
        const string title = request_body["title"];
        const string description = request_body["description"];
        const string price = request_body["price"];
        const string type = request_body["type"];
        const string status = "active";

        const auto uuid = to_string(gen_uuid());

        // Insert new post into the posts table
        const auto result = database::client::query(
            "INSERT INTO posts (id, user_id, title, description, price, type, status) VALUES ($1, $2, $3, $4, $5, $6)",
            {uuid, user_id, title, description, price, type, status}
        );

        if (result.empty())
            {
            res.result(http::status::internal_server_error);
            res.body() = R"({"message": "Internal Server Error"})";
            res.prepare_payload();
            return res;
        }

        nlohmann::json response;
        response["message"] = "Post created successfully";
        response["post_id"] = result[0][0];

        res.result(http::status::created);
        res.body() = response.dump();
        res.prepare_payload();
        return res;
    }

http::response<http::string_body> buy_post(http::request<http::string_body> const &req,
                                           http::response<http::string_body> &res)
{
    nlohmann::json request_body = nlohmann::json::parse(req.body());
    const string user_id = request_body["user_id"];
    const string post_id = request_body["post_id"];

    // Retrieve post status
    const auto post_status_result = database::client::query("SELECT status FROM posts WHERE id = $1;", {post_id});
    if (post_status_result.empty())
    {
        res.result(http::status::internal_server_error);
        res.body() = R"({"message": "Internal Server Error"})";
        res.prepare_payload();
        return res;
    }
    string status = post_status_result[0][0];
    if (status == "inactive" || status == "sold")
    {
        res.result(http::status::bad_request);
        res.body() = R"({"message": "Post is not available for purchase"})";
        res.prepare_payload();
        return res;
    }

    // Retrieve user balance
    const auto user_balance_result = database::client::query("SELECT balance FROM users WHERE id = $1;", {user_id});
    if (user_balance_result.empty())
    {
        res.result(http::status::internal_server_error);
        res.body() = R"({"message": "Internal Server Error"})";
        res.prepare_payload();
        return res;
    }
    int balance = stod(user_balance_result[0][0]);

    // Retrieve post price
    const auto post_price_result = database::client::query("SELECT price FROM posts WHERE id = $1;", {post_id});
    if (post_price_result.empty())
    {
        res.result(http::status::internal_server_error);
        res.body() = R"({"message": "Internal Server Error"})";
        res.prepare_payload();
        return res;
    }
    int price = stod(post_price_result[0][0]);

    // Update user balance
    balance -= price;

    if (balance < 0)
    {
        res.result(http::status::bad_request);
        res.body() = R"({"message": "Insufficient balance"})";
        res.prepare_payload();
        return res;
    }
    else
    {
        if (database::client::query("UPDATE users SET balance = $1 WHERE id = $2;", {to_string(balance), user_id}).empty())
        {
            res.result(http::status::internal_server_error);
            res.body() = R"({"message": "Internal Server Error"})";
            res.prepare_payload();
            return res;
        }

        // Update post status
        if (database::client::query("UPDATE posts SET status = 'sold' WHERE id = $1;", {post_id}).empty())
        {
            res.result(http::status::internal_server_error);
            res.body() = R"({"message": "Internal Server Error"})";
            res.prepare_payload();
            return res;
        }

        res.result(http::status::created);
        res.body() = R"({"message": "Post purchased successfully"})";
        res.prepare_payload();
        return res;
    }
}


    http::response<http::string_body> get_post(http::request<http::string_body> const &req,
                                               http::response<http::string_body> &res)
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
        if (!is_valid_uuid(post_id, uuid) || uuid.version() != 4)
            {
            res.result(http::status::bad_request);
            res.body() = nlohmann::json::parse(R"({"message": "Invalid Post ID format"})").dump();
            res.prepare_payload();
            return res;
        }

        const auto post = database::client::query("SELECT * FROM posts WHERE id = $1;", {post_id});
        if (post.empty())
            {
            res.result(http::status::not_found);
            res.body() = nlohmann::json::parse(R"({"message": "Post not found"})").dump();
            res.prepare_payload();
            return res;
        }

        nlohmann::json response;
        response["post"] =
            {
            {"id", post[0][0]},
            {"user_id", post[0][1]},
            {"title", post[0][2]},
            {"description", post[0][3]},
            {"price", post[0][4]},
            {"type", post[0][5]},
            {"status", post[0][6]},
        };

        const string post_type = response["post"]["type"];
        vector<vector<string> > post_data;
        if (post_type == "sale")
            {
            post_data = database::client::query("SELECT * FROM transactions WHERE post_id = $1;", {post_id});
            response["post"]["transactions"] = nlohmann::json::array();
            for (const auto &transaction: post_data)
                {
                response["post"]["transactions"].push_back({
                    {"id", transaction[0]},
                    {"user_id", transaction[1]},
                    {"price", transaction[3]},
                });
            }
        } else
            {
            response["post"]["bids"] = bid::get_bids_by_post_id(post_id);
        }

        res.result(http::status::ok);
        res.body() = response.dump();
        res.prepare_payload();
        return res;
    }

    http::response<http::string_body> change_post(http::request<http::string_body> const &req,
                                                  http::response<http::string_body> &res)
    {
        nlohmann::json request_body = nlohmann::json::parse(req.body());
        const string post_id = request_body["post_id"];
        // Assuming the logged-in user ID is stored in the request
        const string logged_in_user_id = req["user_id"];

        // Validate post ID
        boost::uuids::uuid uuid;
        if (!is_valid_uuid(post_id, uuid) || uuid.version() != 4)
        {
            res.result(http::status::bad_request);
            res.body() = R"({"message": "Invalid Post ID format"})";
            res.prepare_payload();
            return res;
        }

        // Retrieve post user_id
        const auto post_result = database::client::query("SELECT user_id FROM posts WHERE id = $1;", {post_id});
        if (post_result.empty())
        {
            res.result(http::status::not_found);
            res.body() = R"({"message": "Not Found"})";
            res.prepare_payload();
            return res;
        }
        string post_user_id = post_result[0][0];

        // Check if the logged-in user is the same as the post user_id
        if (logged_in_user_id != post_user_id)
        {
            res.result(http::status::forbidden);
            res.body() = R"({"message": "You are not authorized to change this post"})";
            res.prepare_payload();
            return res;
        }

        // Update post status to inactive
        const auto result = database::client::query("UPDATE posts SET status = 'inactive' WHERE id = $1;", {post_id});
        if (result.empty())
        {
            res.result(http::status::internal_server_error);
            res.body() = R"({"message": "Internal Server Error"})";
            res.prepare_payload();
            return res;
        }

        nlohmann::json response;
        response["message"] = "Post status updated to inactive";

        res.result(http::status::ok);
        res.body() = response.dump();
        res.prepare_payload();
        return res;
    }
}
