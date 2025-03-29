#include <boost/beast/http.hpp>
#include <nlohmann/json.hpp>

#include "test.h"

namespace http = boost::beast::http;

using namespace std;

namespace controllers
{
	http::response<http::string_body> test::test_get(http::request<http::string_body> const& req,
	                                                 http::response<http::string_body>& res)
	{
		nlohmann::json json_response;

		json_response.emplace("message", "Hello, World!");
		json_response.emplace("test", "This is a test");

		res.body() = json_response.dump();
		res.prepare_payload();
		return res;
	}

	http::response<http::string_body> test::test_post(http::request<http::string_body> const& req,
	                                                  http::response<http::string_body>& res)
	{
		auto json_request = nlohmann::json::parse(req.body());
		const string request_body = json_request.dump();
		//cout << request_body << endl;

		const string response_message = request_body;

		res.body() = response_message;
		res.prepare_payload();
		return res;
	}
}
