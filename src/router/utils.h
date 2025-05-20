#pragma once

#include <string>
#include <string_view>
#include <map>
#include <optional>
#include <cstdint>


// Function to decode and verify JWT
std::optional<std::string>
decode_jwt(const std::string& token, const std::string& secret_key, const std::string& name);

bool 
match_jwt(const std::string& token, const std::string& secret_key, const std::string& key, const std::string& value);


std::optional<std::string> 
get_env_var(const std::string& key);

std::string 
get_env_var(const std::string& key, const std::string& def);
