#include "db.h"

#include <soci/soci.h>
#include <nlohmann/json.hpp>

#include <boost/algorithm/string/join.hpp>
#include <boost/iterator/transform_iterator.hpp>

#include <vector>
#include <list>
#include <sstream>
#include <iostream>
#include <print>

using Json = nlohmann::json;

namespace {

bool is_gzip_compressed(const std::string& data) {
	return data.size() >= 2 &&
		   static_cast<unsigned char>(data[0]) == 0x1F &&
		   static_cast<unsigned char>(data[1]) == 0x8B;
}

}

std::string db::blob(soci::row const& r, int i) {
	auto blob = r.move_as<soci::blob>(i); // or "data" if you're using names

	std::size_t len = blob.get_len();
	std::string content(len, '\0'); // Pre-allocate the string

	blob.read(0, &content[0], len);

	return std::move(content);
}

std::optional<soci::row> db::get(
	soci::session& db, 
	const std::string& table, 
	const db::GetParams& params) 
{

	// soci::row row;

	// soci::statement stmt = (db.prepare << "select " 
	// 	<< params.attributes
	// 	<< " from " << table
	// 	<< " where id = :val"
	// 	//<< value
	// 	, soci::use(params.value)
	// 	, soci::into(row))
	// 	;

	// stmt.execute(); // Run the query

	// if (!stmt.fetch()) {
	// 	std::cerr << "Not found" << std::endl;
	// 	return {};
	// }

	// return std::move(row);
}

std::optional<soci::row> db::get(soci::session& db, const std::string& table, const Params& params, std::string_view attributes) {

	for (auto&& [name, value] : params) {

		soci::row row;
		soci::indicator ind;

		soci::statement stmt = (db.prepare << "select " 
			<< attributes 
			<< " from " << table 
			<< " where " << name << " = :val"  
			//<< value
			, soci::use(value)
			, soci::into(row))
			;

		stmt.execute(); // Run the query

		if (!stmt.fetch()) {
			std::cerr << "Not found" << std::endl;
			return {};
		}

		return std::move(row);
	}

	return {};
}

soci::rowset<soci::row> db::ls(soci::session& db, const std::string& table, int offset, int limit) {
	return (db.prepare << "select * from " << table 
		<< " LIMIT " << limit << " OFFSET " << offset);
}

soci::rowset<soci::row> db::ls(
	soci::session& db,
	const std::string& table,
	const Params& params,
	int offset,
	int limit
) {
	if (params.empty()) {
		// Fallback to overload without filters
		return db::ls(db, table, offset, limit);
	}

	// Construct WHERE clause dynamically
	std::ostringstream query;
	query << "SELECT * FROM " << table << " WHERE ";

	std::vector<std::string> conditions;
	for (const auto& [key, value] : params) {
		if (key == "created") {

			conditions.push_back(key + " > " + db.get_backend()->timestamp(key));
			
		} else {
			conditions.push_back(key + " = :" + key);
		}
	}

	query << boost::algorithm::join(conditions, " AND ");
	query << " LIMIT " << limit << " OFFSET " << offset;

	auto sql = query.str();

	soci::statement st = (db.prepare << sql);

	// Create and prepare the statement
	// soci::statement st = (db.prepare << query.str(), soci::into(rs));

	// Bind each parameter using exchange_for_rowset
	for (const auto& [key, value] : params) {

		if (key == "created") {
			int64_t ts = std::stoll(value); 
			st.exchange(soci::use(ts, key));
		}
		else {
			st.exchange(soci::use(value, key));
		}
	}

	st.define_and_bind(); // Must be called before executing

	// // Create rowset from the bound statement
	// soci::rowset<soci::row> rs(st);

	return st;

}

/*
soci::rowset<soci::row> db::ls(soci::session& db, const std::string& table, const Params& params, int offset, int limit) {

	if (params.size() == 0)
		return db::ls(db, table, offset, limit);
	

	auto where = [](auto& params) -> std::string {

		std::vector<std::string> where;

		where.reserve(params.size());

		for (auto& [key, value] : params) {

			if (key == "created") {
				where.emplace_back(key + " > :" + key);
			}
			else {
				where.emplace_back(key + " = :" + key);
			}
		}
		
		return boost::algorithm::join(where, " and ");
	};


	soci::statement st = (db.prepare << "select * from " << table 
		<< where(params) 
		<< " LIMIT " << limit << " OFFSET " << offset);

	for (auto& [key, value] : params)
		st.exchange(soci::use(value, key));

	st.define_and_bind();

	return soci::rowset<soci::row>(st);
}
*/

soci::rowset<soci::row> search_sqlite(
	soci::session& db, 
	const std::string& table, 
	const std::string& fts_table, 
	const std::vector<std::string>& rank, 
	const std::string& value, int offset, int limit) 
{
	std::string r = fts_table;

	if (rank.size() > 0) {
		// auto to_string = [](auto v) -> std::string {
		// 	return std::to_string(v);
		// };

		// r += "," + boost::algorithm::join(
		// 	boost::make_iterator_range(
		// 		boost::make_transform_iterator(rank.begin(), to_string),
		// 		boost::make_transform_iterator(rank.end(), to_string)),
		// 	",");

		r += "," + boost::algorithm::join(rank, ",");
	}

	return (db.prepare <<
			"SELECT " << table << ".*, bm25(" << r << ") AS rank"
			" FROM " << table <<
			" JOIN " << fts_table << " ON " << table << ".id = " << fts_table << ".rowid"
			" WHERE " << fts_table << " MATCH :search_term"
			" ORDER BY rank",
			soci::use(value, "search_term") // Bind the search term safely
		);
}

soci::rowset<soci::row> search_postgre(
	soci::session& db, 
	const std::string& table, 
	const std::string& index, 
	const std::vector<std::string>& rank, 
	const std::string& value, int offset, int limit) 
{
	auto extract_rank = [](auto& rank) -> std::string {

		auto r = boost::algorithm::join(rank, ",");

		for (auto i = rank.size(); i < 4; ++i)
			r += ",0";

		return "'{" + r + "}',";

	};

	std::string tsquery = "plainto_tsquery('english', :value)";

	return (db.prepare << 
		"SELECT *,"
		"ts_rank(" << (rank.size() > 0 ? extract_rank(rank) : "")  << index << "," << tsquery << ") AS rank "
		"FROM " << table << " "
		"WHERE " << index << " @@ " << tsquery << " "
		"ORDER BY rank DESC "
		<< " LIMIT " << limit << " OFFSET " << offset, soci::use(value), soci::use(value));
}

soci::rowset<soci::row> db::search(
	soci::session& db, 
	const std::string& table, 
	const std::string& fts_table, 
	const std::vector<std::string>& rank, 
	const std::string& value, 
	int offset, int limit) {
	
	auto backend = db.get_backend_name();

	if (backend == "sqlite")
		return search_sqlite(db, table, fts_table, rank, value, offset, limit);

	if (backend == "pg")
		return search_postgre(db, table, fts_table, rank, value, offset, limit);

	return {};
}


std::optional<std::string> db::insert(soci::session& db, const std::string& table, const std::string& body) {
	std::string id;

	Json json = Json::parse(body);

	if (!json.is_object()) {
		std::cerr << "Expected JSON object" << std::endl;
		return {};
	}

	std::vector<std::string> columns, placeholders;

	for (auto it = json.begin(); it != json.end(); ++it) {

		auto& key = it.key();

		columns.push_back(key);
		placeholders.push_back(":" + key);
	}

	std::ostringstream sql;

	// Build SQL string
	sql << "INSERT INTO " << table << " ("
		<< boost::algorithm::join(columns, ",") << ") VALUES ("
		<< boost::algorithm::join(placeholders, ",") << ") RETURNING id";

	soci::statement st = (db.prepare << sql.str(), soci::into(id));

	for (auto it = json.begin(); it != json.end(); ++it) {

		auto& key = it.key();
		auto& value = it.value();

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

	st.define_and_bind();

	try {
		st.execute(true);

		return std::move(id);
	} catch (const soci::soci_error& e) {
		std::cerr << "Insert failed: " << e.what() << "\n";
	}

	return {};
}

std::optional<std::string> db::insert(soci::session& db, const std::string& table, const Params& params) {

	using Param = std::pair<const std::string, std::string>;

	std::string id;

	// Transformer: takes pair -> returns key (string)
	auto key_extractor = [](const Param& p) -> const std::string& {
		return p.first;
	};

	auto placeholder_extractor = [](const Param& p) -> std::string {
		return ":" + p.first;
	};

	std::ostringstream sql;

	// Build SQL string
	sql << "INSERT INTO " << table << " ("
		<< boost::algorithm::join(
			boost::make_iterator_range(
				boost::make_transform_iterator(params.begin(), key_extractor),
				boost::make_transform_iterator(params.end(), key_extractor)),
			",") 
		<< ") VALUES ("
		<< boost::algorithm::join(
			boost::make_iterator_range(
				boost::make_transform_iterator(params.begin(), placeholder_extractor),
				boost::make_transform_iterator(params.end(), placeholder_extractor)),
			",") 
		<< ") RETURNING id";

	soci::statement st = (db.prepare << sql.str(), soci::into(id));

	std::list<soci::blob> blobs;

	for (auto& [key, value] : params) {

		if (is_gzip_compressed(value)) {
			auto& blob = blobs.emplace_back(db); // create and store blob
			blob.write(0, value.data(), value.size());

			st.exchange(soci::use(blob, key));
		}
		else {
			st.exchange(soci::use(value, key));
		}

	}

	st.define_and_bind();

	try {
		st.execute(true);

		std::println("INSERT successfull, id={0}", id);

		return std::move(id);
	} catch (const soci::soci_error& e) {
		std::cerr << "Insert failed: " << e.what() << "\n";
	}

	return {};
}

bool db::del(soci::session& db, const std::string& table, const std::string& id) {
	soci::statement st = (db.prepare << 
		"DELETE FROM " << table << " WHERE id = :id"
	, soci::use(id));

	try {
		st.execute(true);

		std::println("DELETE successfull, id={0}", id);

		return true;
	} catch (const soci::soci_error& e) {
		std::cerr << "DELETE failed: " << e.what() << "\n";
	}

	return false;
}