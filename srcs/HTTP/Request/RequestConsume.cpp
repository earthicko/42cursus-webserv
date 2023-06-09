#include "HTTP/ParsingFail.hpp"
#include "HTTP/Request.hpp"
#include "HTTP/const_values.hpp"
#include "utils/string.hpp"
#include <cstdlib>

using namespace HTTP;

static int parseHTTPVersion(const std::string &http_version_str)
{
	static const std::string http_prefix = "HTTP/";

	const size_t version_position = http_version_str.find(http_prefix);
	if (version_position != 0)
		throw(HTTP::InvalidValue());

	const std::string version_str
		= getbackstr(http_version_str, version_position + http_prefix.size());

	const size_t dot_position = version_str.find(".");
	if (dot_position == std::string::npos)
		throw(HTTP::InvalidValue());

	const std::string major_str = getfrontstr(version_str, dot_position);
	const std::string minor_str = getbackstr(version_str, dot_position + 1);
	const int major = toNum<int>(major_str);
	const int minor = toNum<int>(minor_str);

	return (major * 1000 + minor);
}

int Request::consumeLine(std::string &buffer,
						 std::string &line,
						 size_t &crlf_pos)
{
	crlf_pos = buffer.find(CRLF);
	if (crlf_pos == std::string::npos)
	{
		LOG_DEBUG(__func__ << ": buffer doesn't have CRLF");
		return (RETURN_TYPE_AGAIN);
	}
	line = consumestr(buffer, crlf_pos);
	consumestr(buffer, CRLF_LEN);
	return (RETURN_TYPE_OK);
}

int Request::consumeStartLine(std::string &buffer)
{
	size_t crlf_pos;
	std::string start_line;

	if (consumeLine(buffer, start_line, crlf_pos) == RETURN_TYPE_AGAIN)
		return (RETURN_TYPE_AGAIN);

	LOG_DEBUG(__func__ << ": start line is \"" << start_line << "\"");

	if (crlf_pos == 0)
	{
		LOG_WARNING(__func__ << ": buffer's first line is empty");
		throw(HTTP::EmptyLineFound());
	}
	if (start_line[0] == ' ')
	{
		LOG_WARNING(__func__ << ": buffer's first character is space");
		throw(HTTP::InvalidFormat());
	}

	std::vector<std::string> tokens = split(start_line, ' ');
	if (tokens.size() != 3)
	{
		LOG_WARNING(__func__ << ": token count mismatch");
		throw(HTTP::InvalidFormat());
	}

	try
	{
		_method = METHOD[tokens[0]];
	}
	catch (const std::runtime_error &e)
	{
		LOG_WARNING(__func__ << ": invalid method");
		throw(HTTP::InvalidValue());
	}
	LOG_DEBUG(__func__ << ": method index is " << _method);

	_uri = tokens[1];
	size_t question_mark_pos = _uri.find('?');
	if (question_mark_pos != std::string::npos)
	{
		_query_string = getbackstr(_uri, question_mark_pos + 1);
		trimbackstr(_uri, question_mark_pos);
	}
	_version = tokens[2];

	try
	{
		_version_num = parseHTTPVersion(tokens[2]);
	}
	catch (const std::invalid_argument &e)
	{
		LOG_WARNING(__func__ << ": invalid HTTP version");
		throw(HTTP::InvalidValue());
	}

	LOG_VERBOSE(__func__ << ": URI: \"" << _uri << "\"");
	LOG_VERBOSE(__func__ << ": QUERY: \"" << _query_string << "\"");
	LOG_VERBOSE(__func__ << ": version: \"" << _version << "\" ("
						 << _version_num << ")");
	LOG_DEBUG(__func__ << ": buffer result in :\"" << buffer << "\"");
	return (RETURN_TYPE_OK);
}

void Request::consumeHeaderGetNameValue(std::string &header_line,
										std::string &name,
										std::vector<std::string> &values,
										bool is_trailer)
{
	const size_t colon_pos = header_line.find(':');
	if (colon_pos == std::string::npos)
	{
		LOG_WARNING(__func__ << ": header line has no colon");
		throw(HTTP::InvalidField());
	}

	name = getfrontstr(header_line, colon_pos);
	if (hasSpace(name))
	{
		LOG_WARNING(__func__ << ": header name has space");
		throw(HTTP::InvalidField());
	}

	if (is_trailer)
	{
		bool found_name = false;
		for (std::vector<std::string>::iterator it = _trailer_values.begin();
			 it != _trailer_values.end();
			 it++)
		{
			if (*it == name)
			{
				found_name = true;
				_trailer_values.erase(it);
				break;
			}
		}
		if (found_name == false)
		{
			LOG_WARNING(__func__ << ": Trailer header doesn't have " << name);
			throw(HTTP::InvalidField());
		}
	}

	const std::string value_part = getbackstr(header_line, colon_pos + 1);
	values = split(value_part, ",");
	for (size_t i = 0; i < values.size(); i++)
	{
		strtrim(values[i], LWS);
		LOG_DEBUG(__func__ << ": new value \"" << values[i] << "\"");
	}
}

int Request::consumeHeader(std::string &buffer)
{
	size_t crlf_pos;
	std::string header_line;

	if (consumeLine(buffer, header_line, crlf_pos) == RETURN_TYPE_AGAIN)
		return (RETURN_TYPE_AGAIN);

	LOG_DEBUG(__func__ << ": header line: " << header_line);
	LOG_DEBUG(__func__ << ": buffer result in :\"" << buffer << "\"");

	if (crlf_pos == 0) // CRLF만 있는 줄: 헤더의 끝을 의미
	{
		LOG_DEBUG(__func__ << ": header line only has CRLF (end of header)");
		LOG_DEBUG(__func__ << ": buffer result in :\"" << buffer << "\"");
		return (RETURN_TYPE_OK);
	}

	std::string name;
	std::vector<std::string> values;
	consumeHeaderGetNameValue(header_line, name, values, false);
	_header.append(name, values);

	return (RETURN_TYPE_IN_PROCESS);
}

int Request::consumeBody(std::string &buffer)
{
	if (buffer.size() < static_cast<size_t>(_content_length))
	{
		LOG_DEBUG(__func__ << ": not enough buffer size");
		return (RETURN_TYPE_AGAIN);
	}

	_body = consumestr(buffer, _content_length);
	LOG_DEBUG(__func__ << ": body result in :\"" << _body << "\"");
	LOG_DEBUG(__func__ << ": buffer result in :\"" << buffer << "\"");
	return (RETURN_TYPE_OK);
}

int Request::consumeChunk(std::string &buffer)
{
	const size_t crlf_pos = buffer.find(CRLF);
	if (crlf_pos == std::string::npos)
	{
		LOG_DEBUG(__func__ << ": buffer doesn't have CRLF");
		return (RETURN_TYPE_AGAIN);
	}

	size_t content_length;
	try
	{
		content_length = ::toHexNum<size_t>(buffer.substr(0, crlf_pos));
	}
	catch (const std::invalid_argument &e)
	{
		(void)e;
		throw(HTTP::InvalidValue());
	}

	LOG_DEBUG(__func__ << ": content length " << content_length);
	// buffer.size() >= 숫자가 적힌 줄 길이 + content_length + CRLF_LEN이면 ok
	if (buffer.size() < crlf_pos + CRLF_LEN + content_length + CRLF_LEN)
	{
		LOG_DEBUG(__func__ << ": not enough buffer size");
		return (RETURN_TYPE_AGAIN);
	}

	consumestr(buffer, crlf_pos + CRLF_LEN);
	_content_length += content_length;
	_body.append(consumestr(buffer, content_length));
	LOG_DEBUG(__func__ << ": body result in :\"" << _body << "\"");
	if (consumestr(buffer, CRLF_LEN) != CRLF)
	{
		LOG_WARNING(__func__ << ": chunk must end with CRLF");
		throw(HTTP::InvalidFormat());
	}
	LOG_DEBUG(__func__ << ": buffer result in :\"" << buffer << "\"");

	if (content_length == 0)
		return (RETURN_TYPE_OK);
	else
		return (RETURN_TYPE_IN_PROCESS);
}

int Request::consumeTrailer(std::string &buffer)
{
	size_t crlf_pos;
	std::string header_line;

	if (consumeLine(buffer, header_line, crlf_pos) == RETURN_TYPE_AGAIN)
		return (RETURN_TYPE_AGAIN);

	LOG_DEBUG(__func__ << ": header line: " << header_line);
	LOG_DEBUG(__func__ << ": buffer result in :\"" << buffer << "\"");

	if (crlf_pos == 0) // CRLF만 있는 줄: 헤더의 끝을 의미
	{
		LOG_DEBUG(__func__ << ": header line only has CRLF (end of header)");
		return (RETURN_TYPE_OK);
	}

	std::string name;
	std::vector<std::string> values;
	consumeHeaderGetNameValue(header_line, name, values, true);

	_header.append(name, values);

	if (_trailer_values.size() == 0)
		return (RETURN_TYPE_OK);
	else
		return (RETURN_TYPE_IN_PROCESS);
}
