#include "context.h"

#include <nlohmann/json.hpp>

#include <boost/url.hpp>

#include <optional>
#include <sstream>
#include <regex>
#include <list>
#include <print>
#include <iostream>
#include <format>

#include "router.h"
#include "config.h"
#include "functions.h"
#include "utils.h"
#include "db.h"

using Json = nlohmann::json;

using Params = std::map<std::string, std::string>;
using Variant = std::variant<int, double, std::string>;
using Row = std::vector<Variant>;

using Context = Router::Context;
using Route = Context::Route;
using Result = Context::Result;
using ReturnType = Router::ReturnType;
using AuthInfo = Context::AuthInfo;
using Query = Context::Query;
using QueryParams = Context::QueryParams;

using FormParams = Params;

const std::string_view 
	PLAIN = "text/plain",
	HTML  = "text/html",
	JSON  = "application/json";

namespace {

void dumpr(const Route& r) {
	std::println("DB => table: {0}, op: {1}", r.db.table, r.db.op);
}

bool match_role(const Route& route, const AuthInfo& auth, const Params& params) {
	//return auth.role == "su" || role == auth.role;
	return route.rule_matcher(route.role, params, auth);
}

bool match_api_key(const std::string& api_key, const std::string& key) {
	return api_key == key;
}

/*

template <typename... Args>
decltype(auto) myfunction(ExecutorHtml& exec, Args&&... args) {
	decltype(auto) r = exec.get(std::forward<Args>(args)...);

	// Do something with r (e.g., logging, inspection)

	return r;  // Return as-is, preserves reference/value type
}

template <typename... Args>
auto myfunction(ExecutorHtml& exec, Args&&... args) {
	auto r = exec.get(std::forward<Args>(args)...);

	// ...

	return std::move(r);  // Return by value, moving r
}

*/

template <typename... Args>
auto get_html(const Context::Errors& errors, ExecutorHtml& html, Args&&... args) {
	auto r = html.get(std::forward<Args>(args)...);

	if (r.status >= 400) {

		if (auto e = errors.find(r.status); e != errors.end()) {

			Route route(e->second);

			route.ejs = e->second;
			route.cache = 10 * 60;

			return html.get(route);
		}
	}

	// todo: check cookie expire


	return r;  // Returning a copy/move
}

} // namespace

Router::Context::Context(int pool_size) :
	pool(pool_size),
	html(pool, views_dir),
	json(pool)
{
	db::register_backends();

	for (int i = 0; i < METHOD_COUNT; ++i)
		root[i].reset(new Node());

}

Router::Context::~Context()
{}

void Router::Context::set(const std::string& var, const std::string& val) {
	vars[var] = val;
}

bool Router::Context::init(const char* path, const std::string& root) {
	Script config;

	register_http_functions(config, this);

	wString data;
	data.load_from_file(path);

	try {
		config.evaluate(data);
	} catch (const CScriptException* e) {
		std::println(stderr, "Failed to load config: {}", e->text.c_str());
		return false;
	}

	// todo: improve
	if (root.ends_with("/")) {
		public_dir = root + public_dir;
		views_dir = root + views_dir;
	}
	else {
		public_dir = root + "/" + public_dir;
		views_dir = root + "/" + views_dir;
	}

	if (!views_dir.ends_with('/'))
		views_dir += '/';
	// router.dump();

	// don't need variables after config loading
	vars.clear();

	return true;
}

Route& Router::Context::addRoute(Router::Method method, const std::string& path) {

	std::istringstream ss(path);
	std::string segment;

	auto node = root[method].get();

	while (std::getline(ss, segment, '/')) {
		if (segment.empty()) continue;

		if (segment[0] == ':') {
			if (!node->param_child) {
				node->param_child.reset(new Node());
				node->param_child->param_name = segment.substr(1);
			}
			node = node->param_child.get();
		} else {
			node = node->children.try_emplace(segment, new Node()).first->second.get();
		}
	}

	return node->route.emplace(path);

	// if (route.db.table.length() > 0)
	// 	addTable(route.db.table);

	// auto& r = router[method];
	// r.insert({ path, route });
}

bool Router::Context::match(Router::Method method, const std::string& path) const {
	return find(method, path).route != nullptr;
}

void Router::Context::cleanup() {
	html.cleanup();
	json.cleanup();
}

ReturnType Router::Context::get(const Route& route, const QueryParams& qp) {
	
	dumpr(route);

	auto const& path = qp.path; 

	if (!route.role.empty()) {

		if (!qp.auth || !match_role(route, qp.auth.value(), qp.params))
			return { 403, "Access forbidden", PLAIN };
		else if (!route.api_key.empty() || !match_api_key(route.api_key, qp.api_key))
			return { 403, "Access forbidden", PLAIN };
	}
	// else if (!cookie.empty()) {
	// 	decode_cookie(cookie, SESSION_SECRET, info);
	// }

	return route.ejs.empty() ? json.get(route, qp) : get_html(errors, html, route, qp);
}

ReturnType Router::Context::post(
	const Route& route, 
	const QueryParams& qp, 
	const std::string& body,
	Payload type) 
{

	if (!route.role.empty()) {

		if (!qp.auth || !match_role(route, qp.auth.value(), qp.params))
			return { 403, "Access forbidden", PLAIN };
	}

	return type == Payload::FORM 
		? html.put(route, body, qp)
		: json.put(route, body, qp);
}

ReturnType Router::Context::remove(const Route& route, const QueryParams& qp) 
{
	if (!route.role.empty()) {

		if (!qp.auth || !match_role(route, qp.auth.value(), qp.params))
			return { 403, "Access forbidden", PLAIN };
	}

	auto& params = qp.params;

	if (auto id = params.find("id"); id != params.end()) {

		try {

			soci::session db(pool);

			if (!Executor::del(db, route.db.table, id->second)) {
				return { 404, "Not found", PLAIN };
			}

			return { 200 };

		} catch (const std::runtime_error& e) {
			return { 500, "Failed to delete" , PLAIN };
		}
	}

	return { 400, "Bad request", PLAIN };
}

Result Router::Context::find(Router::Method method, const std::string& path) const {
	std::istringstream ss(path);
	std::string segment;
	Params params;

	auto node = root[method].get();

	while (std::getline(ss, segment, '/')) {
		if (segment.empty()) continue;

		if (node->children.count(segment)) {
			node = node->children.at(segment).get();
		} else if (node->param_child) {
			params[node->param_child->param_name] = segment;
			node = node->param_child.get();
		} else {
			// std::cout << "404 Not Found: " << path << "\n";
			return { nullptr };
		}
	}

	if (node && node->route) {
		return { &node->route.value(), std::move(params) };
	}
	
	return { nullptr };
}

void Router::Context::connectDatabase(const std::string& url) {

	static const std::regex url_regex(R"((\w+)://(.*))");

	std::smatch match;
	if (!std::regex_match(url, match, url_regex) || match.size() != 3) {
		throw std::invalid_argument("Invalid DB URL: " + url);
	}

	const std::string scheme = match[1];
	const std::string conn = match[2];

	// if (scheme == "sqlite") {
	// 	db.open(soci::sqlite3, conn);
	// }
	// else if (scheme == "pg" || scheme == "postgres" || scheme == "postgresql") {
	// 	db.open(soci::postgresql, conn);
	// }
	// else {
	// 	throw std::invalid_argument("Unsupported DB scheme: " + scheme);
	// }

	for (std::size_t i = 0; i < pool.size(); ++i) {
		//pool.at(i).open(soci::sqlite3, "test.db");  // or PostgreSQL/MySQL
		pool.at(i).open(url);
	}


	//db.set_log_stream(&std::cout);             // Output logs to std::cout

	

}

// std::string Router::Context::templatePath(const std::string& path) const {
// 	return views_dir + path;
// }

void Router::Context::database(const std::string& url) {
	try {

		// ctx->sql.reset(new soci::session(url));
		connectDatabase(url);

	}
	catch (soci::soci_error const& e) {
		std::cerr << "Connection to \"" << url << "\" failed: "
				  << e.what() << "\n";
	}
}
