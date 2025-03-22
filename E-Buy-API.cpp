#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/strand.hpp>
#include <boost/config.hpp>
#include <nlohmann/json.hpp>
#include <iostream>
#include <memory>
#include <string>
#include <thread>

#include "routers/api.h"

using namespace std;

namespace beast = boost::beast; // from <boost/beast.hpp>
namespace http = beast::http;   // from <boost/beast/http.hpp>
namespace net = boost::asio;    // from <boost/asio.hpp>
using tcp = net::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

// This function produces an HTTP response for the given request.
static http::response<http::string_body> handle_request(http::request<http::string_body> const& req, http::response<http::string_body>& res)
{
	cout << http::to_string(req.method()) << " " << req.target() << endl;

	if (req.target().starts_with("/api")) return routers::api::handle_resquest(req, res);

	return http::response<http::string_body>{http::status::not_found, req.version()};
}

// This class handles an HTTP server connection.
class Session : public enable_shared_from_this<Session>
{
	tcp::socket socket_;
	beast::flat_buffer buffer_;
	http::request<http::string_body> req_;

public:
	explicit Session(tcp::socket socket) : socket_(move(socket)) {}

	void run()
	{
		do_read();
	}

private:
	void do_read()
	{
		auto self(shared_from_this());
		http::async_read(socket_, buffer_, req_, [this, self](beast::error_code ec, size_t) {
			if (!ec) {
				http::response<http::string_body> res{ http::status::ok, req_.version() };
				res.set(http::field::server, "Beast");
				res.set(http::field::content_type, "application/json");
				res.keep_alive(req_.keep_alive());

				do_write(handle_request(req_, res));
			}
			});
	}

	void do_write(http::response<http::string_body> res)
	{
		auto self(shared_from_this());
		auto sp = make_shared<http::response<http::string_body>>(move(res));
		http::async_write(socket_, *sp, [this, self, sp](beast::error_code ec, size_t) {
			socket_.shutdown(tcp::socket::shutdown_send, ec);
			});
	}
};

// This class accepts incoming connections and launches the sessions.
class Listener : public enable_shared_from_this<Listener>
{
	net::io_context& ioc_;
	tcp::acceptor acceptor_;

public:
	Listener(net::io_context& ioc, tcp::endpoint endpoint) : ioc_(ioc), acceptor_(net::make_strand(ioc))
	{
		beast::error_code ec;

		// Open the acceptor
		acceptor_.open(endpoint.protocol(), ec);
		if (ec)
		{
			cerr << "Open error: " << ec.message() << endl;
			return;
		}

		// Allow address reuse
		acceptor_.set_option(net::socket_base::reuse_address(true), ec);
		if (ec)
		{
			cerr << "Set option error: " << ec.message() << endl;
			return;
		}

		// Bind to the server address
		acceptor_.bind(endpoint, ec);
		if (ec)
		{
			cerr << "Bind error: " << ec.message() << endl;
			return;
		}

		// Start listening for connections
		acceptor_.listen(net::socket_base::max_listen_connections, ec);
		if (ec)
		{
			cerr << "Listen error: " << ec.message() << endl;
			return;
		}

		do_accept();
	}

	/*void run()
	{
		do_accept();
	}*/

private:
	void do_accept()
	{
		acceptor_.async_accept(net::make_strand(ioc_), [this](beast::error_code ec, tcp::socket socket) {
			if (!ec)
			{
				make_shared<Session>(move(socket))->run();
			}
			do_accept();
			});
	}
};

int main()
{
	try
	{
		auto const address = net::ip::make_address("0.0.0.0");
		unsigned short port = 3000;

		net::io_context ioc{ 1 };

		const auto test = make_shared<Listener>(ioc, tcp::endpoint{ address, port });
		//test->run(); // This doesn't work even though it's in the example code

		ioc.run();
	}
	catch (const exception& e)
	{
		cerr << "Error: " << e.what() << endl;
	}
}
