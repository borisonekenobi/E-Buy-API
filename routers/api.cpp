#include "api.h"

#include "../controllers/authentication.h"
#include "../controllers/post.h"

namespace http = boost::beast::http;

namespace routers::api
{
	http::response<http::string_body> handle_request(http::request<http::string_body> const& req,
	                                                 http::response<http::string_body>& res)
	{
		if (req.method() == http::verb::post && req.target() == "/api/sign-up")
			return controllers::authentication::sign_up(req, res);
		if (req.method() == http::verb::post && req.target() == "/api/change-password")
			return controllers::authentication::change_password(req, res);
		if (req.method() == http::verb::post && req.target() == "/api/sign-in")
			return controllers::authentication::sign_in(req, res);
		if (req.method() == http::verb::post && req.target() == "/api/renew-tokens")
			return controllers::authentication::renew_tokens(req, res);

		if (req.method() == http::verb::post && req.target().starts_with("/api/posts/buy/"))
			return controllers::post::buy(req, res);

		if (req.method() == http::verb::post && req.target().starts_with("/api/posts/bid/"))
			return controllers::post::bid(req, res);

		if (req.method() == http::verb::post && req.target() == "/api/posts")
			return controllers::post::create(req, res);
		if (req.method() == http::verb::get && req.target() == "/api/posts/sale")
			return controllers::post::find(req, res);
		if (req.method() == http::verb::get && req.target() == "/api/posts/auction")
            return controllers::post::find(req, res);
		if (req.method() == http::verb::get && req.target().starts_with("/api/posts/"))
			return controllers::post::find_one(req, res);
		if (req.method() == http::verb::put && req.target().starts_with("/api/posts/"))
			return controllers::post::update(req, res);
		if (req.method() == http::verb::delete_ && req.target().starts_with("/api/posts/"))
			return controllers::post::delete_(req, res);

		return http::response<http::string_body>{http::status::not_found, req.version()};
	}
}
