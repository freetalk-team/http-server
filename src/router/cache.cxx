#include "cache.h"

#include <mutex>
#include <print>

int64_t Cache::put(const std::string& key, const std::string& content, const std::string_view type, unsigned seconds) {
		
	auto ttl = std::chrono::steady_clock::now() + std::chrono::seconds(seconds);

	std::unique_lock lock(mutex_);

	// type if valid
	cache_[key] = { content, type, ttl, seconds };
	// type is not valid

	return ttl.time_since_epoch().count();
}

std::optional<Cache::Result> 
Cache::get(const std::string& key) {
	std::shared_lock lock(mutex_);
	auto it = cache_.find(key);
	if (it != cache_.end() && it->second.expires > std::chrono::steady_clock::now()) {
		auto& r = it->second;
		return { { r.content, r.type, r.expires.time_since_epoch().count(), r.seconds } };
	}
	// Upgrade to exclusive lock to erase expired entry
	// lock.unlock();
	// std::unique_lock ulock(mutex_);
	// cache_.erase(key);
	return std::nullopt;
}

void Cache::cleanup() {

	int cleaned = 0;

	{
		std::unique_lock lock(mutex_);
		auto now = std::chrono::steady_clock::now();
		for (auto it = cache_.begin(); it != cache_.end(); ) {
			if (it->second.expires <= now) {
				it = cache_.erase(it);
				cleaned++;
			} else {
				++it;
			}
		}
	}

	if (cleaned > 0)
		std::println("Cache cleaned routes: {0}", cleaned);
}