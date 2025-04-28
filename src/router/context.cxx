#include "context.h"

#include <boost/url.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <boost/algorithm/string/join.hpp>

#include <nlohmann/json.hpp>

#include <soci/sqlite3/soci-sqlite3.h>
#include <soci/postgresql/soci-postgresql.h>

#include <print>
#include <iostream>
#include <optional>
#include <sstream>
#include <regex>
#include <format>

#include "router.h"
#include "ejs.h"
#include "config.h"
#include "functions.h"

using Params = std::unordered_map<std::string, std::string>;
using Variant = std::variant<int, double, std::string>;
using Row = std::vector<Variant>;
using Json = nlohmann::json;

using Context = Router::Context;
using Route = Context::Route;
using Result = Context::Result;
using ReturnType = Router::ReturnType;

const std::string_view 
	PLAIN = "text/plain",
	HTML  = "text/html",
	JSON  = "application/json";


const std::unordered_map<std::string, soci::data_type> DataType = {
	{ "TEXT", soci::dt_string },
	{ "SERIAL", soci::dt_unsigned_long_long },
};

void dumpr(const Route& r) {
	std::println("DB => table: {0}, op: {1}", r.db.table, r.db.op);
}

Json row_to_json(const soci::row& row);
Json rowset_to_json(const soci::rowset<soci::row>& rs);

CScriptVar* row_to_js(const soci::row& row, CScriptVar* v = new CScriptVar());
CScriptVar* rowset_to_js(const soci::rowset<soci::row>& rs);

std::string gzip_compress(const std::string& data);

wString build_query(const std::string& path, const Context::Query& params, int offset);

Router::Context::Context(int pool_size) :
	pool(pool_size) 
{
	soci::register_factory_sqlite3();
	soci::register_factory_postgresql();

	for (int i = 0; i < METHOD_COUNT; ++i)
		root[i].reset(new Node());

}

Router::Context::~Context()
{}

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

	return true;
}

void Router::Context::addRoute(Router::Method method, const std::string& path, const Route& route) {

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

	node->route = route;

	// if (route.db.table.length() > 0)
	// 	addTable(route.db.table);

	// auto& r = router[method];
	// r.insert({ path, route });
}

bool Router::Context::match(Router::Method method, const std::string& path) const {
	return find(method, path).route != nullptr;
}

void Router::Context::cleanup() {
	cache.cleanup();
}

ReturnType Router::Context::get(const Route& route, const QueryParams& qp) 
{
	dumpr(route);

	auto const& path = qp.path; 
	auto c = cache.get(path);


	if (c) {
		std::println("Found in cache: {0}", path);

		return { 200, std::move(c->body), std::move(c->type), c->ts, c->expires };
	}

	auto& db = route.db;
	bool usedb = !db.table.empty();

	if (usedb && db.op == "schema") {
		return schema(route);
	}

	auto r = qp.params.size() > 0 ? get(route, qp.params) : ls(route, qp.route, qp.query);

	if (r.status < 300 && route.cache > 0) {

		r.ts = cache.put(path, r.body, r.type, route.cache);
		r.expires = route.cache;
	}

	return std::move(r);
}

ReturnType Router::Context::post(
	const Route& route, 
	const Params& params, 
	const std::string& body) 
{

	return put(route, body);
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

ReturnType Router::Context::get(const Route& route, const Params& params) {

	soci::session db(pool);

	auto& table = route.db.table;
	auto templ = views_dir + route.ejs;

	std::println("Executing template: {}", templ);

	if (table.empty()) {

		if (templ.empty())
			return { 404, "No template found", PLAIN };

		Ejs ejs;

		ejs.loadFile(templ);

		auto res = ejs.execute();
		
		return { 200, gzip_compress(res), HTML };
	}

	std::string res;
	std::string_view type;

	for (auto&& [name, value] : params) {

		soci::row row;
		soci::indicator ind;

		// // std::string query = "SELECT * FROM " + table + " WHERE " + name + " = :val";
		// // db << query, soci::use(value), soci::into(row, ind);
		
		// db << "select * from " << table 
		// 	// << " where " << name << " = :val" 
		// 	<< " where " << name << "="  
		// 	// << '"' << value << '"'
		// 	<< value
		// 	<< " limit 1"
		// 	// , soci::use(value) 
		// 	, soci::into(row, ind)
		// 	//, soci::into(row)
		// 	;

		// if (ind != soci::i_ok) {
		// 	std::cerr << "Not found" << std::endl;
		// 	// return "";
		// }

		// std::println("ROW SIZE: {0}", row.size());

		// if (row.size() == 0) {
		// 	std::cerr << "Not found" << std::endl;
		// 	return "";
		// }

		soci::statement stmt = (db.prepare << "select * from " << table 
			<< " where " << name << " = :val"  
			//<< value
			, soci::use(value)
			, soci::into(row))
			;

		stmt.execute(); // Run the query

		if (!stmt.fetch()) {
			std::cerr << "Not found" << std::endl;
			return { 404, "Not found", PLAIN };
		}

		if (templ.length() > 0) {

			// render ejs file
			Ejs ejs;

			auto a = row_to_js(row, ejs.getRoot());

			ejs.loadFile(templ);

			res = ejs.execute();

			type = HTML;
			//std::println("HTML\n{0}", html);
		}

		else {

			res = row_to_json(row).dump();

			//std::println("{0}", json.dump());

			// render as JSON

			type = JSON;
		}

		std::println("RESPONSE\n{0}", res);

		// return std::move(res);

		return { 200, gzip_compress(res), type };

	}

	return { 404 };
}

ReturnType Router::Context::ls(const Route& route, const std::string& path, const Query& q) {

	soci::session db(pool);

	auto& table = route.db.table;
	auto& alias = route.db.alias;

	auto templ = templatePath(route.ejs);

	std::println("Executing template: {0} on table {1}", templ, table);

	if (table.empty()) {

		if (templ.empty())
			return { 404, "No template found", PLAIN };

		Ejs ejs;

		ejs.loadFile(templ);

		auto res = ejs.execute();
		
		return { 200, gzip_compress(res), HTML };
	}

	std::string res, type;
	
	int status = 200;

	int limit = route.db.limit, offset = q.offset;

	soci::rowset<soci::row> rs = (
		db.prepare << "select * from " << table << " LIMIT :limit OFFSET :offset",
		soci::use(limit, "limit"), 
		soci::use(offset, "offset")
	);

	if (!templ.empty()) {

		auto a = rowset_to_js(rs);

		auto& items = alias.empty() ? table : alias;

		Ejs ejs;

		ejs.addVariable(table, a);

		if (offset > 0)
			ejs.addVariable("p", new CScriptVar(build_query(path, q, offset - limit > 0 ? offset - limit : 0)));
		else
			ejs.addVariable("p", new CScriptVar(false));

		if (a->getArrayLength() == limit)
			ejs.addVariable("n", new CScriptVar(build_query(path, q, offset + limit) ));
		else
			ejs.addVariable("n", new CScriptVar(false));
			
		ejs.loadFile(templ);

		try {

			res = ejs.execute();
		}
		catch (CScriptException* e) {
			res = std::format(R"(<body><h1 style="color:red">ERROR:</h1><code>{}</code></body>)", e->text.c_str());
			status = 500;

			delete e;
		}

		type = HTML;
	}

	else {

		res = rowset_to_json(rs).dump();
		type = JSON;
	}

	std::println("RESPONSE\n{0}", res);

	// return std::move(res);

	return { 
		status, 
		status < 300 ? gzip_compress(res) : std::move(res), 
		type 
	};
}

ReturnType Router::Context::schema(const Route& route) {
	soci::session db(pool);

	auto backend = db.get_backend_name();

	std::list<std::pair<std::string, std::string>> info;

	int column_count = 0;

	if (backend == "sqlite") {
			
		soci::rowset<soci::row> rs = (db.prepare << "PRAGMA table_info(" << route.db.table << ")");
		
		for (auto const& row : rs) {

			auto dt = DataType.find(row.get<std::string>(2))->second;

			auto name = row.get<std::string>(1);
			auto type = "text";

			switch (dt) {

				case soci::dt_integer:
				case soci::dt_long_long:
				case soci::dt_unsigned_long_long:
				case soci::dt_double:
				type = "number";
			}

			info.emplace_back(std::move(name), std::move(type));

			++column_count;
		}
	}

	if (route.ejs.empty()) {
		// json
	}
	else {

		Ejs ejs;


		auto templ = templatePath(route.ejs);

		if (!ejs.loadFile(templ)) {
			return { 404, "Not found", PLAIN };
		}

		auto schema = new CScriptVar();
		int i = 0;

		schema->setArray();

		for (auto& [name, type] : info) {

			auto v = new CScriptVar();

			v->addChild("name", new CScriptVar(name));
			v->addChild("type", new CScriptVar(type));

			schema->setArrayIndex(i++, v);
		}

		ejs.addVariable("schema", schema);

		std::string res;
		int status = 200;

		try {

			res = ejs.execute();
			res = gzip_compress(res);
		}
		catch (CScriptException* e) {
			res = std::format(R"(<body><h1 style="color:red">ERROR:</h1><code>{}</code></body>)", e->text.c_str());

			delete e;

			status = 500;
		}

		return { status, res, HTML };
	}

	return { 200, "{}", JSON };
}

ReturnType Router::Context::put(const Route& route, const std::string& body) {

	soci::session db(pool);

	auto& table = route.db.table;
	
	Json json = Json::parse(body);

	if (!json.is_object()) {
		std::cerr << "Expected JSON object" << std::endl;
		return { 400, "Bad request", "text/plain" };
	}

	std::vector<std::string> columns;
	std::vector<std::string> placeholders;

	// soci::statement st = (db.prepare << "");

	std::ostringstream sql;
	// std::ostringstream values;

	for (auto it = json.begin(); it != json.end(); ++it) {
		const std::string& key = it.key();
		columns.push_back(key);
		placeholders.push_back(":" + key);
	}

	// Build SQL string
	sql << "INSERT INTO " << table << " ("
		<< boost::algorithm::join(columns, ", ") << ") VALUES ("
		<< boost::algorithm::join(placeholders, ", ") << ")";

	soci::statement st = (db.prepare << sql.str());

	for (auto it = json.begin(); it != json.end(); ++it) {
		const std::string& key = it.key();
		const Json& value = it.value();

		columns.push_back(key);
		placeholders.push_back(":" + key);

		// Bind based on type
		if (value.is_string()) {
			auto v = value.get<std::string>();
			st.exchange(soci::use(v, key));
		} else if (value.is_number_integer()) {
			auto v = value.get<int>();
			st.exchange(soci::use(v, key));
		} else if (value.is_number_float()) {
			auto v = value.get<double>();
			st.exchange(soci::use(v, key));
		} else if (value.is_boolean()) {
			auto v = value.get<bool>() ? 1 : 0;
			st.exchange(soci::use(v, key)); // SQLite has no bool
		} else if (value.is_null()) {
			//st.exchange(soci::use(std::string{}, key, soci::i_null));
		} else {
			// throw std::runtime_error("Unsupported value type for key: " + key);
		}
	}

	// Build the query
	// std::ostringstream query;
	// query << "INSERT INTO " << table << " ("
	// 	<< boost::join(columns, ", ") << ") VALUES ("
	// 	<< boost::join(placeholders, ", ") << ")";

	// st.alloc();
	// st.set_query(query.str());
	st.define_and_bind();

	try {
		st.execute(true);
	} catch (const soci::soci_error& e) {
		std::cerr << "Insert failed: " << e.what() << "\n";

		return { 400 };
	}

	// st.execute(true);
	
	// int affected_rows = st.get_affected_rows();

	// std::println("Affected rows: {0}", affected_rows);

	// int code = 400;

	// if (affected_rows > 0)
	// 	code = 200;

	return { 200 };
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

std::string Router::Context::templatePath(const std::string& path) const {
	return views_dir + path;
}

void Router::Context::addTable(const std::string& table) {

	soci::session db(pool);

	Schema info;

	if (db.is_connected()) {

		auto be = db.get_backend_name();
		std::println("Adding table: {0} {1}", table, be);

		int column_count = 0;

		if (be == "sqlite3") {
			
			soci::rowset<soci::row> rs = (db.prepare << "PRAGMA table_info(" << table << ")");
			
			for (auto const& row : rs) {

				info[row.get<std::string>(1)] = DataType.find(row.get<std::string>(2))->second;

				++column_count;
			}
			// // soci::statement stmt = (db.prepare << "PRAGMA table_info(" << table << ")");
			// std::string query = "PRAGMA table_info(\"" + table + "\")";
			// soci::statement stmt = (db.prepare << query);

			// soci::row row;
			// stmt.exchange(soci::into(row));  // bind row to receive output
			// stmt.define_and_bind();

			// stmt.execute(true);

			// int column_count = 0;
			
			// while (stmt.fetch()) {
			// 	++column_count; // Increment count for each column
			// }
			
			

		}

		std::println("COLUMN COUNT: {0}", column_count);
	}
	
	tables[table] = std::move(info);
}

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

CScriptVar* row_to_js(const soci::row& row, CScriptVar* v) {

	for (std::size_t i = 0; i < row.size(); ++i) {
		const soci::column_properties& props = row.get_properties(i);
		const std::string& name = props.get_name();

		if (row.get_indicator(i) != soci::i_null) {
		
			switch (props.get_data_type()) {
				case soci::dt_string:
				v->addChild(name.c_str(), new CScriptVar(row.get<std::string>(i).c_str()));
				break;

				case soci::dt_double:
				v->addChild(name.c_str(), new CScriptVar(row.get<double>(i)));
				break;

				case soci::dt_integer:
				case soci::dt_long_long:
				case soci::dt_unsigned_long_long:
				v->addChild(name.c_str(), new CScriptVar(row.get<int>(i)));
				break;

				case soci::dt_date:
					// Format date as string
					{
						std::ostringstream oss;
						std::tm t = row.get<std::tm>(i);
						oss << std::put_time(&t, "%Y-%m-%d %H:%M:%S");
						// obj[name] = oss.str();
					}
					break;
				default:
				break;
			}
		}
	}

	return v;
}

CScriptVar* rowset_to_js(const soci::rowset<soci::row>& rs) {

	auto v = new CScriptVar();

	v->setArray();

	int i = 0;
	for (const auto& row : rs) {
		v->setArrayIndex(i++, row_to_js(row));
	}

	return v;
}

std::string gzip_compress(const std::string& data) {
	std::ostringstream compressed;
	boost::iostreams::filtering_ostream out;
	out.push(boost::iostreams::gzip_compressor());
	out.push(compressed);
	out << data;
	boost::iostreams::close(out);
	return compressed.str();
}


// Our upsert function: it builds an INSERT ... ON CONFLICT query with bound parameters,
// then performs a SELECT query to retrieve the auto-generated id.
// Parameters:
//   db         : the opened soci::session
//   table      : the name of the table (e.g., "users")
//   rowData    : a JSON object containing key/value pairs to insert/update
//   uniqueKey  : the column name to use for conflict detection (e.g., "email")
//   db_backend : a string identifying the backend ("postgresql" or "sqlite")
// Returns the id (assumed to be in column "id") of the inserted (or updated) row.
int upsert_row(soci::session& db,
			   const std::string& table,
			   const nlohmann::json& rowData,
			   const std::string& uniqueKey,
			   const std::string& db_backend)
{
	if (!rowData.is_object())
		throw std::runtime_error("Expected JSON object for row data");

	std::vector<std::string> columns;
	std::vector<std::string> placeholders;
	std::vector<std::string> assignments;

	// We will store bound values in shared pointers to keep them alive until statement execution.
	std::map<std::string, std::shared_ptr<void>> bindings;

	// Build lists for columns and placeholders.
	for (auto it = rowData.begin(); it != rowData.end(); ++it)
	{
		std::string key = it.key();
		columns.push_back(key);
		placeholders.push_back(":" + key);

		// For the update clause, update each column except the unique key.
		if (key != uniqueKey) {
			// For PostgreSQL, the new value is referred to as EXCLUDED.key.
			assignments.push_back(key + " = EXCLUDED." + key);
		}
	}

	// Build the INSERT statement.
	std::ostringstream oss;
	oss << "INSERT INTO " << table << " ("
		<< boost::algorithm::join(columns, ", ") << ") VALUES ("
		<< boost::algorithm::join(placeholders, ", ") << ") ";
	oss << "ON CONFLICT (" << uniqueKey << ") ";

	// For PostgreSQL, we use DO UPDATE.
	// (SQLite starting with 3.24.0 also supports a similar syntax; adjust if necessary.)
	if (!assignments.empty())
		oss << "DO UPDATE SET " << boost::algorithm::join(assignments, ", ");
	else
		oss << "DO NOTHING";  // In case rowData contains only the unique key

	std::string sqlQuery = oss.str();

	// Prepare the statement using the constructed SQL.
	soci::statement st = (db.prepare << sqlQuery);

	// Bind all parameters from the JSON object.
	for (auto it = rowData.begin(); it != rowData.end(); ++it)
	{
		const std::string key = it.key();
		const auto& value = it.value();

		if (value.is_string())
		{
			auto ptr = std::make_shared<std::string>(value.get<std::string>());
			bindings[key] = ptr;
			st.exchange(soci::use(*ptr, key));
		}
		else if (value.is_number_integer())
		{
			auto ptr = std::make_shared<int>(value.get<int>());
			bindings[key] = ptr;
			st.exchange(soci::use(*ptr, key));
		}
		else if (value.is_number_float())
		{
			auto ptr = std::make_shared<double>(value.get<double>());
			bindings[key] = ptr;
			st.exchange(soci::use(*ptr, key));
		}
		else if (value.is_boolean())
		{
			// For booleans, bind as integer (1 or 0)
			auto ptr = std::make_shared<int>(value.get<bool>() ? 1 : 0);
			bindings[key] = ptr;
			st.exchange(soci::use(*ptr, key));
		}
		else if (value.is_null())
		{
			// Bind null values using a dummy string; the indicator i_null marks it as null.
			auto ptr = std::make_shared<std::string>("");
			bindings[key] = ptr;
			// st.exchange(soci::use(*ptr, key, soci::i_null));
		}
		else
		{
			throw std::runtime_error("Unsupported JSON type for key: " + key);
		}
	}

	st.define_and_bind();
	st.execute(true);
	// Note: For INSERT/UPDATE, execute(true) typically returns false because no rows are available to fetch.
	// That is expected.

	// Retrieve the id of the inserted/updated row.
	// We assume the unique key uniquely identifies the row and the table has an auto-increment column "id".
	int id = -1;
	// Prepare a SELECT query for id.
	std::ostringstream ossSel;
	ossSel << "SELECT id FROM " << table << " WHERE " << uniqueKey << " = :uniqVal";
	std::string selectQuery = ossSel.str();

	// Bind the value from rowData for the unique key.
	if (!rowData.contains(uniqueKey))
		throw std::runtime_error("Unique key '" + uniqueKey + "' is missing from JSON data");

	// For this simple example, assume the unique key is a string.
	// (You can extend it to check for number types if needed.)
	std::string uniqueValue = rowData.at(uniqueKey).get<std::string>();

	db << selectQuery, soci::use(uniqueValue, "uniqVal"), soci::into(id);

	if (id == -1)
		throw std::runtime_error("Failed to retrieve id after upsert.");

	return id;
}

std::vector<std::string> get_unique_columns(
	soci::session& db,
	const std::string& table) {
	std::vector<std::string> unique_columns;

	std::string backend = db.get_backend_name();

	if (backend == "sqlite3") {
		soci::row row;

		// Step 1: get all indexes on table
		soci::statement stmt = (db.prepare << "PRAGMA index_list('" + table + "')", soci::into(row));
		stmt.execute();

		while (stmt.fetch()) {
			std::string index_name = row.get<std::string>(1); // index name
			int is_unique = row.get<int>(2);                  // 1 = unique

			if (is_unique) {
				// Step 2: get columns for the unique index
				soci::row index_row;
				soci::statement idx_stmt = (db.prepare << "PRAGMA index_info('" + index_name + "')", soci::into(index_row));
				idx_stmt.execute();

				while (idx_stmt.fetch()) {
					std::string col = index_row.get<std::string>(2); // column name
					unique_columns.push_back(col);
				}
			}
		}

	} else if (backend == "postgresql") {
		soci::rowset<std::string> rs = (db.prepare << R"(
			SELECT a.attname
			FROM pg_index i
			JOIN pg_attribute a ON a.attrelid = i.indrelid AND a.attnum = ANY(i.indkey)
			WHERE i.indrelid = :tbl::regclass
			  AND i.indisunique = true
		)", soci::use(table));

		for (const auto& col : rs) {
			unique_columns.push_back(col);
		}

	} else {
		throw std::runtime_error("Unsupported backend: " + backend);
	}

	return unique_columns;
}

wString build_query(const std::string& path, const Router::Context::Query& params, int offset) {

	if (params.size() == 0 && offset == 0)
		return path;

	std::ostringstream os;

	os << path << "?o=" << offset;

	for (auto& [name, value] : params)
		os << '&' << name << '=' << value;

	return os.str();
}