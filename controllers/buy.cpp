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
        if (!is_valid_uuid(post_id, uuid) || uuid.version() != 4)
        {
            res.result(http::status::bad_request);
            res.body() = nlohmann::json::parse(R"({"message": "Invalid Post ID format"})").dump();
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
        if (post[5] != "sale")
        {
            res.result(http::status::forbidden);
            res.body() = nlohmann::json::parse(R"({"message": "This post is not for sale."})").dump();
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
                {to_string(gen_uuid()), data["id"], to_string(uuid), post[POST_PRICE_INDEX]}
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
            try {
                database::client::transactional_query(db, "ROLLBACK TRANSACTION;", {});
                database::client::close_connection(db);
            } catch (...) {}

            res.result(http::status::internal_server_error);
            res.body() = R"({"message": "Internal Server Error"})";
            res.prepare_payload();
            return res;
        }

        nlohmann::json response;
        response["message"] = "Post bought successfully";
        response["post"] = {
            {"id", post[0]},
            {"user_id", post[1]},
            {"title", post[2]},
            {"description", post[3]},
            {"price", post[4]},
            {"type", post[5]},
            {"status", "sold"},
            {"transaction", {
                {"id", to_string(gen_uuid())},
                {"user_id", data["id"]},
                {"price", post[4]},
            }}
        };
        res.result(http::status::ok);
        res.body() = response.dump();
        res.prepare_payload();
        return res;
    }
}
