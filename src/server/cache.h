

template <class Request, class Response>
bool set_cache_headers(const Request& req,
					   Response& res,
					   const std::string& etag,
					   int max_age_seconds) {

	// Set Cache-Control
	res.set(http::field::cache_control, "public, max-age=" + std::to_string(max_age_seconds));
	res.set(http::field::etag, etag);

	// Check If-None-Match
	auto it = req.find(http::field::if_none_match);
	if (it != req.end() && it->value() == etag) {
		res.result(http::status::not_modified);
		return true; // Indicates content doesn't need to be sent
	}

	return false; // Content should be sent
}

template <class Body, class Allocator>
bool set_cache_headers(const http::request<Body, http::basic_fields<Allocator>>& req,
					   http::response<http::file_body>& res,
					   const fs::path& file_path, size_t size,
					   int max_age_seconds = 3600) {

	// Generate ETag
	std::string etag = compute_etag(file_path, size);
	
	if (set_cache_headers(req, res, etag, max_age_seconds)) {
		res.body().close(); // Release file
		return true;
	}

	return false;

}

template <class Body, class Allocator>
bool set_cache_headers(const http::request<Body, http::basic_fields<Allocator>>& req,
					   http::response<http::string_body>& res,
					   size_t size, int64_t ts, 
					    int max_age_seconds = 3600) {
	if (max_age_seconds == 0)
		return false;

	// Generate ETag
	std::string etag = compute_etag(ts, size);
	return set_cache_headers(req, res, etag, max_age_seconds);
}
