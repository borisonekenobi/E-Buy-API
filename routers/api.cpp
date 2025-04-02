#include "api.h"

#include "../controllers/post.h"
#include "../controllers/test.h"

namespace http = boost::beast::http;

namespace routers::api
{
	http::response<http::string_body> handle_request(http::request<http::string_body> const& req,
	                                                 http::response<http::string_body>& res)
	{
		if (req.method() == http::verb::get && req.target() == "/api/test")
			return controllers::test::test_get(req, res);
		if (req.method() == http::verb::post && req.target() == "/api/test")
			return controllers::test::test_post(req, res);

		if (req.method() == http::verb::post && req.target() == "/api/posts")
			return controllers::post::create_post(req, res);
		if (req.method() == http::verb::get && req.target().starts_with("/api/posts/"))
			return controllers::post::get_post(req, res);

		return http::response<http::string_body>{http::status::not_found, req.version()};
	}
}
