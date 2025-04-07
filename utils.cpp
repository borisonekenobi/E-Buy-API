#include "utils.h"

#include <boost/uuid/string_generator.hpp>

#include <nlohmann/json.hpp>

#include "authentication-functions.h"

using namespace std;

// Checks if the given string is a valid UUID
bool is_valid_uuid(const string& str, boost::uuids::uuid& result)
{
    try
    {
        result = boost::uuids::string_generator()(str);
        return result.version() == boost::uuids::uuid::version_random_number_based;
    }
    catch (...)
    {
        return false;
    }
}

bool is_valid_uuid(const string& str)
{
    boost::uuids::uuid result;
    return is_valid_uuid(str, result);
}

// Checks if the token data is correct
bool is_valid_token_data(const nlohmann::basic_json<>& data)
{
    return
        !data.empty() &&
        data.contains("id") &&
        is_valid_uuid(data["id"].get<string>()) &&
        data.contains("name") &&
        data.contains("username") &&
        data.contains("type") &&
        data.contains("status");
}

bool is_valid_renew_data(const nlohmann::basic_json<>& data)
{
    return
        !data.empty() &&
        data.contains("type") &&
        data["type"].get<string>() == "refresh" &&
        data.contains("user") &&
        data["user"].contains("id") &&
        is_valid_uuid(data["user"].get<nlohmann::json>()["id"].get<string>()) &&
        data["user"].contains("name") &&
        data["user"].contains("username") &&
        data["user"].contains("type") &&
        data["user"].contains("status");
}

// Checks if the body of the request is malformed
bool is_malformed_body(const string& req_body, const vector<string>& required_fields, string& err_message,
                       nlohmann::json& body)
{
    if (req_body.empty())
    {
        err_message = R"({"message": "Request body is empty"})";
        return true;
    }

    try
    {
        body = nlohmann::json::parse(req_body);
    }
    catch (...)
    {
        err_message = R"({"message": "Invalid JSON format"})";
        return true;
    }

    bool malformed = false;
    string joined;
    for (int i = 0; i < required_fields.size(); ++i)
    {
        joined += required_fields[i];
        if (i != required_fields.size() - 1) joined += ", ";
        if (!body.contains(required_fields[i])) malformed = true;
    }

    if (malformed)
    {
        err_message = R"({"message": "Missing one or many required fields: )" + joined + R"("})";
        return true;
    }

    return false;
}

#define BEARER_PREFIX "Bearer "
#define BEARER_PREFIX_LENGTH 7

// Checks if the authorization header is malformed (access token)
bool is_malformed_auth(const string& auth_header, string& err_message, nlohmann::json& data)
{
    if (auth_header.empty())
    {
        err_message = R"({"message": "Authorization header is missing"})";
        return true;
    }

    if (!auth_header.starts_with(BEARER_PREFIX))
    {
        err_message = R"({"message": "Invalid authorization header format"})";
        return true;
    }

    data = verify_token(auth_header.substr(BEARER_PREFIX_LENGTH));
    if (!is_valid_token_data(data))
    {
        err_message = R"({"message": "Invalid or expired token"})";
        return true;
    }

    return false;
}

// Checks if the authorization header is malformed (refresh token)
bool is_malformed_renew(const string& auth_header, string& err_message, nlohmann::json& data)
{
    if (auth_header.empty())
    {
        err_message = R"({"message": "Authorization header is missing"})";
        return true;
    }

    if (!auth_header.starts_with(BEARER_PREFIX))
    {
        err_message = R"({"message": "Invalid authorization header format"})";
        return true;
    }

    data = verify_token(auth_header.substr(BEARER_PREFIX_LENGTH));
    if (!is_valid_renew_data(data))
    {
        err_message = R"({"message": "Invalid or expired token"})";
        return true;
    }

    return false;
}

#define MAX_PRICE 1000000.00

bool is_valid_price(double& result, string& err_message)
{
    if (result < 0)
    {
        err_message = R"({"message": "Price cannot be negative"})";
        return false;
    }
    if (result > MAX_PRICE)
    {
        err_message = R"({"message": "Price cannot exceed 1,000,000"})";
        return false;
    }

    result = round(result * 100) / 100.0;
    return true;
}

bool is_valid_price(const string& inp, string& err_message, double& result)
{
    try
    {
        result = stod(inp);
    }
    catch (const invalid_argument&)
    {
        err_message = R"({"message": "Invalid price format"})";
        return false;
    }

    return is_valid_price(result, err_message);
}

bool is_valid_price(const nlohmann::basic_json<>& price, string& err_message, double& result)
{
    if (price.is_string()) return is_valid_price(price.get<string>(), err_message, result);
    if (price.is_number())
    {
        result = price.get<double>();
        return is_valid_price(result, err_message);
    }
    err_message = R"({"message": "Price must be a number"})";
    return false;
}

void prepare_response(http::response<http::string_body>& res, const http::status status, const string& body)
{
    res.result(status);
    res.body() = body;
    res.prepare_payload();
}
