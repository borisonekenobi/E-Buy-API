#include "utils.h"

#include <boost/uuid/string_generator.hpp>

#include <nlohmann/json.hpp>

using namespace std;

bool is_valid_uuid(const string& str, boost::uuids::uuid& result)
{
    try
    {
        result = boost::uuids::string_generator()(str);
        return result.version() != boost::uuids::uuid::version_unknown;
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

// Checks if the token data is correct.
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
