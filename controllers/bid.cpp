#include <nlohmann/json.hpp>

#include "post.h"

#include "../authentication-functions.h"
#include "../utils.h"

#include "../database/client.h"

using namespace std;

namespace controllers::post
{
    void bid(http::request<http::string_body> const& req, http::response<http::string_body>& res)
    {
        nlohmann::json auth;
        if (string message; is_malformed_auth(req[http::field::authorization], message, auth))
            return prepare_response(res, http::status::unauthorized, message);

        const string post_id = req.target().substr(15);
        boost::uuids::uuid uuid;
        if (!is_valid_uuid(post_id, uuid))
            return prepare_response(res, http::status::bad_request, R"({"message": "Invalid Post ID format"})");

        nlohmann::json body;
        if (string message; is_malformed_body(req.body(), {"price"}, message, body))
            return prepare_response(res, http::status::bad_request, message);

        double price;
        if (string message; !is_valid_price(body["price"], message, price))
            return prepare_response(res, http::status::bad_request, message);

        auto posts = database::client::query(
            "SELECT * FROM posts WHERE id = $1 AND status = 'active';",
            {to_string(uuid)}
        );
        if (posts.empty())
            return prepare_response(res, http::status::not_found, R"({"message": "Post not found"})");

        const auto& post = posts[FIRST_OR_ONLY];
        if (post[POST_TYPE_INDEX] != "auction")
            return prepare_response(res, http::status::forbidden, R"({"message": "Post is not an auction"})");

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

        auto insert_results = database::client::query(
            "INSERT INTO bids (id, user_id, post_id, price) VALUES ($1, $2, $3, $4);",
            {to_string(gen_uuid()), auth["id"].get<string>(), to_string(uuid), to_string(price)}
        );

        auto bids = database::client::query(
            "SELECT * FROM bids WHERE post_id = $1 ORDER BY price DESC;",
            {to_string(uuid)}
        );
        for (const auto& bid : bids)
            response["post"]["bids"].push_back({
                {"id", bid[BID_ID_INDEX]},
                {"user_id", bid[BID_USER_ID_INDEX]},
                {"price", bid[BID_PRICE_INDEX]},
            });

        prepare_response(res, http::status::ok, response.dump());
    }
}
