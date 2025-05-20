#pragma once

#include "base.h"
#include "cache.h"

class ExecutorJson : public Executor {

public:

	using Executor::Executor;

	ReturnType get(const Route&, const QueryParams& = {});
	ReturnType put(const Route&, const std::string& body, const QueryParams& = {});

	void cleanup();

private:
	ReturnType get(soci::session&, const Route&, const QueryParams&);
	ReturnType ls(soci::session&, const Route&, const QueryParams&);

private:
	Cache<std::string> cache;
};