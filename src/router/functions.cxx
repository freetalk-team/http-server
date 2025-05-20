
#include "config.h"
#include "context.h"
#include "utils.h"

#include <boost/algorithm/string/join.hpp>

#include <print>

using Context = Router::Context;
using Route = Context::Route;

void js_route(CScriptVar *v, void *userdata) {
	printf("> %s\n", v->getParameter("path")->getString().c_str());
}

void set(unsigned& v, const std::string& name, CScriptVar* p) {
	auto child = p->findChild(name);
	if (child)
		v = child->var->getInt();
}

void set(std::string& v, const std::string& name, CScriptVar* p) {
	auto child = p->findChild(name);
	if (child)
		v = child->var->getString();
}

template <typename T>
void set(T& v, const std::string& name, CScriptVarLink* p) {
	set(v, name, p->var);
}

void parse_db(Route::Database& db, CScriptVar* params) {
	auto v = params->findChild("db");
	if (!v) return;
		
	db.table = v->var->getParameter("table")->getString();

	set(db.op, "op", v);
	set(db.limit, "limit", v);
	set(db.alias, "alias", v);
	set(db.created, "created", v);
	set(db.updated, "updated", v);

	if (auto c = v->var->findChild("attr"); c) {
		auto var = c->var;
		if (var->isArray()) {

			std::vector<std::string> attrs;
			for (int i = 0; i < var->getArrayLength(); ++i)
				attrs.push_back(var->getArrayIndex(i)->getString());

			db.attributes = boost::algorithm::join(attrs, ",");
		}
		else {
			db.attributes = var->getString();
		}
	}

	if (auto c = v->var->findChild("fts"); c) {

		Route::Database::Fts fts;
		auto var = c->var;

		fts.index = var->getParameter("index")->getString();

		auto t = var->findChild("table");
		if (t)
			fts.table = t->var->getString();
		else
			fts.table = db.table + "_fts";

		auto r = var->findChild("rank");
		if (r) {

			for (int i = 0; i < r->var->getArrayLength(); ++i) 
				fts.rank.push_back(r->var->getArrayIndex(i)->getString());
		}

		db.fts = std::move(fts);
	}

	if (auto c = v->var->findChild("bind"); c) {
		auto var = c->var;

		for (auto p = var->firstChild; p; p = p->nextSibling)
			db.bind[p->name] = p->var->getString();
	}

}

void js_request(Router::Method m, CScriptVar *v, void *userdata) {

	// std::println("JS GET:");

	auto ctx = reinterpret_cast<Context*>(userdata);

	auto params = v->getParameter("params");

	if (params->isString()) {
		// todo: static routes
	}

	else {

		auto& route = ctx->addRoute(m, v->getParameter("path")->getString());

		CScriptVarLink* v;

		parse_db(route.db, params);

		set(route.ejs, "ejs", params);
		set(route.cache, "cache", params);
		set(route.redirect, "redirect", params);

		if (auto c = params->findChild("role"); c)
			route.set_role(c->var->getString());

	}

	// printf("> %s\n", path.c_str());
}

void js_get(CScriptVar *v, void *userdata) {
	js_request(Router::GET, v, userdata);
}

void js_post(CScriptVar *v, void *userdata) {
	js_request(Router::POST, v, userdata);
}

void js_delete(CScriptVar *v, void *userdata) {
	js_request(Router::DELETE, v, userdata);
}

void js_database(CScriptVar *v, void *userdata) {

	auto url = v->getParameter("url")->getString();
	// printf("> %s\n", url);

	auto r = reinterpret_cast<Context*>(userdata);

	r->database(url);
}

void js_set(CScriptVar *v, void *userdata) {

	auto ctx = reinterpret_cast<Context*>(userdata);

	auto name = v->getParameter("name")->getString();
	auto value = v->getParameter("value")->getString();

	if (value[0] == '[' && value[value.length() - 1] == ']') {
		auto env = value.substr(1, value.length() - 2);
		auto def = v->getParameter("def");

		ctx->set(name, get_env_var(env, def->isUndefined() ? "" : def->getString()));

	}
	else {
		ctx->set(name, value);
	}

	// Script::addRoot(name, value);
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
	js.addNative("function set(name, value, def)", js_set, p);

	// routes
	js.addNative("function route(path, params)", js_route, p);
	js.addNative("function post(path, params)", js_post, p);
	js.addNative("function get(path, params)", js_get, p);
	js.addNative("function delete(path, params)", js_delete, p);
}