#include <boost/beast/http.hpp>
#include <nlohmann/json.hpp>

#include "test.h"
#include "../authentication-functions.h"
#include "../database/client.h"

namespace http = boost::beast::http;
using db = database::client;

using namespace std;

namespace controllers
{
	http::response<http::string_body> test::test_get(http::request<http::string_body> const& req,
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

		const auto data = verifyToken(auth_header.substr(space_pos + 1));

		nlohmann::json json_response;

		const auto countries = db::query("SELECT country FROM cities GROUP BY country;", {});
		json_response["countries"] = nlohmann::json::array();
		for (const auto& country : countries)
			json_response["countries"].push_back(country[0]);
		json_response["your_auth_data"] = data;
		auto new_token_data = nlohmann::json();
		new_token_data["name"] = "test";
		json_response["new_token"] = generateAccessToken(new_token_data);

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
