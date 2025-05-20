#include "json.h"
#include "db.h"
#include "utils.h"

#include <soci/soci.h>

#include <print>
#include <sstream>

using ReturnType = Executor::ReturnType;

const std::string_view 
	JSON = "application/json",
	PLAIN = "text/plain";

ReturnType ExecutorJson::get(const Route& route, const QueryParams& qp) {

	auto const& path = qp.path; 

	if (route.cache > 0) {

		auto c = cache.get(path);

		if (c) {
			std::println("Found in cache: {0}", path);

			return { 200, std::move(c->body), JSON, c->ts, c->expires };
		}
	}

	soci::session db(pool);

	auto r = qp.params.size() > 0 ? get(db, route, qp) : ls(db, route, qp);

	if (r.status < 300 && route.cache > 0) {

		auto res = cache.put(path, r.body, r.type, route.cache);

		r.ts = res.ts;
		r.expires = res.expires;
	}

	return std::move(r);
}

ReturnType ExecutorJson::ls(soci::session& db, const Route& route, const QueryParams& qp) {

	auto& table = route.db.table;

	int limit = route.db.limit, offset = qp.query.offset;
	
	auto rs = Executor::ls(db, route, qp);
	auto res = to_json(rs);

	return { 200, gzip_compress(res), JSON };
}

ReturnType ExecutorJson::put(const Route& route, const std::string& body, const QueryParams& qp) {
	auto& table = route.db.table;

	soci::session db(pool);

	auto res = db::insert(db, table, body);

	if (!res) 
		return { 400, "Bad request", PLAIN };

	std::ostringstream os;

	os << "{\"id\":\"" << *res << "\"}";

	return { 200, gzip_compress(os.str()), JSON };
}

ReturnType ExecutorJson::get(soci::session& db, const Route& route, const QueryParams& qp) {

	auto& table = route.db.table;
	auto& attributes = route.db.attributes;
	auto& type = route.db.type;

	auto r = db::get(db, table, qp.params, attributes);
	if (!r)
		return { 404, "Not found", PLAIN };


	auto& row = *r;

	if (row.size() == 1 && !type.empty()) {

		if (row.get_indicator(0) == soci::i_null)
			return { 404, "null", PLAIN };

		const soci::column_properties& props = row.get_properties(0);

		if (props.get_data_type() == soci::dt_blob)
			return { 200, db::blob(row), type };
	}

	auto res = to_json(row);

	std::println("JSON response:\n{0}", res);

	return { 200, gzip_compress(res), JSON };
}


void ExecutorJson::cleanup() {
	cache.cleanup();
}