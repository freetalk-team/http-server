#pragma once

#include <chrono>
#include <string>
#include <optional>
#include <shared_mutex>
#include <unordered_map>

template <typename ContentType>
class Cache {
public:

	struct Result {
		ContentType& body;
		std::string_view type;
		int64_t ts;
		unsigned expires;
	};


	Result put(const std::string& key, ContentType&&, std::string_view type, unsigned seconds = 10*60 );

	template <typename T = ContentType>
	std::enable_if_t<std::is_copy_constructible_v<T>, Result>
	put(const std::string& key, const T&, std::string_view type, unsigned seconds = 10 * 60);

	std::optional<Result> 
	get(const std::string& key);

	void cleanup();

private:
	struct Entry {
		ContentType content;
		std::string_view type;
		std::chrono::steady_clock::time_point expires;
		unsigned seconds;
	};

	std::unordered_map<std::string, Entry> cache_;
	mutable std::shared_mutex mutex_;
};