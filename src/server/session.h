

template<typename Stream>
net::awaitable<void, executor_type>
run_websocket_session(
	Stream& stream,
	beast::flat_buffer& buffer,
	http::request<http::string_body> req,
	beast::string_view doc_root)
{
	auto cs = co_await net::this_coro::cancellation_state;
	auto ws = websocket::stream<Stream&>{ stream };

	// Set suggested timeout settings for the websocket
	ws.set_option(
		websocket::stream_base::timeout::suggested(beast::role_type::server));

	// Set a decorator to change the Server of the handshake
	ws.set_option(websocket::stream_base::decorator(
		[](websocket::response_type& res)
		{
			res.set(
				http::field::server,
				std::string(kServer) +
					" advanced-server-flex");
		}));

	// Accept the websocket handshake
	co_await ws.async_accept(req);

	while(!cs.cancelled())
	{
		// Read a message
		auto [ec, _] = co_await ws.async_read(buffer, net::as_tuple);

		if(ec == websocket::error::closed || ec == ssl::error::stream_truncated)
			co_return;

		if(ec)
			throw boost::system::system_error{ ec };

		// Echo the message back
		ws.text(ws.got_text());
		co_await ws.async_write(buffer.data());

		// Clear the buffer
		buffer.consume(buffer.size());
	}

	// A cancellation has been requested, gracefully close the session.
	auto [ec] = co_await ws.async_close(
		websocket::close_code::service_restart, net::as_tuple);

	if(ec && ec != ssl::error::stream_truncated)
		throw boost::system::system_error{ ec };
}

template<typename Stream>
net::awaitable<void, executor_type>
run_session(
	Stream& stream,
	beast::flat_buffer& buffer,
	beast::string_view doc_root)
{
	auto cs = co_await net::this_coro::cancellation_state;

	while(!cs.cancelled())
	{
		http::request_parser<http::string_body> parser;
		parser.body_limit(10000);

		auto [ec, _] =
			co_await http::async_read(stream, buffer, parser, net::as_tuple);

		if(ec == http::error::end_of_stream)
			co_return;

		if(websocket::is_upgrade(parser.get()))
		{
			// The websocket::stream uses its own timeout settings.
			beast::get_lowest_layer(stream).expires_never();

			co_await run_websocket_session(
				stream, buffer, parser.release(), doc_root);

			co_return;
		}

		auto res = handle_request(doc_root, parser.release());
		if(!res.keep_alive())
		{
			co_await beast::async_write(stream, std::move(res));
			co_return;
		}

		co_await beast::async_write(stream, std::move(res));
	}
}

net::awaitable<void, executor_type>
plain_session(
	stream_type stream,
	beast::string_view doc_root) 
{
	beast::flat_buffer buffer;

	co_await run_session(stream, buffer, doc_root);

	if(!stream.socket().is_open())
		co_return;

	stream.socket().shutdown(net::ip::tcp::socket::shutdown_send);
}


net::awaitable<void, executor_type>
detect_session(
	stream_type stream,
	ssl::context& ctx,
	beast::string_view doc_root)
{
	beast::flat_buffer buffer;

	// Allow total cancellation to change the cancellation state of this
	// coroutine, but only allow terminal cancellation to propagate to async
	// operations. This setting will be inherited by all child coroutines.
	co_await net::this_coro::reset_cancellation_state(
		net::enable_total_cancellation(), net::enable_terminal_cancellation());

	// We want to be able to continue performing new async operations, such as
	// cleanups, even after the coroutine is cancelled. This setting will be
	// inherited by all child coroutines.
	co_await net::this_coro::throw_if_cancelled(false);

	stream.expires_after(std::chrono::seconds(30));

	if(co_await beast::async_detect_ssl(stream, buffer))
	{
		ssl::stream<stream_type> ssl_stream{ std::move(stream), ctx };

		auto bytes_transferred = co_await ssl_stream.async_handshake(
			ssl::stream_base::server, buffer.data());

		buffer.consume(bytes_transferred);

		co_await run_session(ssl_stream, buffer, doc_root);

		if(!ssl_stream.lowest_layer().is_open())
			co_return;

		// Gracefully close the stream
		auto [ec] = co_await ssl_stream.async_shutdown(net::as_tuple);
		if(ec && ec != ssl::error::stream_truncated)
			throw boost::system::system_error{ ec };
	}
	else
	{
		co_await run_session(stream, buffer, doc_root);

		if(!stream.socket().is_open())
			co_return;

		stream.socket().shutdown(net::ip::tcp::socket::shutdown_send);
	}
}
