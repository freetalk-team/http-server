#pragma once

#include <chrono>
#include <string>
#include <optional>
#include <shared_mutex>
#include <unordered_map>

class Cache {
public:

	struct Result {
		std::string body;
		std::string_view type;
		int64_t ts;
		unsigned expires;
	};

	int64_t put(const std::string& key, const std::string& content, const std::string_view& type, unsigned seconds = 10*60 );

	std::optional<Result> 
	get(const std::string& key);

	void cleanup();

private:
	struct Entry {
		std::string content;
		std::string_view type;
		std::chrono::steady_clock::time_point expires;
		unsigned seconds;
	};

	std::unordered_map<std::string, Entry> cache_;
	mutable std::shared_mutex mutex_;
};