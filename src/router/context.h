#pragma once

#include "router.h"
#include "cache.h"

#include <soci/soci.h>

#include <map>

struct Router::Context {

	using Schema = std::unordered_map<std::string, soci::data_type>;
	using TableInfo = std::unordered_map<std::string, Schema>;
	using Params = std::map<std::string, std::string>;

	struct Route {

		unsigned cache = 0;

		struct {

			std::string table, alias, op;
			unsigned limit = 10;

		} db;

		std::string ejs;
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
		bool mobile = false;
	};

	struct Result {
		const Route* route;
		Params params;
	};

	struct Node {

		using Ptr = std::unique_ptr<Node>;

		std::unordered_map<std::string, Ptr> children;
		Ptr param_child;
		std::string param_name;
		std::optional<Route> route;
	};

	soci::connection_pool pool;
	//soci::session db;
	Cache cache;
	TableInfo tables;
	std::string public_dir = "public", views_dir = "views";
	
	Node::Ptr root[METHOD_COUNT];

	Context(int pool_size=4);
	~Context();

	bool init(const char* path, const std::string& root);

	void database(const std::string&);
	void addRoute(Method, const std::string& path, const Route&);

	bool match(Method, const std::string& path) const;
	Result find(Method, const std::string& path) const;

	ReturnType get(const Route&, const QueryParams&);
	ReturnType post(const Route&, const Params&, const std::string& body);
	
	void cleanup();

protected:

	void connectDatabase(const std::string& url);
	void addTable(const std::string& table);

	ReturnType get(const Route&, const Params&);
	ReturnType ls(const Route&, const std::string& path, const Query&);
	ReturnType put(const Route&, const std::string& body);

	ReturnType schema(const Route&);

	std::string templatePath(const std::string& path) const;
};