#pragma once

#include <boost/beast/http.hpp>

namespace http = boost::beast::http;

namespace controllers
{
	class test
	{
	public:
		static http::response<http::string_body> test_get(http::request<http::string_body> const& req, http::response<http::string_body>& res);
		static http::response<http::string_body> test_post(http::request<http::string_body> const& req, http::response<http::string_body>& res);
	};
}
