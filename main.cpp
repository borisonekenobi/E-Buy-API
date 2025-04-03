#include <iostream>
#include <memory>
#include <string>
#include <boost/asio/strand.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>

#include "authentication-functions.h"
#include "routers/api.h"

#define WHITE_FOREGROUND(x) "\033[37m" << (x) << "\033[0m"
#define GREEN_FOREGROUND(x) "\033[32m" << (x) << "\033[0m"
#define CYAN_FOREGROUND(x) "\033[36m" << (x) << "\033[0m"
#define YELLOW_FOREGROUND(x) "\033[33m" << (x) << "\033[0m"
#define RED_FOREGROUND(x) "\033[31m" << (x) << "\033[0m"

using namespace std;

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = net::ip::tcp;

static void print_status(http::request<http::string_body> const& req,
                         http::response<http::string_body>& res,
                         const long long elapsed)
{
	const auto method = req.method_string();
	const auto target = req.target();
	const auto status_code = res.result_int();
	const auto payload_size = res.body().empty() ? "-" : to_string(res.body().size());

	cout << method << " " << target << " ";

	switch (status_code / 100)
	{
	case 1:
		cout << WHITE_FOREGROUND(status_code);
		break;
	case 2:
		cout << GREEN_FOREGROUND(status_code);
		break;
	case 3:
		cout << CYAN_FOREGROUND(status_code);
		break;
	case 4:
		cout << YELLOW_FOREGROUND(status_code);
		break;
	case 5:
	default:
		cout << RED_FOREGROUND(status_code);
		break;
	}

	cout << " " << elapsed << " ms - " << payload_size << endl;
}

static http::response<http::string_body> handle_request(http::request<http::string_body> const& req,
                                                        http::response<http::string_body>& res)
{
	try
	{
		if (req.method() == http::verb::options)
		{
			res.result(http::status::no_content);
			res.prepare_payload();
			return res;
		}
		
		const auto now = chrono::system_clock::now();

		if (req.target().starts_with("/api")) res = routers::api::handle_request(req, res);

		const auto elapsed = chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now() - now).count();
		print_status(req, res, elapsed);
		return res;
	}
	catch (const exception& e)
	{
		cerr << "Error: " << e.what() << endl;
		res.result(http::status::internal_server_error);
		res.body() = R"({"message": "Internal Server Error"})";
		res.prepare_payload();
		print_status(req, res, 0);
		return res;
	}
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
				res.set(http::field::access_control_allow_methods, "GET, POST, PUT, DELETE, OPTIONS");
				res.set(http::field::access_control_allow_headers, "Content-Type, Authorization");
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
		initialize_auth();

		auto const address = net::ip::make_address("0.0.0.0");
		constexpr unsigned short port = 3000;

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
