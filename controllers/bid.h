
#pragma once

#include <boost/beast/http.hpp>
#include <nlohmann/json.hpp>

namespace http = boost::beast::http;

namespace controllers::bid
{
    http::response<http::string_body> place_bid(http::request<http::string_body> const& req,
                                                  http::response<http::string_body>& res);
    http::response<http::string_body> get_bid(http::request<http::string_body> const& req,
                                               http::response<http::string_body>& res);
    nlohmann::json get_bids_by_post_id(const std::string& post_id);
}