//
// Copyright (c) 2022 Klemens D. Morgenstern (klemens dot morgenstern at gmx dot net)
// Copyright (c) 2024 Mohammad Nejati
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/beast
//

//------------------------------------------------------------------------------
//
// Example: Advanced server, flex (plain + SSL)
//
//------------------------------------------------------------------------------

// #include "example/common/server_certificate.hpp"

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/beast.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/stacktrace.hpp>

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <list>
#include <string>
#include <thread>
#include <vector>
#include <filesystem>
#include <print>

#include "utils.h"
#include "router/router.h"


namespace beast     = boost::beast;
namespace http      = beast::http;
namespace websocket = beast::websocket;
namespace net       = boost::asio;
namespace ssl       = boost::asio::ssl;
namespace fs        = std::filesystem;

using executor_type = net::strand<net::io_context::executor_type>;
using stream_type   = typename beast::tcp_stream::rebind_executor<executor_type>::other;
using acceptor_type = typename net::ip::tcp::acceptor::rebind_executor<executor_type>::other;

using namespace std::chrono_literals;

const auto CleanupInterval = 20s;

Router router;

#include "server/cache.h"
#include "server/request.h"
#include "server/session.h"
#include "server/timer.h"




/** A thread-safe task group that tracks child tasks, allows emitting
	cancellation signals to them, and waiting for their completion.
*/
class task_group
{
	std::mutex mtx_;
	net::steady_timer cv_;
	std::list<net::cancellation_signal> css_;

public:
	task_group(net::any_io_executor exec)
		: cv_{ std::move(exec), net::steady_timer::time_point::max() }
	{
	}

	task_group(task_group const&) = delete;
	task_group(task_group&&)      = delete;

	/** Adds a cancellation slot and a wrapper object that will remove the child
		task from the list when it completes.

		@param completion_token The completion token that will be adapted.

		@par Thread Safety
		@e Distinct @e objects: Safe.@n
		@e Shared @e objects: Safe.
	*/
	template<typename CompletionToken>
	auto
	adapt(CompletionToken&& completion_token)
	{
		auto lg = std::lock_guard{ mtx_ };
		auto cs = css_.emplace(css_.end());

		class remover
		{
			task_group* tg_;
			decltype(css_)::iterator cs_;

		public:
			remover(
				task_group* tg,
				decltype(css_)::iterator cs)
				: tg_{ tg }
				, cs_{ cs }
			{
			}

			remover(remover&& other) noexcept
				: tg_{ std::exchange(other.tg_, nullptr) }
				, cs_{ other.cs_ }
			{
			}

			~remover()
			{
				if(tg_)
				{
					auto lg = std::lock_guard{ tg_->mtx_ };
					if(tg_->css_.erase(cs_) == tg_->css_.end())
						tg_->cv_.cancel();
				}
			}
		};

		return net::bind_cancellation_slot(
			cs->slot(),
			net::consign(
				std::forward<CompletionToken>(completion_token),
				remover{ this, cs }));
	}

	/** Emits the signal to all child tasks and invokes the slot's
		handler, if any.

		@param type The completion type that will be emitted to child tasks.

		@par Thread Safety
		@e Distinct @e objects: Safe.@n
		@e Shared @e objects: Safe.
	*/
	void
	emit(net::cancellation_type type)
	{
		auto lg = std::lock_guard{ mtx_ };
		for(auto& cs : css_)
			cs.emit(type);
	}

	/** Starts an asynchronous wait on the task_group.

		The completion handler will be called when:

		@li All the child tasks completed.
		@li The operation was cancelled.

		@param completion_token The completion token that will be used to
		produce a completion handler. The function signature of the completion
		handler must be:
		@code
		void handler(
			boost::system::error_code const& error  // result of operation
		);
		@endcode

		@par Thread Safety
		@e Distinct @e objects: Safe.@n
		@e Shared @e objects: Safe.
	*/
	template<
		typename CompletionToken =
			net::default_completion_token_t<net::any_io_executor>>
	auto
	async_wait(
		CompletionToken&& completion_token =
			net::default_completion_token_t<net::any_io_executor>{})
	{
		return net::
			async_compose<CompletionToken, void(boost::system::error_code)>(
				[this, scheduled = false](
					auto&& self, boost::system::error_code ec = {}) mutable
				{
					if(!scheduled)
						self.reset_cancellation_state(
							net::enable_total_cancellation());

					if(!self.cancelled() && ec == net::error::operation_aborted)
						ec = {};

					{
						auto lg = std::lock_guard{ mtx_ };

						if(!css_.empty() && !ec)
						{
							scheduled = true;
							return cv_.async_wait(std::move(self));
						}
					}

					if(!std::exchange(scheduled, true))
						return net::post(net::append(std::move(self), ec));

					self.complete(ec);
				},
				completion_token,
				cv_);
	}
};


net::awaitable<void, executor_type>
listen(
	task_group& task_group,
	ssl::context& ctx,
	net::ip::tcp::endpoint endpoint,
	beast::string_view doc_root)
{
	auto cs       = co_await net::this_coro::cancellation_state;
	auto executor = co_await net::this_coro::executor;
	auto acceptor = acceptor_type{ executor, endpoint };

	// Allow total cancellation to propagate to async operations.
	co_await net::this_coro::reset_cancellation_state(
		net::enable_total_cancellation());

	while(!cs.cancelled())
	{
		auto socket_executor = net::make_strand(executor.get_inner_executor());
		auto [ec, socket] =
			co_await acceptor.async_accept(socket_executor, net::as_tuple);

		if(ec == net::error::operation_aborted)
			co_return;

		if(ec)
			throw boost::system::system_error{ ec };

		net::co_spawn(
			std::move(socket_executor),
			// detect_session(stream_type{ std::move(socket) }, ctx, doc_root),
			plain_session(stream_type{ std::move(socket) }, doc_root),
			task_group.adapt(
				[](std::exception_ptr e)
				{
					if(e)
					{
						// std::cerr << boost::stacktrace::stacktrace();

						try
						{
							std::rethrow_exception(e);
						}
						catch(std::exception& e)
						{
							std::cerr << "Error in session: " << e.what() << "\n";
						}
					}
				}));
	}
}

net::awaitable<void, executor_type>
handle_signals(task_group& task_group)
{
	auto executor   = co_await net::this_coro::executor;
	auto signal_set = net::signal_set{ executor, SIGINT, SIGTERM };

	auto sig = co_await signal_set.async_wait();

	if(sig == SIGINT)
	{
		std::cout << "Gracefully cancelling child tasks...\n";
		task_group.emit(net::cancellation_type::total);

		// Wait a limited time for child tasks to gracefully cancell
		auto [ec] = co_await task_group.async_wait(
			net::as_tuple(net::cancel_after(std::chrono::seconds{ 5 })));

		if(ec == net::error::operation_aborted) // Timeout occurred
		{
			std::cout << "Sending a terminal cancellation signal...\n";
			task_group.emit(net::cancellation_type::terminal);
			co_await task_group.async_wait();
		}

		std::cout << "Child tasks completed.\n";
	}
	else // SIGTERM
	{
		executor.get_inner_executor().context().stop();
	}
}

void signal_handler(int signum) {
	std::cerr << "Caught signal " << signum << std::endl;
	std::cerr << boost::stacktrace::stacktrace();
	std::exit(EXIT_FAILURE);
}

int
main(int argc, char* argv[])
{
	// Check command line arguments.
	// Check command line arguments.
	if (argc < 3) {
		std::cerr << "Usage: server <address> <port> <doc_root=.> <config=router.conf> <num_workers=4>\n";
		std::cerr << "  For IPv4, try:\n";
		std::cerr << "    server 0.0.0.0 80 . config.js 4\n";
		std::cerr << "  For IPv6, try:\n";
		std::cerr << "    server 0::0 80 . config.js 4\n";
		return EXIT_FAILURE;
	}

	const char* args[] = { argv[1], argv[2],
		argc < 4 ? "." : argv[3],
		argc < 5 ? "router.conf" : argv[4],
		argc < 6 ? "4" : argv[5]
	};


	auto const address = net::ip::make_address(args[0]);
	auto const port = static_cast<unsigned short>(std::atoi(args[1]));
	auto const endpoint = net::ip::tcp::endpoint{ address, port };
	auto const threads = std::max<int>(1, std::atoi(args[4]));
	// auto const threads = 1;

	if (!router.init(args[3], args[2])) {
		std::cerr << "Failed to initialize router\n";
		return EXIT_FAILURE;
	}

	router.dump();

	beast::string_view const doc_root(router.publicDir());
	

	// The io_context is required for all I/O
	net::io_context ioc{ threads };

	// The SSL context is required, and holds certificates
	ssl::context ctx{ ssl::context::tlsv12 };

	// This holds the self-signed certificate used by the server
	// load_server_certificate(ctx);

	std::signal(SIGSEGV, signal_handler);

	// Track coroutines
	task_group task_group{ ioc.get_executor() };

	// Create and launch a listening coroutine
	net::co_spawn(
		net::make_strand(ioc),
		listen(task_group, ctx, endpoint, doc_root),
		task_group.adapt(
			[](std::exception_ptr e)
			{
				if(e)
				{
					try
					{
						std::rethrow_exception(e);
					}
					catch(std::exception& e)
					{
						std::cerr << "Error in listener: " << e.what() << "\n";
					}
				}
			}));

	// Create and launch a signal handler coroutine
	net::co_spawn(
		net::make_strand(ioc), handle_signals(task_group), net::detached);

	// Run the I/O service on the requested number of threads
	std::vector<std::thread> v;
	v.reserve(threads - 1);
	for(auto i = threads - 1; i > 0; --i)
		v.emplace_back([&ioc] { ioc.run(); });

	net::co_spawn(ioc,
		start_periodic_timer(ioc, CleanupInterval, []() {
			router.cleanup();
		}),
		task_group.adapt([](std::exception_ptr e) {

		}));

	ioc.run();

	// Block until all the threads exit
	for(auto& t : v)
		t.join();

	std::cout << "Bye!\n";

	return EXIT_SUCCESS;
}
