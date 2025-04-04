#pragma once

#include <string>

#include <boost/beast/http.hpp>
#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>

#include <nlohmann/json.hpp>

#define FIRST_OR_ONLY 0
#define INTERNAL_SERVER_ERROR R"({"message": "Internal server error"})"
#define DATABASE_ERROR "Database error"

namespace http = boost::beast::http;

inline boost::uuids::random_generator gen_uuid;
bool is_valid_uuid(const std::string& str, boost::uuids::uuid& result);
bool is_malformed_body(const std::string& req_body, const std::vector<std::string>& required_fields,
                       std::string& err_message, nlohmann::json& body);
bool is_malformed_auth(const std::string& auth_header, std::string& err_message, nlohmann::json& data);
bool is_malformed_renew(const std::string& auth_header, std::string& err_message, nlohmann::json& data);
bool is_valid_price(const nlohmann::basic_json<>& price, std::string& err_message, double& result);
http::response<http::string_body> prepare_response(http::response<http::string_body>& res, http::status status,
                                                   const std::string& body);
