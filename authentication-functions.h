#pragma once

#include <string>
#include <nlohmann/json.hpp>
#include <jwt-cpp/jwt.h>

using namespace std;

constexpr auto jwtExpireTime = chrono::seconds(60 * 60 * 1000); // 1 hour

void initialize_auth();
tuple<string, string> hash(const string& data, const string& salt);
string generateAccessToken(const nlohmann::basic_json<>& data);
nlohmann::basic_json<> verifyToken(const string& token);
