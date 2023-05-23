#include "../const_values.hpp"
#include "../error_pages.hpp"
#include "ConfigDirective.hpp"
#include "HTTP/Server.hpp"
#include "utils/string.hpp"
#include <cctype>

using namespace HTTP;

int Server::getPort(void) const
{
	return (_port);
}

std::string Server::getErrorPage(const int code)
{
	if (_error_pages.find(code) == _error_pages.end())
		return (generateErrorPage(code));
	int rc = _error_pages[code]->task();
	if (rc == async::status::OK)
		return (_error_pages[code]->retrieve());
	else if (rc == async::status::AGAIN)
		return (generateErrorPage(code));
	// TODO: 예외 처리
}

Response Server::generateErrorResponse(const int code)
{
	Response response;

	std::string body = getErrorPage(code);
	response.setStatus(code);
	response.setBody(body);
	response.setContentLength(body.length());
	response.setContentType("text/html");
	return (response);
}

void Server::registerErrorResponse(const int fd, const int code)
{
	_output_queue[fd].push(generateErrorResponse(code));
}

void Server::ensureClientConnected(int client_fd)
{
	if (_output_queue.find(client_fd) == _output_queue.end())
	{
		throw(std::runtime_error("Client fd " + toStr(client_fd)
								 + " is not yet connected."));
	}
}
