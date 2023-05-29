#include "WebServer.hpp"
#include "HTTP/error_pages.hpp"
#include "async/IOTaskHandler.hpp"
#include "async/Logger.hpp"
#include "utils/string.hpp"

WebServer::WebServer(void) : _logger(async::Logger::getLogger("WebServer"))
{
}

void WebServer::parseMaxBodySize(const ConfigContext &root_context)
{
	const char *dir_name = "client_max_body_size";

	if (root_context.countDirectivesByName(dir_name) != 1)
	{
		_logger << async::error << root_context.name() << " should have 1 "
				<< dir_name;
		root_context.throwException(PARSINGEXC_INVALID_N_DIR);
	}

	const ConfigDirective &body_size_directive
		= root_context.getNthDirectiveByName(dir_name, 0);

	if (body_size_directive.is_context())
	{
		_logger << async::error << dir_name << " should not be context";
		root_context.throwException(PARSINGEXC_UNDEF_DIR);
	}
	if (body_size_directive.nParameters() != 1)
	{
		_logger << async::error << dir_name << " should have 1 parameter(s)";
		body_size_directive.throwException(PARSINGEXC_INVALID_N_ARG);
	}

	_max_body_size = toNum<size_t>(body_size_directive.parameter(0));
	_logger << async::info << "global max body size is " << _max_body_size;
}

void WebServer::parseUploadStore(const ConfigContext &root_context)
{
	const char *dir_name = "upload_store";

	if (root_context.countDirectivesByName(dir_name) != 1)
	{
		_logger << async::error << root_context.name() << " should have 1 "
				<< dir_name;
		root_context.throwException(PARSINGEXC_INVALID_N_DIR);
	}

	const ConfigDirective &upload_directive
		= root_context.getNthDirectiveByName(dir_name, 0);

	if (upload_directive.is_context())
	{
		_logger << async::error << dir_name << " should not be context";
		root_context.throwException(PARSINGEXC_UNDEF_DIR);
	}
	if (upload_directive.nParameters() != 1)
	{
		_logger << async::error << dir_name << " should have 1 parameter(s)";
		upload_directive.throwException(PARSINGEXC_INVALID_N_ARG);
	}

	_upload_store = upload_directive.parameter(0);
	_logger << async::info << "upload store path set to " << _upload_store;
}

void WebServer::parseTimeout(const ConfigContext &root_context)
{
	const char *dir_name = "timeout";

	if (root_context.countDirectivesByName(dir_name) != 1)
	{
		_logger << async::error << root_context.name() << " should have 1 "
				<< dir_name;
		root_context.throwException(PARSINGEXC_INVALID_N_DIR);
	}

	const ConfigDirective &timeout_directive
		= root_context.getNthDirectiveByName(dir_name, 0);

	if (timeout_directive.is_context())
	{
		_logger << async::error << dir_name << " should not be context";
		root_context.throwException(PARSINGEXC_UNDEF_DIR);
	}
	if (timeout_directive.nParameters() != 1)
	{
		_logger << async::error << dir_name << " should have 1 parameter(s)";
		timeout_directive.throwException(PARSINGEXC_INVALID_N_ARG);
	}

	_timeout_ms = toNum<unsigned int>(timeout_directive.parameter(0));
	_logger << async::info << "global timeout is " << _timeout_ms;
}

void WebServer::parseServer(const ConfigContext &server_context)
{
	HTTP::Server server(server_context, _max_body_size, _timeout_ms);
	int port = server.getPort();
	_logger << async::info << "Created Server at port " << port;
	if (_tcp_procs.find(port) == _tcp_procs.end())
	{
		_tcp_procs[port] = async::TCPIOProcessor(port);
		_tcp_procs[port].initialize();
		_logger << async::verbose << "Created TCP IO Processor at port "
				<< port;
		_servers[port] = _Servers();
		_request_buffer[port] = _ReqBufFdMap();
	}
	_servers[port].push_back(server);
}

WebServer::WebServer(const ConfigContext &root_context)
	: _logger(async::Logger::getLogger("WebServer"))
{
	parseMaxBodySize(root_context);
	parseUploadStore(root_context);
	parseTimeout(root_context);

	const char *dir_name = "server";
	size_t n_servers = root_context.countDirectivesByName(dir_name);
	if (n_servers < 1)
	{
		_logger << async::error << root_context.name()
				<< " should have 1 or more " << dir_name;
		root_context.throwException(PARSINGEXC_INVALID_N_DIR);
	}
	for (size_t i = 0; i < n_servers; i++)
	{
		parseServer((const ConfigContext &)(root_context.getNthDirectiveByName(
			dir_name, i)));
	}
}

WebServer::~WebServer()
{
	for (_TCPProcMap::iterator it = _tcp_procs.begin(); it != _tcp_procs.end();
		 it++)
		it->second.finalize(NULL);
}

WebServer::WebServer(const WebServer &orig)
	: _tcp_procs(orig._tcp_procs), _servers(orig._servers),
	  _timeout_ms(orig._timeout_ms), _logger(orig._logger)
{
}

WebServer &WebServer::operator=(const WebServer &orig)
{
	_tcp_procs = orig._tcp_procs;
	_servers = orig._servers;
	_timeout_ms = orig._timeout_ms;
	return (*this);
}

void WebServer::parseRequestForEachFd(int port, async::TCPIOProcessor &tcp_proc)
{
	for (async::TCPIOProcessor::iterator it = tcp_proc.begin();
		 it != tcp_proc.end(); it++)
	{
		int client_fd = *it;
		if (tcp_proc.rdbuf(client_fd).empty())
			continue;

		if (_request_buffer[port].find(client_fd)
			== _request_buffer[port].end())
			_request_buffer[port][client_fd] = HTTP::Request();

		int rc;
		try
		{
			rc = _request_buffer[port][client_fd].parse(
				tcp_proc.rdbuf(client_fd), _max_body_size);
		}
		catch (const HTTP::Request::ParsingFail &e)
		{
			// TODO: 오류 상황에 따라 에러 코드 세분화
			// TODO: 에러 코드에 따라 연결 끊을 수도 있게 처리
			_logger << "Parsing failure: " << e.what() << '\n';
			_request_buffer[port][client_fd] = HTTP::Request();
			HTTP::Response res = generateErrorResponse(400); // Bad Request
			_tcp_procs[port].wrbuf(client_fd) += res.toString();
			_logger << async::debug << "Added to wrbuf: \"" << res.toString()
					<< "\"";
			continue;
		}

		switch (rc)
		{
		case HTTP::Request::RETURN_TYPE_OK:
			registerRequest(port, client_fd, _request_buffer[port][client_fd]);
			_request_buffer[port][client_fd] = HTTP::Request();
			break;

		case HTTP::Request::RETURN_TYPE_AGAIN:
			// let it run again at next call
			break;

		default:
			// handle error
			break;
		}
	}
}

WebServer::_Servers::iterator WebServer::findNoneNameServer(int port)
{
	for (_Servers::iterator it = _servers[port].begin();
		 it != _servers[port].end(); it++)
	{
		if (it->hasServerName() == false)
			return (it);
	}
	return (_servers[port].end());
}

void WebServer::registerRequest(int port, int client_fd, HTTP::Request &request)
{
	for (_Servers::iterator it = _servers[port].begin();
		 it != _servers[port].end(); it++)
	{
		if (it->isForMe(request))
		{
			it->registerRequest(client_fd, request);
			return;
		}
	}
	_logger << async::warning << "No matching server for "
			<< request.getHeaderValue("Host", 0);
	// 일치하는 Host가 없을 시, 해당 포트의 server_name이 없는 서버를 찾아보고
	// 그러한 서버가 없다면 해당 포트의 첫 서버에 등록
	if (findNoneNameServer(port) != _servers[port].end())
		findNoneNameServer(port)->registerRequest(client_fd, request);
	else
		_servers[port].front().registerRequest(client_fd, request);
}

void WebServer::retrieveResponseForEachFd(int port, _Servers &servers)
{
	for (_Servers::iterator server_it = servers.begin();
		 server_it != servers.end(); server_it++)
	{
		server_it->task();
		while (true)
		{
			int client_fd = server_it->hasResponses();
			if (client_fd < 0)
				break;
			_logger << async::verbose << "Response for client " << client_fd
					<< " has been found";
			HTTP::Response res = server_it->retrieveResponse(client_fd);
			_tcp_procs[port].wrbuf(client_fd) += res.toString();
			_logger << async::debug << "Added to wrbuf: \"" << res.toString()
					<< "\"";
		}
	}
}

HTTP::Response WebServer::generateErrorResponse(const int code)
{
	HTTP::Response response;

	std::string body = HTTP::generateErrorPage(code);
	response.setStatus(code);
	response.setBody(body);
	response.setContentLength(body.length());
	response.setContentType("text/html");
	return (response);
}

void WebServer::disconnect(int port, int client_fd)
{
	_request_buffer[port].erase(client_fd);
	for (_Servers::iterator it = _servers[port].begin();
		 it != _servers[port].end(); it++)
	{
		try
		{
			it->disconnect(client_fd);
		}
		catch (const HTTP::Server::ClientNotFound &e)
		{
			(void)e;
		}
	}
	_logger << async::info << "Disconnected client fd " << client_fd
			<< " from port " << port;
}

void WebServer::task(void)
{
	async::IOTaskHandler::task();
	for (_TCPProcMap::iterator it = _tcp_procs.begin(); it != _tcp_procs.end();
		 it++)
	{
		int port = it->first;
		async::TCPIOProcessor &tcp = it->second;

		while (!tcp.disconnected_clients.empty())
		{
			int disconnected_fd = tcp.disconnected_clients.front();
			tcp.disconnected_clients.pop();
			disconnect(port, disconnected_fd);
		}
		parseRequestForEachFd(port, tcp);
	}
	for (_ServerMap::iterator it = _servers.begin(); it != _servers.end(); it++)
		retrieveResponseForEachFd(it->first, it->second);
}
