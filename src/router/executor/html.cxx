#include "html.h"
#include "db.h"
#include "ejs.h"
#include "utils.h"

#include <soci/soci.h>

#include <print>
#include <format>
#include <sstream>

using ReturnType = Executor::ReturnType;

const std::string_view 
	HTML = "text/html",
	PLAIN = "text/plain",
	EJS = "text/ejs";

namespace {

std::string html_error(const char* err) {
	return std::format(R"(<body><h1 style="color:red">ERROR:</h1><code>{}</code></body>)", err);
}

std::string build_query(const std::string& path, const Executor::Query& params, int offset) {

	if (params.size() == 0 && offset == 0)
		return path;

	std::ostringstream os;

	os << path << "?o=" << offset;

	for (auto& [name, value] : params)
		os << '&' << name << '=' << value;

	return os.str();
}

void encode_params(Params& params) {
	for (auto& [key, value] : params) {
		if (key == "md") {
			value = gzip_compress(value);
		}
	}
}

void add_auth_info(Ejs::Env const& env, const Executor::AuthInfo& auth) {

	auto user = env.object("user");

	user.var("uid", auth.uid)
		.var("name", auth.name)
		.var("email", auth.email)
		.var("role", auth.role);
}

} // namespace

ExecutorHtml::ExecutorHtml(soci::connection_pool& pool, const std::string& views_dir) :
	Executor(pool),
	views_dir(views_dir)
{}

ReturnType ExecutorHtml::get(const Route& route, const QueryParams& qp) {
	auto& path = qp.path; 

	auto c = cache.get(path);

	if (c) {
		std::println("Found in cache: {0}", path);

		return { 200, std::move(c->body), HTML, c->ts, c->expires };
	}

	ReturnType r;

	try {

		if (route.db.table.empty()) {

			r = qp.auth ? execute(route.ejs, qp.auth.value()) : execute(route.ejs);
		}
		else {
			soci::session db(pool);

			if (route.db.op == "schema")
				r = schema(db, route);
			else
				r = qp.params.size() > 0 ? get(db, route, qp) : ls(db, route, qp);
		}

	}
	catch (const std::runtime_error& e) {
		return { 500, html_error(e.what()), HTML };
	}

	if (r.status < 300 && route.cache > 0) {

		auto res = cache.put(path, r.body, r.type, route.cache);

		r.ts = res.ts;
		r.expires = res.expires;
	}

	return std::move(r);
}

ReturnType ExecutorHtml::put(const Route& route, const std::string& body, const QueryParams& qp) {
	auto& table = route.db.table;
	auto& bind = route.db.bind;

	soci::session db(pool);

	auto form = parse_form(body);

	// encode columns
	encode_params(form);

	for (auto& [name, value] : bind) {


		if (value.starts_with("auth.")) {

			if (!qp.auth)
				return { 400, "Bad request", PLAIN };

			auto& auth = qp.auth.value();

			std::string_view m(value.begin() + 5, value.end());

			if (m == "uid") 
				form[name] = auth.uid;
			else if (m == "name")
				form[name] = auth.name;
			else if (m == "email")
				form[name] = auth.email;
		}
		else {
			form[name] = value;
		}
	}

	auto res = db::insert(db, table, form);

	if (!res) 
		return { 400, "Bad request", PLAIN };

	return { 308, route.path + '/' + *res };
}

ReturnType ExecutorHtml::ls(soci::session& db, const Route& route, const QueryParams& qp) {
	auto ejs = loadScript(route.ejs);

	auto& auth = qp.auth;

	if (auth)
		add_auth_info(ejs, auth.value());

	return ls(ejs, db, route, qp);
}

ReturnType ExecutorHtml::ls(Ejs::Env& ejs, soci::session& db, const Route& route, const QueryParams& qp) {

	auto& table = route.db.table;
	auto& alias = route.db.alias;

	int status = 200, limit = route.db.limit, offset = qp.query.offset;
	std::string res;
	
	auto rs = Executor::ls(db, route, qp);
	auto& items = alias.empty() ? table : alias;


	ejs.var(items, rs);
	ejs.var("path", qp.path);

	auto len = ejs.length(items);

	if (offset > 0)
		ejs.var("prev", build_query(qp.route, qp.query, offset - limit > 0 ? offset - limit : 0));
	else
		ejs.var("prev", 0);
	

	if (len == limit)
		ejs.var("next", build_query(qp.route, qp.query, offset + limit));
	else
		ejs.var("next", 0);

	res = ejs.execute();

	return { 200 , gzip_compress(res), HTML };
}

ReturnType ExecutorHtml::get(soci::session& db, const Route& route, const QueryParams& qp) {
	auto ejs = loadScript(route.ejs);

	auto& auth = qp.auth;

	if (auth)
		add_auth_info(ejs, auth.value());

	ejs.var("mobile", qp.mobile);

	return get(ejs, db, route, qp);
}

ReturnType ExecutorHtml::get(Ejs::Env& ejs, soci::session& db, const Route& route, const QueryParams& qp) {

	auto& table = route.db.table;
	auto& attributes = route.db.attributes;

	// db::GetParams params;

	auto r = db::get(db, table, qp.params, attributes);
	if (!r)
		return { 404, "Not found", PLAIN };

	std::string res;
	std::string_view type;

	auto& row = *r;

	// render ejs file

	ejs.vars(row);
		
	res = ejs.execute();
	
	std::println("RESPONSE\n{0}", res);

	return { 200, gzip_compress(res), HTML };
}

Executor::ReturnType ExecutorHtml::execute(const std::string& path) {

	auto ejs = loadScript(path);

	std::string res;

	std::println("Executing template: {0}", path);

	res = ejs.execute();
	
	
	return { 200, gzip_compress(res), HTML };
}



Executor::ReturnType ExecutorHtml::execute(const std::string& path, const Executor::AuthInfo& info) {

	auto ejs = loadScript(path);

	std::string res;

	//std::println("Executing template: {}", path);

	add_auth_info(ejs, info);

	res = ejs.execute();
	
	return { 200, gzip_compress(res), HTML };
}

std::string ExecutorHtml::templatePath(const std::string& path) const {
	return views_dir + path;
}

Ejs::Env ExecutorHtml::loadScript(const std::string& path, unsigned expires) {


	Ejs *s;

	if (auto found = scripts.get(path); found) {
		s = &found->body;
	}
	else {
		Ejs ejs;

		auto templ = templatePath(path);

		std::println("Loading template: {0}", templ);

		ejs.loadFile(templ);
		

		auto r = scripts.put(path, std::move(ejs), EJS, expires);

		s = &r.body;
	}
	
	return s->env("html");
}

ReturnType ExecutorHtml::schema(soci::session& db, const Route& route) {

	auto backend = db.get_backend_name();

	int column_count = 0;

	if (backend == "sqlite") {
			
		soci::rowset<soci::row> rs = (db.prepare << "PRAGMA table_info(" << route.db.table << ")");

		auto ejs = loadScript(route.ejs);

		ejs.schemaSqlite("schema", rs);

		auto res = ejs.execute();

		return { 200, gzip_compress(res), HTML };
	}

	return { 400 };
}

void ExecutorHtml::cleanup() {
	cache.cleanup();
	scripts.cleanup();
}