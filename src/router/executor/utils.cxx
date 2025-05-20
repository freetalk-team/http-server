#include "utils.h"

#include <soci/soci.h>
#include <nlohmann/json.hpp>
#include <cmark.h>

#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/filter/gzip.hpp>

#include <sstream>

using Json = nlohmann::json;

std::string gzip_compress(const std::string& data) {
	std::ostringstream compressed;
	boost::iostreams::filtering_ostream out;
	out.push(boost::iostreams::gzip_compressor());
	out.push(compressed);
	out << data;
	boost::iostreams::close(out);
	return compressed.str();
}

std::string gzip_decompress(const std::string& data) {
	std::ostringstream compressed;
	boost::iostreams::filtering_ostream out;
	out.push(boost::iostreams::gzip_decompressor());
	out.push(compressed);
	out << data;
	boost::iostreams::close(out);
	return compressed.str();
}

Json row_to_json(const soci::row& row) {
	Json obj;

	for (std::size_t i = 0; i < row.size(); ++i) {
		const soci::column_properties& props = row.get_properties(i);
		const std::string& name = props.get_name();

		if (row.get_indicator(i) == soci::i_null) {
			obj[name] = nullptr;
		} else {
			switch (props.get_data_type()) {
				case soci::dt_string:
					obj[name] = row.get<std::string>(i);
					break;
				case soci::dt_double:
					obj[name] = row.get<double>(i);
					break;
				case soci::dt_integer:
					obj[name] = row.get<int>(i);
					break;
				case soci::dt_long_long:
					obj[name] = row.get<long long>(i);
					break;
				case soci::dt_unsigned_long_long:
					obj[name] = row.get<unsigned long long>(i);
					break;
				case soci::dt_date:
					// Format date as string
					{
						std::ostringstream oss;
						std::tm t = row.get<std::tm>(i);
						oss << std::put_time(&t, "%Y-%m-%d %H:%M:%S");
						obj[name] = oss.str();
					}
					break;
				default:
					obj[name] = "unsupported";
			}
		}
	}

	return std::move(obj);
}

Json rowset_to_json(const soci::rowset<soci::row>& rs) {
	Json result = Json::array();

	for (const auto& row : rs) {
		result.emplace_back(row_to_json(row));
	}

	return std::move(result);
}

std::string to_json(const soci::row& r) {
	return row_to_json(r).dump();
}


std::string to_json(const soci::rowset<soci::row>& rs) {
	return rowset_to_json(rs).dump();
}


Params parse_form(const std::string& body) {

	Params params;

	std::istringstream stream(body);
	std::string pair;

	while (std::getline(stream, pair, '&')) {
		auto pos = pair.find('=');
		if (pos != std::string::npos) {

			auto key = url_decode(pair.substr(0, pos));
			auto value = url_decode(pair.substr(pos + 1));

			if (value.empty())
				continue;

			params.emplace(std::move(key), std::move(value));
		}
	}

	return std::move(params);
}

// Simple URL-decoding function
std::string url_decode(const std::string& in) {
	std::string out;
	out.reserve(in.size());
	
	for (size_t i = 0; i < in.size(); ++i) {
		if (in[i] == '%') {
			if (i + 2 < in.size()) {
				std::istringstream iss(in.substr(i + 1, 2));
				int hex = 0;
				if (iss >> std::hex >> hex) {
					out += static_cast<char>(hex);
					i += 2;
				}
			}
		} else if (in[i] == '+') {
			out += ' ';  // '+' means space in application/x-www-form-urlencoded
		} else {
			out += in[i];
		}
	}
	return out;
}


ScopedString markdown_to_html(const std::string& md) {
	char* html = cmark_markdown_to_html(md.c_str(), md.length(), CMARK_OPT_DEFAULT);

	return ScopedString(html);
}