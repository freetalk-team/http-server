#include "utils.h"


#include <jwt-cpp/jwt.h>
#include <cmark.h>


#include <vector>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <cctype>




std::optional<std::string>
decode_jwt(const std::string& token, const std::string& secret, const std::string& name) {
	
	try {
		// Replace `your_secret_key` with the same secret used for signing the JWT
		//const std::string secret_key = "your_secret_key";

		// Decode the JWT token using the secret key
		auto decoded = jwt::decode(token);

		auto verifier = jwt::verify()
			.allow_algorithm(jwt::algorithm::hs256{secret});

		verifier.verify(decoded); // Throws if verification fails

		return decoded.get_payload_claim(name).as_string();
	
	} catch (const jwt::error::token_verification_exception& e) {
		std::cerr << "Error decoding JWT: " << e.what() << std::endl;
	}

	return {};
}

bool match_jwt(
	const std::string& token, 
	const std::string& secret_key, 
	const std::string& key,
	const std::string& value) 
{
	auto r = decode_jwt(token, secret_key, key);
	return r && *r == value;
}




// bool match_auth_token(
// 	std::string_view cookie, 
// 	const std::string& key, 
// 	const std::string& value) 
// {
// 	auto cookies = parse_cookies(cookie);

// 	auto auth = cookies.find("AuthToken");
// 	if (auto auth = cookies.find("AuthToken"); auth != cookies.end()) {

// 		auto r = decode_jwt(auth->second, SESSION_SECRET, "role");
// 		if (r && *r == route.role) {
// 			return put(route, body, type);
// 		}
// 	}
// }

std::optional<std::string> get_env_var(const std::string& key) {
	const char* val = std::getenv(key.c_str());
	if (val == nullptr) {
		return std::nullopt; // environment variable not found
	}
	return std::string(val);
}


std::string get_env_var(const std::string& key, const std::string& def) {
	auto v = get_env_var(key);
	return v ? *v : def;
}
