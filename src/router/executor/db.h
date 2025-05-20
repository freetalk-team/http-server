#pragma once

#include "soci_fwd.h"

#include <string>
#include <map>
#include <optional>
#include <vector>


using Params = std::map<std::string, std::string>;

namespace db {

struct GetParams {
	std::string_view value;
	std::string_view column = "id";
	std::string_view attributes = "*";
};

std::string blob(soci::row const&, int = 0);

std::optional<soci::row> 
get(soci::session&, const std::string& table, const Params&, std::string_view attributes = "*");

std::optional<soci::row> 
get(soci::session&, const std::string& table, const GetParams&);

soci::rowset<soci::row> 
ls(soci::session&, const std::string& table, int offset, int limit);

soci::rowset<soci::row> 
ls(soci::session&, const std::string& table, const Params&, int offset, int limit);

soci::rowset<soci::row> 
search(soci::session&, const std::string& table, const std::string& fts_table, const std::vector<std::string>& rank, const std::string& value, int offset, int limit);

std::optional<std::string> 
insert(soci::session&, const std::string& table, const std::string& json);

std::optional<std::string> 
insert(soci::session&, const std::string& table, const Params& form);

bool del(soci::session&, const std::string& table, const std::string& id);

}