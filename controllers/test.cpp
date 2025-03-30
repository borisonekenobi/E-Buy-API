#include <boost/beast/http.hpp>
#include <nlohmann/json.hpp>

#include "test.h"
#include "../database/client.h"

namespace http = boost::beast::http;

using namespace std;

namespace controllers
{
	http::response<http::string_body> test::test_get(http::request<http::string_body> const& req,
	                                                 http::response<http::string_body>& res)
	{
		nlohmann::json json_response;

		const auto countries = database::client::query("SELECT country FROM cities GROUP BY country;");
		json_response["countries"] = nlohmann::json::array();
		for (const auto& country : countries)
			json_response["countries"].push_back(country[0]);

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
