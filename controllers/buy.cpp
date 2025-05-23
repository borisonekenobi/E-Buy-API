#include <nlohmann/json.hpp>

#include "post.h"

#include "../authentication-functions.h"
#include "../utils.h"

#include "../database/client.h"

using namespace std;

namespace controllers::post
{
    void buy(http::request<http::string_body> const& req, http::response<http::string_body>& res)
    {
        nlohmann::json auth;
        if (string message; is_malformed_auth(req[http::field::authorization], message, auth))
            return prepare_response(res, http::status::unauthorized, message);

        const string post_id = req.target().substr(15);
        boost::uuids::uuid post_uuid;
        if (!is_valid_uuid(post_id, post_uuid))
            return prepare_response(res, http::status::bad_request, R"({"message": "Invalid Post ID format"})");

        const auto posts = database::client::query(
            "SELECT * FROM posts WHERE id = $1 AND status = 'active';",
            {to_string(post_uuid)}
        );
        if (posts.empty())
            return prepare_response(res, http::status::not_found, R"({"message": "Post not found"})");

        const auto& post = posts[FIRST_OR_ONLY];
        if (post[POST_TYPE_INDEX] != "sale")
            return prepare_response(res, http::status::forbidden, R"({"message": "This post is not for sale"})");

        const auto uuid = gen_uuid();
        sqlite3* db = database::client::open_connection();
        try
        {
            /*** CRITICAL SECTION ***/
            // TODO: Use a mutex or a lock to ensure thread safety
            database::client::transactional_query(db, "BEGIN TRANSACTION;", {});
            database::client::transactional_query(
                db,
                "INSERT INTO transactions (id, user_id, post_id, price) VALUES ($1, $2, $3, $4);",
                {to_string(uuid), auth["id"].get<string>(), to_string(post_uuid), post[POST_PRICE_INDEX]}
            );
            database::client::transactional_query(
                db,
                "UPDATE posts SET status = 'sold' WHERE id = $1;",
                {to_string(post_uuid)}
            );
            database::client::transactional_query(db, "COMMIT TRANSACTION;", {});
            database::client::close_connection(db);
            /*** END CRITICAL SECTION ***/
        }
        catch (const exception& e)
        {
            cerr << e.what() << endl;

            // Try to roll back (safe fallback)
            try
            {
                database::client::transactional_query(db, "ROLLBACK TRANSACTION;", {});
                database::client::close_connection(db);
            }
            catch (...)
            {
            }

            throw;
        }

        nlohmann::json response;
        response["message"] = "Post bought successfully";
        response["post"] = {
            {"id", post[POST_ID_INDEX]},
            {"user_id", post[POST_USER_ID_INDEX]},
            {"title", post[POST_TITLE_INDEX]},
            {"description", post[POST_DESCRIPTION_INDEX]},
            {"price", post[POST_PRICE_INDEX]},
            {"type", post[POST_TYPE_INDEX]},
            {"status", "sold"},
            {
                "transaction", {
                    {"id", to_string(uuid)},
                    {"user_id", auth["id"].get<string>()},
                    {"price", post[POST_PRICE_INDEX]},
                }
            }
        };

        prepare_response(res, http::status::ok, response.dump());
    }
}
