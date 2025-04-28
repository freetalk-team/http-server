
#include "router.h"
#include "context.h"

#include <boost/url.hpp>

#include <charconv>
#include <print>

Router::Context::Query parse_query(const std::string& query);


Router::Router() {}

Router::~Router() {

}

bool Router::init(const char* path, const std::string& root, int pool_size) {
	ctx.reset(new Router::Context(pool_size));

	return ctx->init(path, root);
}

void Router::cleanup() {
	std::println("Router cleanup!");

	ctx->cleanup();
}

Router::ReturnType Router::get(const std::string& path, bool mobile) {

	auto ec = boost::urls::parse_origin_form(path);

	if (ec.has_error()) {
		// todo

		std::println("Failed to execute path: {0}", path);
		return { 400, "Invalid url" };
	}

	auto url = ec.value();
	auto route = url.encoded_path().decode();

	std::println("URI: {0}", route);

	auto r = ctx->find(Router::GET, route);

	if (r.route != nullptr) {
		std::println("Executing GET => {0}", path);

		Context::QueryParams params = { 
			path, 
			std::move(route),
			std::move(r.params), 
			parse_query(url.encoded_query().decode()),
			mobile
		};

		return ctx->get(*r.route, params);
	}

	return { 404, "Not found" };
}

Router::ReturnType Router::post(const std::string& path, const std::string& body) {

	std::println("BODY:\n{0}", body);

	auto ec = boost::urls::parse_origin_form(path);

	if (ec.has_error()) {
		// todo

		std::println("Failed to execute path: {0}", path);
		return { 400, "Invalid url" };
	}

	auto url = ec.value();
	auto route = url.encoded_path().decode();
	auto query = url.encoded_query().decode();

	std::println("URI: {0}, {1}", route, query);

	auto r = ctx->find(Router::POST, path);

	if (r.route != nullptr) {
		std::println("Executing POST => {0}",path);

		auto& route = *r.route;
		auto& params = r.params;

		for (auto& [name, value] : params)
			std::println("  {0} => {1}", name, value);

		return ctx->post(*r.route, r.params, body);
	}

	return { 404, "Not found" };
}

void Router::dump() const {


	// for (auto& r : router) {

	// 	std::println("Routes: {0}", r.size());

	// 	for (auto& i : r) {

	// 		auto& path = i.first;
	// 		auto& route = i.second;

	// 		std::println("## {0} => ", path);
	// 		std::println("DB: table: {0}, op: {1}", route.db.table, route.db.op);
	// 	}
	// }

}

std::string const& Router::publicDir() const {
	return ctx->public_dir;
}

std::string Router::method(Method m) {

	static const std::string methods[] = {
		"GET", "POST", "DELETE"
	};

	return methods[m];
} 

std::optional<int> parse_int(const std::string& s) {
	int value;
	auto [ptr, ec] = std::from_chars(s.data(), s.data() + s.size(), value);
	if (ec == std::errc() && ptr == s.data() + s.size()) {
		return value;
	}
	return std::nullopt; // Failed to parse
}

Router::Context::Query parse_query(const std::string& query) {
	Router::Context::Query result;

	std::istringstream ss(query);
	std::string pair;

	while (std::getline(ss, pair, '&')) {
		size_t eq_pos = pair.find('=');
		if (eq_pos != std::string::npos) {
			auto key = pair.substr(0, eq_pos);
			auto value = pair.substr(eq_pos + 1);

			if (key == "o") {

				auto offset = parse_int(value);

				if (offset)
					result.offset = *offset;
			}
			else {
				result[key] = std::move(value);
			}
		
		}
	}

	return std::move(result);
}
