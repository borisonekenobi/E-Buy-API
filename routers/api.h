#pragma once

#include <boost/beast/http.hpp>

namespace http = boost::beast::http;

namespace routers::api
{
	http::response<http::string_body> handle_request(http::request<http::string_body> const& req,
	                                                 http::response<http::string_body>& res);
}
