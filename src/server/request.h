
const std::string kServer = "sipme";

template<class Request>
bool is_mobile(Request const& req) {

	if (auto&& h = req.base().find(http::field::user_agent); h != req.base().end()) {
		std::string_view ua = h->name_string();
		return is_mobile_user_agent(ua);
	}

	return false;
}


// Return a response for the given request.
//
// The concrete type of the response message (which depends on the
// request), is type-erased in message_generator.
template<class Body, class Allocator>
http::message_generator
handle_request(
	beast::string_view doc_root,
	http::request<Body, http::basic_fields<Allocator>>&& req)
{
	// Returns a bad request response
	auto const bad_request =
	[&req](beast::string_view why)
	{
		http::response<http::string_body> res{http::status::bad_request, req.version()};
		res.set(http::field::server, kServer);
		res.set(http::field::content_type, "text/html");
		res.keep_alive(req.keep_alive());
		res.body() = std::string(why);
		res.prepare_payload();
		return res;
	};

	// Returns a not found response
	auto const not_found =
	[&req](beast::string_view target)
	{
		http::response<http::string_body> res{http::status::not_found, req.version()};
		res.set(http::field::server, kServer);
		res.set(http::field::content_type, "text/html");
		res.keep_alive(req.keep_alive());
		res.body() = "The resource '" + std::string(target) + "' was not found.";
		res.prepare_payload();
		return res;
	};

	// Returns a server error response
	auto const server_error =
	[&req](beast::string_view what)
	{
		http::response<http::string_body> res{http::status::internal_server_error, req.version()};
		res.set(http::field::server, kServer);
		res.set(http::field::content_type, "text/html");
		res.keep_alive(req.keep_alive());
		res.body() = what;
		res.prepare_payload();
		return res;
	};

	// Make sure we can handle the method
	if( req.method() != http::verb::get &&
		req.method() != http::verb::head)
		return bad_request("Unknown HTTP-method");

	auto&& target = req.target();

	// Request path must be absolute and not contain "..".
	if( target.empty() ||
		target[0] != '/' ||
		target.find("..") != beast::string_view::npos)
		return bad_request("Illegal request-target");



	auto&& mime = mime_type(target);

	if (mime.size() == 0) {
		// todo: call router

		auto r = router.get(target, is_mobile(req));

		if (r.status < 300) {

			http::response<http::string_body> res{http::status::ok, req.version()};
			res.set(http::field::server, kServer);
			res.set(http::field::content_type, r.type);

			if (!set_cache_headers(req, res, r.body.size(), r.ts, r.expires)) {

				res.set(http::field::content_encoding, "gzip");
				res.body() = r.body;
				res.prepare_payload();
			}

			res.keep_alive(req.keep_alive());

			return res;
		}

		if (r.status >= 500)
			return server_error(r.body.empty() ? "Internal error" : r.body);

		if (target.back() != '/')
			return not_found("Not found");
	}

	// Build the path to the requested file
	std::string path = path_cat(doc_root, target);
	
	if(target.back() == '/')
		path.append("index.html");

	std::println("GET: {0}", path);

	// Attempt to open the file
	beast::error_code ec;
	http::file_body::value_type body;
	body.open(path.c_str(), beast::file_mode::scan, ec);

	// Handle the case where the file doesn't exist
	if(ec == beast::errc::no_such_file_or_directory)
		return not_found(target);

	// Handle an unknown error
	if(ec)
		return server_error(ec.message());

	// Cache the size since we need it after the move
	auto const size = body.size();

	// Respond to HEAD request
	if(req.method() == http::verb::head)
	{
		http::response<http::empty_body> res{http::status::ok, req.version()};
		res.set(http::field::server, kServer);
		res.set(http::field::content_type, mime);
		res.content_length(size);
		res.keep_alive(req.keep_alive());
		return res;
	}

	std::string etag = compute_etag(path, size)
		, cache_control = "public, max-age=" + std::to_string(120);


	auto it = req.find(http::field::if_none_match);
	if (it != req.end() && it->value() == etag) {
		http::response<http::empty_body> res{http::status::not_modified, req.version()};

		res.set(http::field::etag, etag);
		res.set(http::field::cache_control, cache_control);
		res.keep_alive(req.keep_alive());

		return res;
	}
	

	// Respond to GET request
	http::response<http::file_body> res{
		std::piecewise_construct,
		std::make_tuple(std::move(body)),
		std::make_tuple(http::status::ok, req.version())};

	res.set(http::field::server, kServer);
	res.set(http::field::content_type, mime);
	res.set(http::field::etag, etag);
	res.set(http::field::cache_control, cache_control);

	res.content_length(body.size());
	res.keep_alive(req.keep_alive());

	return res;
}