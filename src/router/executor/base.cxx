#include "base.h"
#include "utils.h"
#include "db.h"

#include <jwt-cpp/jwt.h>
#include <soci/soci.h>

#include <boost/url.hpp>

#include <iostream>
#include <print>
#include <sstream>
#include <charconv>
#include <regex>

namespace {

std::optional<int> parse_int(const std::string& s) {
	int value;
	auto [ptr, ec] = std::from_chars(s.data(), s.data() + s.size(), value);
	if (ec == std::errc() && ptr == s.data() + s.size()) {
		return value;
	}
	return std::nullopt; // Failed to parse
}

Executor::Query parse_query(const std::string& query) {
	Executor::Query result;

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

bool match_true(const std::string& rule, const Params&, const Executor::AuthInfo&) { 
	return true;
}

bool match_role(const std::string& rule, const Params&, const Executor::AuthInfo& auth) { 
	if (auth.role == "su")
		return true;

	return auth.role == rule; 
}

bool match_rule(const std::string& rule, const Params& params, const Executor::AuthInfo& auth) { 
	if (auth.role == "su")
		return true;

	std::regex ruleRegex(R"(^([a-zA-Z0-9_]+)=auth\.([a-zA-Z0-9_]+)$)");
	 std::smatch match;

	if (std::regex_match(rule, match, ruleRegex)) {
		std::string param = match[1]; // e.g., "id"
		std::string authField = match[2]; // e.g., "uid"

		if (auto p = params.find(param); p != params.end()) {

			std::string authValue;

			// Access the corresponding field in auth
			if (authField == "uid") authValue = auth.uid;
			else if (authField == "name") authValue = auth.name;
			else if (authField == "email") authValue = auth.email;
			else if (authField == "role") authValue = auth.role;
			else return false;

			return p->second == authValue;
		}
	}

	return false; 
}



} // namespace

Executor::Route::Route(const std::string& path) :
	path (path),
	rule_matcher(match_true)
{}

void Executor::Route::set_role(const std::string& r) {

	role = r;

	if (r.find('=') != std::string::npos)
		rule_matcher = match_rule;
	else
		rule_matcher = match_role;
	
}


Executor::Executor(soci::connection_pool& pool) : pool(pool) {

}

bool Executor::QueryParams::parse(const std::string& url) {
	auto ec = boost::urls::parse_origin_form(url);

	if (ec.has_error()) {
		// todo

		std::println(std::cerr, "Failed to parse path: {0}", url);
		return false;
	}

	auto u = ec.value();

	path = url;
	route = u.encoded_path().decode();
	query = parse_query(u.encoded_query().decode());
	
	return true;
}

bool Executor::del(soci::session& db, const std::string& table, const std::string& id) {
	return db::del(db, table, id);
}

soci::rowset<soci::row> Executor::ls(soci::session& db, const Route& route, const QueryParams& qp) {

	int limit = route.db.limit, offset = qp.query.offset;
	auto& table = route.db.table;

	if (!route.db.fts)
		return db::ls(db, table, qp.query, offset, limit);

	auto& fts = *route.db.fts;
	auto& q = qp.query;

	if (auto index = q.find(fts.index); index != q.end() && !index->second.empty()) 
		return db::search(db, table, fts.table, fts.rank, index->second, offset, limit);

	// return db::ls(db, table, offset, limit);
	return db::ls(db, table, qp.query, offset, limit);
}

bool Executor::AuthInfo::parse(const std::string& token, const std::string& secret) {
	try {
		auto decoded = jwt::decode(token);

		auto verifier = jwt::verify()
			.allow_algorithm(jwt::algorithm::hs256{secret})
			//.with_issuer("auth0")
			; // optional: expected issuer

		verifier.verify(decoded); // Throws if verification fails

		role   = decoded.get_payload_claim("r").as_string();
		uid    = decoded.get_payload_claim("u").as_string();
		name   = decoded.get_payload_claim("n").as_string();
		email  = decoded.get_payload_claim("e").as_string();
		expire = decoded.get_payload_claim("x").as_integer();

		return true;
	} 
	catch (const jwt::error::token_verification_exception& e) {
		std::cerr << "Error decoding JWT: " << e.what() << std::endl;
	}

	return false;
}

bool Executor::QueryParams::parse_auth_token(const std::string& token, const std::string& secret) {
	return auth.emplace().parse(token, secret);
}

bool Executor::QueryParams::parse_auth_header(const std::string& header) {
	api_key = header; // todo
	return true;
}