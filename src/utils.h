#pragma once


#include <boost/beast/core/string_type.hpp>

#include <map>
#include <string_view>
#include <filesystem>

using Params = std::map<std::string, std::string>;

// Return a reasonable mime type based on the extension of a file.
boost::beast::string_view
mime_type(boost::beast::string_view path);

bool is_mobile_user_agent(std::string_view);

// Append an HTTP rel-path to a local filesystem path.
// The returned path is normalized for the platform
std::string
path_cat(boost::beast::string_view base, boost::beast::string_view path);

std::string 
compute_etag(const std::filesystem::path& file_path);

std::string 
compute_etag(const std::filesystem::path& file_path, size_t);

std::string 
compute_etag(int64_t ts, size_t);


Params parse_cookies(std::string_view header);