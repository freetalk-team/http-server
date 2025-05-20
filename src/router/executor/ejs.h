#pragma once

#include "soci_fwd.h"

#include <string>
#include <string_view>
#include <variant>
#include <vector>


using rowset = soci::rowset<soci::row>;
using row = soci::row;

using plain = std::string_view;
struct if_block;
struct for_block;

using block = std::variant<plain, if_block, for_block>;
using sequence = std::vector<block>;


struct if_block {

	std::string cond;
	sequence _true, _false;

	if_block(const std::string_view& c) : cond(c.data(), c.size()) {}

};

struct for_block {

	std::string cond;
	sequence _loop;

	for_block(const std::string_view& c) : cond(c.data(), c.size()) {}
};


class Ejs {
	public:

	Ejs();
	~Ejs();

	Ejs(Ejs&&) noexcept;
	Ejs& operator=(Ejs&&) noexcept;

	// Prevent copying
	Ejs(const Ejs&) = delete;
	Ejs& operator=(const Ejs&) = delete;

	bool loadFile(const std::string&);

	std::string execute(const std::string&);

	struct Env {
		Ejs& ejs;
		void* ctx;
		bool own = false;

		~Env();

		std::string execute() const;

		void var(const std::string& name, int) const;
		void var(const std::string& name, double) const;
		void var(const std::string& name, const std::string&) const;
		void var(const std::string& name, const char*) const;
		void var(const std::string& name, bool) const;

		void var(const std::string& name, const row&) const;
		void vars(const row&) const;
		void var(const std::string& name, const rowset&) const;
		//void var(const std::string& name, Au)

		struct Object {
			void* ctx;
			std::string name;

			Object(void* ctx, const std::string& name);
			~Object();

			Object const& var(const std::string& name, const std::string&) const;
		};

		Object object(const std::string& name) const;

		int length(const std::string& name) const;

		void schemaSqlite(const std::string& name, const rowset&) const;
	};

	Env env(const char* id = nullptr);

	const sequence& getFlow() const { return blocks; }

private:

	void* ctx;

	std::string content;
	sequence blocks;
};