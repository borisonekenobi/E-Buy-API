#include <iostream>
#include <memory>
#include <string>
#include <boost/asio/strand.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>

#include "routers/api.h"

using namespace std;

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = net::ip::tcp;

static http::response<http::string_body> handle_request(http::request<http::string_body> const& req,
                                                        http::response<http::string_body>& res)
{
	cout << http::to_string(req.method()) << " " << req.target() << endl;

	if (req.target().starts_with("/api")) return routers::api::handle_request(req, res);

	return http::response<http::string_body>{http::status::not_found, req.version()};
}

class Session : public enable_shared_from_this<Session>
{
	tcp::socket socket_;
	beast::flat_buffer buffer_;
	http::request<http::string_body> req_;

public:
	explicit Session(tcp::socket socket) : socket_(move(socket))
	{
	}

	void run()
	{
		do_read();
	}

private:
	void do_read()
	{
		async_read(socket_, buffer_, req_, [this, self(shared_from_this())](const beast::error_code& ec, size_t)
		{
			if (!ec)
			{
				http::response<http::string_body> res{http::status::ok, req_.version()};
				res.set(http::field::server, "Beast");
				res.set(http::field::content_type, "application/json");
				res.set(http::field::access_control_allow_origin, "*");
				res.keep_alive(req_.keep_alive());

				do_write(handle_request(req_, res));
			}
		});
	}

	void do_write(http::response<http::string_body> res)
	{
		auto sp = make_shared<http::response<http::string_body>>(move(res));
		http::async_write(socket_, *sp, [this, self(shared_from_this()), sp](beast::error_code ec, size_t)
		{
			socket_.shutdown(tcp::socket::shutdown_send, ec);
		});
	}
};

class Listener : public enable_shared_from_this<Listener>
{
	net::io_context& ioc_;
	tcp::acceptor acceptor_;

public:
	Listener(net::io_context& ioc, const tcp::endpoint& endpoint) : ioc_(ioc), acceptor_(make_strand(ioc))
	{
		beast::error_code ec;

		acceptor_.open(endpoint.protocol(), ec);
		if (ec)
		{
			cerr << "Open error: " << ec.message() << endl;
			return;
		}

		acceptor_.set_option(net::socket_base::reuse_address(true), ec);
		if (ec)
		{
			cerr << "Set option error: " << ec.message() << endl;
			return;
		}

		acceptor_.bind(endpoint, ec);
		if (ec)
		{
			cerr << "Bind error: " << ec.message() << endl;
			return;
		}

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
		acceptor_.async_accept(make_strand(ioc_), [this](const beast::error_code& ec, tcp::socket socket)
		{
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

		net::io_context ioc{1};

		const auto test = make_shared<Listener>(ioc, tcp::endpoint{address, port});
		//test->run(); // This doesn't work even though it's in the example code

		ioc.run();
	}
	catch (const exception& e)
	{
		cerr << "Error: " << e.what() << endl;
	}
}
