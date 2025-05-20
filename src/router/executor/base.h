#pragma once

#include "../router.h"
#include "soci_fwd.h"

#include <string>
#include <optional>
#include <map>
#include <vector>
#include <functional>

class Executor {
public:

	using ReturnType = Router::ReturnType;
	using Payload = Router::Payload;

	using Params = std::map<std::string, std::string>;
	using Attributes = std::unordered_map<std::string, std::string>;

	struct AuthInfo {
		std::string role, uid, name, email;
		uint64_t expire;

		bool parse(const std::string& token, const std::string& secret);
	};

	struct Route {

		Route(const std::string& path);

		struct Database {

			std::string table, alias, op;
			unsigned limit = 10;

			std::string created, updated, attributes = "*", type;
			Attributes bind;

			// struct Attribute {
			// 	std::string name, type;
			// 	bool compressed = false;
			// };

			// std::vector<Attribute> attributes;



			struct Fts {
				std::string table, index = "name";
				std::vector<std::string> rank;
			};

			std::optional<Fts> fts;
		};

		std::string path;

		unsigned cache = 0;

		std::string ejs;
		std::string redirect;
		std::string role, api_key;

 		Database db;

		std::function<bool(const std::string&, const Params&, const AuthInfo&)> 
		rule_matcher;
		
		void set_role(const std::string& role);
	};


	struct Query :  std::map<std::string, std::string> {
		using std::map<std::string, std::string>::map;

		int offset = 0;
	};

	

	struct QueryParams {
		std::string path;
		std::string route;
		Params params;
		Query query;

		std::string api_key;
		std::optional<AuthInfo> auth;

		bool mobile = false;

		bool parse(const std::string& path);
		bool parse_auth_token(const std::string& token, const std::string& secret);
		bool parse_auth_header(const std::string& header);
	};

	

	Executor(soci::connection_pool&);

	static bool 
	del(soci::session&, const std::string& table, const std::string& id);

	static soci::rowset<soci::row> 
	ls(soci::session& db, const Route& route, const QueryParams& qp);

protected:

	soci::connection_pool& pool;
};