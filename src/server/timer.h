


net::awaitable<void> start_periodic_timer(
	net::io_context& ioc,
	std::chrono::seconds interval,
	std::function<void()> tick)
{
	net::steady_timer timer(ioc);

	// Attach to cancellation
	// net::cancellation_slot slot = co_await net::this_coro::cancellation_slot;
	// cancel_signal.slot().assign(slot);

	for (;;)
	{
		tick(); // Call the user-defined tick

		timer.expires_after(interval);
		boost::system::error_code ec;
		co_await timer.async_wait(net::redirect_error(net::use_awaitable, ec));

		if (ec == net::error::operation_aborted)
		{
			std::cout << "Periodic timer cancelled\n";
			co_return;
		}
	}
}
