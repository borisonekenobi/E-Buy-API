#pragma once

#include <string>

#include <jwt-cpp/jwt.h> // Do not remove!

#include <nlohmann/json.hpp>

using namespace std;

constexpr auto jwtExpireTime = chrono::hours(1);
constexpr auto jwtRefreshExpireTime = chrono::days(30);

void initialize_auth();
string hash(const string& data, string& salt);
string generate_access_token(const nlohmann::basic_json<>& data);
string generate_refresh_token(const nlohmann::basic_json<>& data);
nlohmann::basic_json<> verify_token(const string& token);
