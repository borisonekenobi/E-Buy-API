#pragma once

#include <string>

#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>

#include <nlohmann/json.hpp>

using namespace std;

inline boost::uuids::random_generator gen_uuid;
bool is_valid_uuid(const string& str, boost::uuids::uuid& result);
bool is_valid_token_data(const nlohmann::basic_json<>& data);
