#include <print>

#include "config.h"
#include "context.h"

using Context = Router::Context;
using Route = Context::Route;

void js_route(CScriptVar *v, void *userdata) {
	printf("> %s\n", v->getParameter("path")->getString().c_str());
}

void js_request(Router::Method m, CScriptVar *v, void *userdata) {

	// std::println("JS GET:");

	auto r = reinterpret_cast<Context*>(userdata);

	Route route;

	auto& path = v->getParameter("path")->getString();
	auto params = v->getParameter("params");

	if (params->isString()) {
		// todo: static routes
	}

	else {

		CScriptVarLink* v;

		v = params->findChild("db");
		if (v) {

			route.db.table = v->var->getParameter("table")->getString();

			auto op = v->var->findChild("op");
			if (op)
				route.db.op = op->var->getString();

			auto limit = v->var->findChild("limit");
			if (limit)
				route.db.limit = limit->var->getInt();

			auto alias = v->var->findChild("alias");
			if (alias)
				route.db.alias = limit->var->getString();
		}

		v = params->findChild("ejs");
		if (v) {
			route.ejs = v->var->getString();
		}

		v = params->findChild("cache");
		if (v) {
			route.cache = v->var->getInt();
		}

		r->addRoute(m, path.c_str(), route);

	}

	// printf("> %s\n", path.c_str());
}

void js_get(CScriptVar *v, void *userdata) {
	js_request(Router::GET, v, userdata);
}

void js_post(CScriptVar *v, void *userdata) {
	js_request(Router::POST, v, userdata);
}

void js_database(CScriptVar *v, void *userdata) {

	auto url = v->getParameter("url")->getString();
	// printf("> %s\n", url);

	auto r = reinterpret_cast<Context*>(userdata);

	r->database(url);
}

void js_set(CScriptVar *v, void *userdata) {

	auto name = v->getParameter("name")->getString();
	auto value = v->getParameter("value");

	Script::addRoot(name, value);
}

void js_public_dir(CScriptVar *v, void *userdata) {
	auto r = reinterpret_cast<Context*>(userdata);

	r->public_dir = v->getParameter("path")->getString();
}

void js_views_dir(CScriptVar *v, void *userdata) {
	auto r = reinterpret_cast<Context*>(userdata);

	r->views_dir = v->getParameter("path")->getString();
}


void register_http_functions(Script& js, void* p) {

	// config
	js.addNative("function public(path)", js_public_dir, p);
	js.addNative("function views(path)", js_views_dir, p);
	js.addNative("function database(url)", js_database, p);
	js.addNative("function set(name, value)", js_set, p);

	// routes
	js.addNative("function route(path, params)", js_route, p);
	js.addNative("function post(path, params)", js_post, p);
	js.addNative("function get(path, params)", js_get, p);
	js.addNative("function delete(path, params)", js_get, p);
}