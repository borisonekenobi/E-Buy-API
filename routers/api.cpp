#include "api.h"
#include "../controllers/test.h"

namespace http = boost::beast::http;

using namespace std;

namespace routers
{
	http::response<http::string_body> api::handle_request(http::request<http::string_body> const& req,
	                                                      http::response<http::string_body>& res)
	{
		if (req.method() == http::verb::get && req.target() == "/api/test")
			return controllers::test::test_get(req, res);
		if (req.method() == http::verb::post && req.target() == "/api/test")
			return controllers::test::test_post(req, res);

		return http::response<http::string_body>{http::status::not_found, req.version()};
	}
}
