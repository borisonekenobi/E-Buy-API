#include <nlohmann/json.hpp>

#include "post.h"

#include "../authentication-functions.h"
#include "../utils.h"

#include "../database/client.h"

using namespace std;

namespace controllers::post
{
    http::response<http::string_body> buy(http::request<http::string_body> const& req,
                                          http::response<http::string_body>& res)
    {
        nlohmann::json auth;
        if (string message; is_malformed_auth(req[http::field::authorization], message, auth))
        {
            res.result(http::status::bad_request);
            res.body() = message;
            res.prepare_payload();
            return res;
        }

        const string post_id = req.target().substr(15);
        boost::uuids::uuid uuid;
        if (!is_valid_uuid(post_id, uuid))
        {
            res.result(http::status::bad_request);
            res.body() = R"({"message": "Invalid Post ID format"})";
            res.prepare_payload();
            return res;
        }

        vector<vector<string>> posts;
        if (!database::client::query(
            "SELECT * FROM posts WHERE id = $1 AND status = 'active';",
            {to_string(uuid)}, posts
        ))
            throw runtime_error(DATABASE_ERROR);

        if (posts.empty())
        {
            res.result(http::status::not_found);
            res.body() = R"({"message": "Post not found."})";
            res.prepare_payload();
            return res;
        }

        const auto& post = posts[0];
        if (post[POST_TYPE_INDEX] != "sale")
        {
            res.result(http::status::forbidden);
            res.body() = R"({"message": "This post is not for sale."})";
            res.prepare_payload();
            return res;
        }

        sqlite3* db = database::client::open_connection();
        try
        {
            /*** CRITICAL SECTION ***/
            // TODO: Use a mutex or a lock to ensure thread safety
            database::client::transactional_query(db, "BEGIN TRANSACTION;", {});
            database::client::transactional_query(
                db,
                "INSERT INTO transactions (id, user_id, post_id, price) VALUES ($1, $2, $3, $4);",
                {to_string(gen_uuid()), auth["id"].get<string>(), to_string(uuid), post[POST_PRICE_INDEX]}
            );
            database::client::transactional_query(
                db,
                "UPDATE posts SET status = 'sold' WHERE id = $1;",
                {to_string(uuid)}
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

            throw runtime_error(DATABASE_ERROR);
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
                    {"id", to_string(gen_uuid())},
                    {"user_id", auth["id"].get<string>()},
                    {"price", post[POST_PRICE_INDEX]},
                }
            }
        };
        res.result(http::status::ok);
        res.body() = response.dump();
        res.prepare_payload();
        return res;
    }
}
