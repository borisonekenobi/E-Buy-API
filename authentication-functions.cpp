#include <fstream>

#include "authentication-functions.h"

map<string, string> env_map;

// Initializes the authentication functions by loading environment variables from a .env file.
void initialize_auth()
{
	ifstream env_file(".env");

	if (!env_file.is_open())
	{
		cerr << "Unable to open .env file" << endl;
		exit(EXIT_FAILURE);
	}

	string line;
	while (getline(env_file, line))
	{
		if (const size_t pos = line.find('='); pos != string::npos)
		{
			const string key = line.substr(0, pos);
			const string value = line.substr(pos + 1);
			env_map[key] = value;
		}
	}
	env_file.close();
}

// Hashes the given data with the given salt.
tuple<string, string> hash(const string& data, const string& salt = "")
{
	return tuple(data, salt);
}

// Generates an access token for the given data.
string generateAccessToken(const nlohmann::basic_json<>& data)
{
	cout << data.dump() << endl;

	return jwt::create()
	       .set_type(env_map["TOKEN_TYPE"])
	       .set_issuer(env_map["TOKEN_ISSUER"])
	       .set_expires_at(jwt::date::clock::now() + jwtExpireTime)
	       .set_payload_claim("data", jwt::claim(data.dump()))
	       .sign(jwt::algorithm::hs256(env_map["TOKEN_SECRET"]));
}

// Verifies the given token.
nlohmann::basic_json<> verifyToken(const string& token)
{
	const auto decoded_token = jwt::decode(token);
	const auto verifier = jwt::verify()
	                      .with_type(env_map["TOKEN_TYPE"])
	                      .with_issuer(env_map["TOKEN_ISSUER"])
	                      .allow_algorithm(jwt::algorithm::hs256(env_map["TOKEN_SECRET"]));
	verifier.verify(decoded_token);

	return nlohmann::json::parse(decoded_token.get_payload_json()["data"].get<string>());
}
