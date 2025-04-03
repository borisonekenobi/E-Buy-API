#pragma once

#include <boost/beast/http.hpp>

#define POST_ID_INDEX 0
#define POST_USER_ID_INDEX 1
#define POST_TITLE_INDEX 2
#define POST_DESCRIPTION_INDEX 3
#define POST_PRICE_INDEX 4
#define POST_TYPE_INDEX 5
#define POST_STATUS_INDEX 6

#define TRANSACTION_ID_INDEX 0
#define TRANSACTION_USER_ID_INDEX 1
#define TRANSACTION_POST_ID_INDEX 2
#define TRANSACTION_PRICE_INDEX 3

#define BID_ID_INDEX 0
#define BID_USER_ID_INDEX 1
#define BID_POST_ID_INDEX 2
#define BID_PRICE_INDEX 3

namespace http = boost::beast::http;

namespace controllers::post
{
    http::response<http::string_body> create(http::request<http::string_body> const& req,
                                             http::response<http::string_body>& res);
    http::response<http::string_body> find(http::request<http::string_body> const& req,
                                           http::response<http::string_body>& res);
    http::response<http::string_body> find_one(http::request<http::string_body> const& req,
                                               http::response<http::string_body>& res);
    http::response<http::string_body> update(http::request<http::string_body> const& req,
                                           http::response<http::string_body>& res);
    http::response<http::string_body> delete_(http::request<http::string_body> const& req,
                                              http::response<http::string_body>& res);

    http::response<http::string_body> bid(http::request<http::string_body> const& req,
                                          http::response<http::string_body>& res);
    http::response<http::string_body> buy(http::request<http::string_body> const& req,
                                          http::response<http::string_body>& res);
}
