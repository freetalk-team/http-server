
#include "router.h"
#include "context.h"
#include "utils.h"

#include <print>

const auto SESSION_SECRET = get_env_var("SESSION_SECRET", "your_secret_key");

Router::Router() {}

Router::~Router() {}

Router& Router::get() {
	static Router r;
	return r;
}

bool Router::init(const char* path, const std::string& root, int pool_size) {
	ctx.reset(new Router::Context(pool_size));

	return ctx->init(path, root);
}

void Router::cleanup() {
	std::println("Router cleanup!");

	ctx->cleanup();
}

Router::ReturnType Router::get(const std::string& path, const Headers& hdr, bool mobile) {

	Context::QueryParams qp;

	if (!qp.parse(path)) {
		std::println("Failed to execute path: {0}", path);
		return { 400, "Invalid url", "text/plain" };
	}

	std::println("URI: {0}", qp.route);

	auto r = ctx->find(Router::GET, qp.route);
	if (!r.route)
		return { 404, "Not found" };

	if (!hdr.token.empty()) {

		if (!qp.parse_auth_token(hdr.token, SESSION_SECRET))
			return { 400, "Invalid token", "text/plain" };
	}
	else if (!hdr.key.empty()) {
		if (!qp.parse_auth_header(hdr.key))
			return { 400, "Invalid auth", "text/plain" };
	}

	std::println("Executing GET => {0}", path);

	qp.params = std::move(r.params);
	qp.mobile = mobile;

	return ctx->get(*r.route, qp);

}

Router::ReturnType Router::post(const std::string& path, const std::string& body, Payload type, const Headers& hdr) {

	Context::QueryParams qp;

	if (!qp.parse(path)) {
		std::println("Failed to execute path: {0}", path);
		return { 400, "Invalid url", "text/plain" };
	}

	std::println("BODY:\n{0}", body);
	std::println("URI: {0}", qp.route);

	auto r = ctx->find(Router::POST, qp.route);
	if (!r.route)
		return { 404, "Not found" };

	if (!hdr.token.empty()) {

		if (!qp.parse_auth_token(hdr.token, SESSION_SECRET))
			return { 400, "Invalid token", "text/plain" };
	}
	else if (!hdr.key.empty()) {
		if (!qp.parse_auth_header(hdr.key))
			return { 400, "Invalid auth", "text/plain" };
	}

	qp.params = std::move(r.params);
		//qp.mobile = mobile;

	return ctx->post(*r.route, qp, body, type);
}

Router::ReturnType Router::remove(const std::string& path, const Headers& hdr) {

	Context::QueryParams qp;

	if (!qp.parse(path)) {
		std::println("Failed to execute path: {0}", path);
		return { 400, "Invalid url", "text/plain" };
	}

	std::println("URI: {0}", qp.route);

	auto r = ctx->find(Router::DELETE, qp.route);
	if (!r.route)
		return { 404, "Not found" };

	if (!hdr.token.empty()) {
		if (!qp.parse_auth_token(hdr.token, SESSION_SECRET))
			return { 400, "Invalid token", "text/plain" };
	}
	else if (!hdr.key.empty()) {
		if (!qp.parse_auth_header(hdr.key))
			return { 400, "Invalid auth", "text/plain" };
	}

	std::println("Executing POST => {0}",path);

	qp.params = std::move(r.params);
	//qp.mobile = mobile;

	return ctx->remove(*r.route, qp);

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


