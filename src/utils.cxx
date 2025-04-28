#include "utils.h"

#include <boost/beast.hpp>
#include <jwt-cpp/jwt.h>

#include <fstream>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <print>

namespace beast = boost::beast;
namespace fs = std::filesystem;


beast::string_view
mime_type(beast::string_view path)
{
	using beast::iequals;
	auto const ext = [&path]
	{
		auto const pos = path.rfind(".");
		if(pos == beast::string_view::npos)
			return beast::string_view{};
		return path.substr(pos);
	}();

	if (ext.size() == 0) { return ""; }

	auto type = ext.substr(1);
	
	if(iequals(type, "htm"))   return "text/html";
	if(iequals(type, "html"))  return "text/html";
	if(iequals(type, "php"))   return "text/html";
	if(iequals(type, "css"))   return "text/css";
	if(iequals(type, "txt"))   return "text/plain";
	if(iequals(type, "js"))    return "application/javascript";
	if(iequals(type, "json"))  return "application/json";
	if(iequals(type, "xml"))   return "application/xml";
	if(iequals(type, "swf"))   return "application/x-shockwave-flash";
	if(iequals(type, "flv"))   return "video/x-flv";
	if(iequals(type, "png"))   return "image/png";
	if(iequals(type, "jpe"))   return "image/jpeg";
	if(iequals(type, "jpeg"))  return "image/jpeg";
	if(iequals(type, "jpg"))   return "image/jpeg";
	if(iequals(type, "gif"))   return "image/gif";
	if(iequals(type, "bmp"))   return "image/bmp";
	if(iequals(type, "ico"))   return "image/vnd.microsoft.icon";
	if(iequals(type, "tiff"))  return "image/tiff";
	if(iequals(type, "tif"))   return "image/tiff";
	if(iequals(type, "svg"))   return "image/svg+xml";
	if(iequals(type, "svgz"))  return "image/svg+xml";
	if(iequals(type, "ttf"))   return "font/ttf";
	if(iequals(type, "woff2")) return "font/woff2";

	return "application/octet-stream";
}

bool is_mobile_user_agent(std::string_view user_agent) {
	return user_agent.find("Mobile") != std::string_view::npos ||
		   user_agent.find("Android") != std::string_view::npos ||
		   user_agent.find("iPhone") != std::string_view::npos ||
		   user_agent.find("iPad") != std::string_view::npos ||
		   user_agent.find("iPod") != std::string_view::npos ||
		   user_agent.find("Opera Mini") != std::string_view::npos ||
		   user_agent.find("IEMobile") != std::string_view::npos;
}


std::string
path_cat(beast::string_view base, beast::string_view path) {
	if(base.empty())
		return std::string(path);
	std::string result(base);

	char constexpr path_separator = '/';
	if(result.back() == path_separator)
		result.resize(result.size() - 1);
	result.append(path.data(), path.size());

	return result;
}

// std::string compute_etag_content(const fs::path& file_path) {
// 	std::ifstream file(file_path, std::ios::binary);
// 	if (!file) return "";

// 	std::ostringstream oss;
// 	oss << file.rdbuf();
// 	std::string content = oss.str();

// 	unsigned char hash[SHA256_DIGEST_LENGTH];
// 	SHA256(reinterpret_cast<const unsigned char*>(content.data()), content.size(), hash);

// 	std::ostringstream etag;
// 	etag << '"';
// 	for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i)
// 		etag << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
// 	etag << '"';

// 	return etag.str();
// }

std::string 
compute_etag(int64_t ts, size_t size) {
	std::ostringstream oss;
	oss << '"' << size << '-' << ts << '"';
	return oss.str();
}

std::string compute_etag(const fs::path& file_path, size_t size) {
	std::error_code ec;
	auto ftime = fs::last_write_time(file_path, ec);
	if (ec) return "\"invalid-etag\"";

	auto time_since_epoch = ftime.time_since_epoch().count(); // use raw count

	return compute_etag(time_since_epoch, size);
}

std::string compute_etag(const fs::path& file_path) {

	std::error_code ec;
	auto size = fs::file_size(file_path, ec);
	if (ec) return "\"invalid-etag\"";

	return compute_etag(file_path, size);
}

// Function to decode and verify JWT
std::optional<std::string>
decode_jwt(const std::string& token, const std::string& secret_key, std::string& name) {
	try {
		// Replace `your_secret_key` with the same secret used for signing the JWT
		//const std::string secret_key = "your_secret_key";

		// Decode the JWT token using the secret key
		auto decoded_token = jwt::decode(token);

		return decoded_token.get_payload_claim(name).as_string();
	
	} catch (const jwt::error::token_verification_exception& e) {
		std::cerr << "Error decoding JWT: " << e.what() << std::endl;
	}

	return {};
}

std::map<std::string, std::string> parse_cookies(const std::string& cookie_header) {
	std::map<std::string, std::string> cookies;
	std::istringstream stream(cookie_header);
	std::string pair;
	while (std::getline(stream, pair, ';')) {
		auto separator = pair.find('=');
		if (separator != std::string::npos) {
			std::string key = pair.substr(0, separator);
			std::string value = pair.substr(separator + 1);
			key.erase(0, key.find_first_not_of(" \t"));
			key.erase(key.find_last_not_of(" \t") + 1);
			value.erase(0, value.find_first_not_of(" \t"));
			value.erase(value.find_last_not_of(" \t") + 1);
			cookies[key] = value;
		}
	}
	return cookies;
}