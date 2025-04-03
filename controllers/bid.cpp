#include <nlohmann/json.hpp>

#include "post.h"

#include "../utils.h"

#include "../database/client.h"

using namespace std;

namespace controllers::post
{
    http::response<http::string_body> bid(http::request<http::string_body> const& req,
                                          http::response<http::string_body>& res)
    {
        nlohmann::json request_body = nlohmann::json::parse(req.body());
        const string user_id = request_body["user_id"];
        const string post_id = request_body["post_id"];
        const int new_price = request_body["new_price"];

        // Retrieve post status and current price
        const auto post_result = database::client::query("SELECT status, price FROM posts WHERE id = $1;", {post_id});
        if (post_result.empty())
        {
            res.result(http::status::internal_server_error);
            res.body() = R"({"message": "Internal Server Error"})";
            res.prepare_payload();
            return res;
        }
        string status = post_result[0][0];
        double current_price = stod(post_result[0][1]);

        if (status == "inactive" || status == "sold")
        {
            res.result(http::status::bad_request);
            res.body() = R"({"message": "Post is not available for bidding"})";
            res.prepare_payload();
            return res;
        }

        if (new_price <= current_price)
        {
            res.result(http::status::bad_request);
            res.body() = R"({"message": "Bid price must be larger than the current price"})";
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
        int balance = stoi(user_balance_result[0][0]);
        if (balance < new_price)
        {
            res.result(http::status::bad_request);
            res.body() = R"({"message": "Insufficient balance"})";
            res.prepare_payload();
            return res;
        }

        const auto uuid = to_string(gen_uuid());

        // Update post with the current bidded price
        if (database::client::query("UPDATE posts SET price = $1 WHERE id = $2;", {to_string(new_price), post_id}).
            empty())
        {
            res.result(http::status::internal_server_error);
            res.body() = R"({"message": "Internal Server Error"})";
            res.prepare_payload();
            return res;
        }

        // Insert new bid into the bids table
        const auto bid_result = database::client::query(
            "INSERT INTO bids (id, user_id, post_id, price) VALUES ($1, $2, $3, $4) RETURNING id;",
            {uuid, user_id, post_id, to_string(new_price)}
        );

        if (bid_result.empty())
        {
            res.result(http::status::internal_server_error);
            res.body() = R"({"message": "Internal Server Error"})";
            res.prepare_payload();
            return res;
        }

        nlohmann::json response;
        response["message"] = "Bid placed successfully";
        response["bid_id"] = bid_result[0][0];

        res.result(http::status::created);
        res.body() = response.dump();
        res.prepare_payload();
        return res;
    }

    nlohmann::json get_bids_by_post_id(const string& post_id)
    {
        nlohmann::json bids_json = nlohmann::json::array();
        const auto bids = database::client::query("SELECT * FROM bids WHERE post_id = $1;", {post_id});

        for (const auto& bid : bids)
        {
            bids_json.push_back({
                {"id", bid[0]},
                {"user_id", bid[1]},
                {"price", bid[3]},
            });
        }

        return bids_json;
    }
}
