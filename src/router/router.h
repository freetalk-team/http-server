#pragma once

#include <string>
#include <memory>



class Router {
	public:

	enum Method : int {

		GET,
		POST,
		DELETE,
		HEAD,
		PUT,
		PATCH,
		OPTIONS,

		METHOD_COUNT
	};


	struct ReturnType {
		int status = 200;
		std::string body;
		std::string_view type;
		int64_t ts = 0;
		unsigned expires = 0;
	};

	Router();
	~Router();

	bool init(const char* config, const std::string& root = ".", int pool_size=4);

	std::string const& publicDir() const;

	ReturnType get(const std::string& path, bool mobile = false);
	ReturnType post(const std::string& path, const std::string& body);

	void cleanup();

	static std::string method(Method);

	void dump() const;

	struct Context;

private:

	std::unique_ptr<Context> ctx;
};

