#pragma once

#include "executor/html.h"
#include "executor/json.h"

#include <soci/soci.h>

#include <map>

class Router::Context {

public:

	using Schema = std::unordered_map<std::string, soci::data_type>;
	using TableInfo = std::unordered_map<std::string, Schema>;

	using Params = Executor::Params;
	using Query = Executor::Query;
	using QueryParams = Executor::QueryParams;
	using AuthInfo = Executor::AuthInfo;
	using Route = Executor::Route;
	using Errors = std::unordered_map<int, std::string>;

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

	std::string public_dir = "public", views_dir = "views";

	

	Context(int pool_size=4);
	~Context();

	bool init(const char* path, const std::string& root);

	void set(const std::string& var, const std::string& val);

	void database(const std::string&);

	Route& addRoute(Method, const std::string& path);

	bool match(Method, const std::string& path) const;
	Result find(Method, const std::string& path) const;

	ReturnType get(const Route&, const QueryParams&);
	ReturnType post(const Route&, const QueryParams&, const std::string& body, Payload type);
	ReturnType remove(const Route&, const QueryParams&);
	
	void cleanup();

protected:

	void connectDatabase(const std::string& url);
	void addTable(const std::string& table);

	ReturnType schema(const Route&);

private:
	soci::connection_pool pool;
	//soci::session db;
	
	ExecutorHtml html;
	ExecutorJson json;
	
	TableInfo tables;
	
	Node::Ptr root[METHOD_COUNT];

	Params vars;
	Errors errors;
};