#include <nlohmann/json.hpp>

#include "post.h"

#include "../authentication-functions.h"
#include "../utils.h"

#include "../database/client.h"

using namespace std;

namespace controllers::post
{
    http::response<http::string_body> bid(http::request<http::string_body> const& req,
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

        const string post_id = req.target().substr(15);
        boost::uuids::uuid uuid;
        if (!is_valid_uuid(post_id, uuid) || uuid.version() != UUIDv4)
        {
            res.result(http::status::bad_request);
            res.body() = nlohmann::json::parse(R"({"message": "Invalid Post ID format"})").dump();
            res.prepare_payload();
            return res;
        }

        const auto body = nlohmann::json::parse(req.body());
        if (!body.contains("price"))
        {
            res.result(http::status::bad_request);
            res.body() = nlohmann::json::parse(R"({"message": "Missing required fields."})").dump();
            res.prepare_payload();
            return res;
        }

        const auto results = database::client::query(
            "SELECT * FROM posts WHERE id = $1 AND status = 'active';",
            {to_string(uuid)}
        );
        if (results.empty())
        {
            res.result(http::status::not_found);
            res.body() = nlohmann::json::parse(R"({"message": "Post not found."})").dump();
            res.prepare_payload();
            return res;
        }

        const auto& post = results[0];
        if (post[POST_TYPE_INDEX] != "auction")
        {
            res.result(http::status::forbidden);
            res.body() = nlohmann::json::parse(R"({"message": "Post is not an auction."})").dump();
            res.prepare_payload();
            return res;
        }

        database::client::query(
            "INSERT INTO bids (id, user_id, post_id, price) VALUES ($1, $2, $3, $4);",
            {to_string(gen_uuid()), data["id"], to_string(uuid), body["price"]}
        );

        const auto bids = database::client::query(
            "SELECT * FROM bids WHERE post_id = $1 ORDER BY price DESC;",
            {to_string(uuid)}
        );

        nlohmann::json response;
        response["message"] = "Bid placed successfully";
        response["post"] = {
            {"id", post[POST_ID_INDEX]},
            {"user_id", post[POST_USER_ID_INDEX]},
            {"title", post[POST_TITLE_INDEX]},
            {"description", post[POST_DESCRIPTION_INDEX]},
            {"price", post[POST_PRICE_INDEX]},
            {"type", post[POST_TYPE_INDEX]},
            {"status", post[POST_STATUS_INDEX]},
            {"bids", nlohmann::json::array()}
        };
        for (const auto& bid : bids)
            response["post"]["bids"].push_back({
                {"id", bid[BID_ID_INDEX]},
                {"user_id", bid[BID_USER_ID_INDEX]},
                {"price", bid[BID_PRICE_INDEX]},
            });
        res.result(http::status::ok);
        res.body() = response.dump();
        res.prepare_payload();
        return res;
    }
}
