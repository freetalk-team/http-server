#include "cache.h"
#include "ejs.h"

#include <mutex>
#include <print>

template <typename R, typename Cache, typename... Args>
R emplace(Cache& cache, const std::string& key, Args&&... args) {
	auto r = cache.emplace(std::piecewise_construct,
						   std::forward_as_tuple(key),
						   std::forward_as_tuple(std::forward<Args>(args)...));
	if (!r.second)
		throw std::runtime_error("Failed to add to cache");

	auto& e = r.first->second;

	return {
		e.content,
		e.type,
		e.expires.time_since_epoch().count(),
		e.seconds
	};
}

template <typename T>
typename Cache<T>::Result
Cache<T>::put(const std::string& key, T&& content, const std::string_view type, unsigned seconds) {
		
	auto ttl = std::chrono::steady_clock::now() + std::chrono::seconds(seconds);

	std::unique_lock lock(mutex_);

	return emplace<Result>(cache_, key, std::forward<T>(content), type, ttl, seconds);
}

template <typename ContentType>
template <typename T>
std::enable_if_t<std::is_copy_constructible_v<T>, typename Cache<ContentType>::Result>
Cache<ContentType>::put(const std::string& key, const T& content, std::string_view type, unsigned seconds) {
		
	auto ttl = std::chrono::steady_clock::now() + std::chrono::seconds(seconds);

	std::unique_lock lock(mutex_);

	return emplace<Result>(cache_, key, content, type, ttl, seconds);
}

template <typename T>
std::optional<typename Cache<T>::Result> 
Cache<T>::get(const std::string& key) {
	std::shared_lock lock(mutex_);
	auto it = cache_.find(key);
	if (it != cache_.end()) {
		std::println("Found in cache: {0}", key);

		auto& r = it->second;
		return { { r.content, r.type, r.expires.time_since_epoch().count(), r.seconds } };
	}
	// Upgrade to exclusive lock to erase expired entry
	// lock.unlock();
	// std::unique_lock ulock(mutex_);
	// cache_.erase(key);
	return std::nullopt;
}

template <typename ContentType>
void Cache<ContentType>::cleanup() {

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

template class Cache<std::string>;
template class Cache<Ejs>;

// Function template instantiations
template Cache<std::string>::Result Cache<std::string>::put<std::string>(
    const std::string&, const std::string&, std::string_view, unsigned);
