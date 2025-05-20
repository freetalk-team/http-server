#pragma once

#include "soci_fwd.h"

#include <string>
#include <map>
#include <memory>

using Params = std::map<std::string, std::string>;

struct StringDeleter {
	void operator()(char* ptr) const {
		std::free(ptr);
	}
};

typedef std::unique_ptr<char, StringDeleter> ScopedString;

std::string gzip_compress(const std::string& data);
std::string gzip_decompress(const std::string& data);

std::string to_json(const soci::row&);
std::string to_json(const soci::rowset<soci::row>&);


// Simple URL-decoding function
std::string url_decode(const std::string&);
Params      parse_form(const std::string&);



ScopedString markdown_to_html(const std::string& md);

