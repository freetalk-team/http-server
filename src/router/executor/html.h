#pragma once

#include "base.h"
#include "cache.h"
#include "ejs.h"


class ExecutorHtml : public Executor {

public:

	ExecutorHtml(soci::connection_pool&, const std::string& views_dir);

	ReturnType get(const Route&, const QueryParams& = {});
	ReturnType put(const Route&, const std::string& body, const QueryParams& = {});


	void cleanup();

private:
	ReturnType get(soci::session&, const Route&, const QueryParams&);
	ReturnType get(Ejs::Env&, soci::session&, const Route&, const QueryParams&);

	ReturnType ls(soci::session&, const Route&, const QueryParams&);
	ReturnType ls(Ejs::Env&, soci::session&, const Route&, const QueryParams&);

	ReturnType schema(soci::session&, const Route&);

	ReturnType execute(const std::string& path);
	ReturnType execute(const std::string& path, const Executor::AuthInfo&);

	std::string templatePath(const std::string& path) const;

	Ejs::Env loadScript(const std::string& path, unsigned expires = 60);

private:

	const std::string& views_dir;

	Cache<std::string> cache;
	Cache<Ejs> scripts;

	//unsigned ejs_cache_seconds = 10 * 60;
};
