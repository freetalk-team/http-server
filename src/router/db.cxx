#include "db.h"

#include <soci/sqlite3/soci-sqlite3.h>
#include <soci/postgresql/soci-postgresql.h>

void db::register_backends() {
	soci::register_factory_sqlite3();
	soci::register_factory_postgresql();
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
// int upsert_row(soci::session& db,
// 			   const std::string& table,
// 			   const Json& rowData,
// 			   const std::string& uniqueKey,
// 			   const std::string& db_backend)
// {
// 	if (!rowData.is_object())
// 		throw std::runtime_error("Expected JSON object for row data");

// 	std::vector<std::string> columns;
// 	std::vector<std::string> placeholders;
// 	std::vector<std::string> assignments;

// 	// We will store bound values in shared pointers to keep them alive until statement execution.
// 	std::map<std::string, std::shared_ptr<void>> bindings;

// 	// Build lists for columns and placeholders.
// 	for (auto it = rowData.begin(); it != rowData.end(); ++it)
// 	{
// 		std::string key = it.key();
// 		columns.push_back(key);
// 		placeholders.push_back(":" + key);

// 		// For the update clause, update each column except the unique key.
// 		if (key != uniqueKey) {
// 			// For PostgreSQL, the new value is referred to as EXCLUDED.key.
// 			assignments.push_back(key + " = EXCLUDED." + key);
// 		}
// 	}

// 	// Build the INSERT statement.
// 	std::ostringstream oss;
// 	oss << "INSERT INTO " << table << " ("
// 		<< boost::algorithm::join(columns, ", ") << ") VALUES ("
// 		<< boost::algorithm::join(placeholders, ", ") << ") ";
// 	oss << "ON CONFLICT (" << uniqueKey << ") ";

// 	// For PostgreSQL, we use DO UPDATE.
// 	// (SQLite starting with 3.24.0 also supports a similar syntax; adjust if necessary.)
// 	if (!assignments.empty())
// 		oss << "DO UPDATE SET " << boost::algorithm::join(assignments, ", ");
// 	else
// 		oss << "DO NOTHING";  // In case rowData contains only the unique key

// 	std::string sqlQuery = oss.str();

// 	// Prepare the statement using the constructed SQL.
// 	soci::statement st = (db.prepare << sqlQuery);

// 	// Bind all parameters from the JSON object.
// 	for (auto it = rowData.begin(); it != rowData.end(); ++it)
// 	{
// 		const std::string key = it.key();
// 		const auto& value = it.value();

// 		if (value.is_string())
// 		{
// 			auto ptr = std::make_shared<std::string>(value.get<std::string>());
// 			bindings[key] = ptr;
// 			st.exchange(soci::use(*ptr, key));
// 		}
// 		else if (value.is_number_integer())
// 		{
// 			auto ptr = std::make_shared<int>(value.get<int>());
// 			bindings[key] = ptr;
// 			st.exchange(soci::use(*ptr, key));
// 		}
// 		else if (value.is_number_float())
// 		{
// 			auto ptr = std::make_shared<double>(value.get<double>());
// 			bindings[key] = ptr;
// 			st.exchange(soci::use(*ptr, key));
// 		}
// 		else if (value.is_boolean())
// 		{
// 			// For booleans, bind as integer (1 or 0)
// 			auto ptr = std::make_shared<int>(value.get<bool>() ? 1 : 0);
// 			bindings[key] = ptr;
// 			st.exchange(soci::use(*ptr, key));
// 		}
// 		else if (value.is_null())
// 		{
// 			// Bind null values using a dummy string; the indicator i_null marks it as null.
// 			auto ptr = std::make_shared<std::string>("");
// 			bindings[key] = ptr;
// 			// st.exchange(soci::use(*ptr, key, soci::i_null));
// 		}
// 		else
// 		{
// 			throw std::runtime_error("Unsupported JSON type for key: " + key);
// 		}
// 	}

// 	st.define_and_bind();
// 	st.execute(true);
// 	// Note: For INSERT/UPDATE, execute(true) typically returns false because no rows are available to fetch.
// 	// That is expected.

// 	// Retrieve the id of the inserted/updated row.
// 	// We assume the unique key uniquely identifies the row and the table has an auto-increment column "id".
// 	int id = -1;
// 	// Prepare a SELECT query for id.
// 	std::ostringstream ossSel;
// 	ossSel << "SELECT id FROM " << table << " WHERE " << uniqueKey << " = :uniqVal";
// 	std::string selectQuery = ossSel.str();

// 	// Bind the value from rowData for the unique key.
// 	if (!rowData.contains(uniqueKey))
// 		throw std::runtime_error("Unique key '" + uniqueKey + "' is missing from JSON data");

// 	// For this simple example, assume the unique key is a string.
// 	// (You can extend it to check for number types if needed.)
// 	std::string uniqueValue = rowData.at(uniqueKey).get<std::string>();

// 	db << selectQuery, soci::use(uniqueValue, "uniqVal"), soci::into(id);

// 	if (id == -1)
// 		throw std::runtime_error("Failed to retrieve id after upsert.");

// 	return id;
// }

// std::vector<std::string> get_unique_columns(
// 	soci::session& db,
// 	const std::string& table) {
// 	std::vector<std::string> unique_columns;

// 	std::string backend = db.get_backend_name();

// 	if (backend == "sqlite3") {
// 		soci::row row;

// 		// Step 1: get all indexes on table
// 		soci::statement stmt = (db.prepare << "PRAGMA index_list('" + table + "')", soci::into(row));
// 		stmt.execute();

// 		while (stmt.fetch()) {
// 			std::string index_name = row.get<std::string>(1); // index name
// 			int is_unique = row.get<int>(2);                  // 1 = unique

// 			if (is_unique) {
// 				// Step 2: get columns for the unique index
// 				soci::row index_row;
// 				soci::statement idx_stmt = (db.prepare << "PRAGMA index_info('" + index_name + "')", soci::into(index_row));
// 				idx_stmt.execute();

// 				while (idx_stmt.fetch()) {
// 					std::string col = index_row.get<std::string>(2); // column name
// 					unique_columns.push_back(col);
// 				}
// 			}
// 		}

// 	} else if (backend == "postgresql") {
// 		soci::rowset<std::string> rs = (db.prepare << R"(
// 			SELECT a.attname
// 			FROM pg_index i
// 			JOIN pg_attribute a ON a.attrelid = i.indrelid AND a.attnum = ANY(i.indkey)
// 			WHERE i.indrelid = :tbl::regclass
// 			  AND i.indisunique = true
// 		)", soci::use(table));

// 		for (const auto& col : rs) {
// 			unique_columns.push_back(col);
// 		}

// 	} else {
// 		throw std::runtime_error("Unsupported backend: " + backend);
// 	}

// 	return unique_columns;
// }
