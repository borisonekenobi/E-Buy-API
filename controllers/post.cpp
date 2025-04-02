#include <nlohmann/json.hpp>
#include <iostream>

#include "post.h"
#include "../utils.h"
#include "../database/client.h"

using namespace std;

namespace controllers::post
{
    http::response<http::string_body> create_post(http::request<http::string_body> const& req,
                                                  http::response<http::string_body>& res)
    {
        res.result(http::status::not_implemented);
        nlohmann::json response;
        response["message"] = "Not implemented";
        res.body() = response.dump();
        res.prepare_payload();
        return res;
    }

    http::response<http::string_body> get_post(http::request<http::string_body> const& req,
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
        response["post"] = {
            {"id", post[0][0]},
            {"user_id", post[0][1]},
            {"title", post[0][2]},
            {"description", post[0][3]},
            {"price", post[0][4]},
            {"type", post[0][5]},
            {"status", post[0][6]},
        };

        const string post_type = response["post"]["type"];
        vector<vector<string>> post_data;
        if (post_type == "sale")
        {
            post_data = database::client::query("SELECT * FROM transactions WHERE post_id = $1;", {post_id});
            response["post"]["transactions"] = nlohmann::json::array();
            for (const auto& transaction : post_data)
            {
                response["post"]["transactions"].push_back({
                    {"id", transaction[0]},
                    {"user_id", transaction[1]},
                    {"price", transaction[3]},
                });
            }
        }
        else
        {
            post_data = database::client::query("SELECT * FROM bids WHERE post_id = $1;", {post_id});
            response["post"]["bids"] = nlohmann::json::array();
            for (const auto& bid : post_data)
            {
                response["post"]["bids"].push_back({
                    {"id", bid[0]},
                    {"user_id", bid[1]},
                    {"price", bid[3]},
                });
            }
        }

        res.result(http::status::ok);
        res.body() = response.dump();
        res.prepare_payload();
        return res;
    }
}
