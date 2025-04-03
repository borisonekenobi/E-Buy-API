#include "authentication-functions.h"

#include <fstream>
#include <iostream>

// #include <jwt-cpp/jwt.h>

#include <openssl/rand.h>
#include <openssl/sha.h>

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

string generate_salt()
{
	constexpr auto length = 32;
	vector<unsigned char> salt(length);

	if (RAND_bytes(salt.data(), length) != 1)
		throw runtime_error("Error generating random bytes");

	stringstream ss;
	for (size_t i = 0; i < length; i++)
		ss << hex << setw(2) << setfill('0') << static_cast<int>(salt[i]);

	return ss.str();
}

string sha256(const std::string& input)
{
	unsigned char hash[SHA256_DIGEST_LENGTH];
	EVP_MD_CTX* context = EVP_MD_CTX_new();
	EVP_DigestInit_ex(context, EVP_sha256(), nullptr);
	EVP_DigestUpdate(context, input.c_str(), input.size());
	EVP_DigestFinal_ex(context, hash, nullptr);
	EVP_MD_CTX_free(context);

	stringstream ss;
	for (const unsigned char i : hash)
		ss << hex << setw(2) << setfill('0') << static_cast<int>(i);
	return ss.str();
}

// Hashes the given data with the given salt.
string hash(const string& data, string& salt)
{
	if (salt.empty())
		salt = generate_salt();

	return sha256(data + salt);
}

// Generates an access token for the given data.
string generate_access_token(const nlohmann::basic_json<>& data)
{
	return jwt::create()
	       .set_type(env_map["TOKEN_TYPE"])
	       .set_issuer(env_map["TOKEN_ISSUER"])
	       .set_expires_at(jwt::date::clock::now() + jwtExpireTime)
	       .set_payload_claim("data", jwt::claim(data.dump()))
	       .sign(jwt::algorithm::hs256(env_map["TOKEN_SECRET"]));
}

string generate_refresh_token(const nlohmann::basic_json<>& data)
{
	return jwt::create()
	       .set_type(env_map["TOKEN_TYPE"])
	       .set_issuer(env_map["TOKEN_ISSUER"])
	       .set_expires_at(jwt::date::clock::now() + jwtRefreshExpireTime)
	       .set_payload_claim("data", jwt::claim(data.dump()))
	       .sign(jwt::algorithm::hs256(env_map["TOKEN_SECRET"]));
}

// Verifies the given token.
nlohmann::basic_json<> verify_token(const string& token)
{
	try {
		const auto decoded_token = jwt::decode(token);
		const auto verifier = jwt::verify()
							  .with_type(env_map["TOKEN_TYPE"])
							  .with_issuer(env_map["TOKEN_ISSUER"])
							  .allow_algorithm(jwt::algorithm::hs256(env_map["TOKEN_SECRET"]));
		verifier.verify(decoded_token);

		return nlohmann::json::parse(decoded_token.get_payload_json()["data"].get<string>());
	}
	catch (...)
	{
		return nlohmann::json();
	}
}
