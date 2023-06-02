#include "HTTP/ParsingFail.hpp"
#include "HTTP/Request.hpp"
#include "HTTP/const_values.hpp"
#include "utils/string.hpp"

using namespace HTTP;

int Request::parseStartLine(std::string &buffer)
{
	int rc = consumeStartLine(buffer);
	ASYNC_LOG_DEBUG(_logger, "Got return code " << rc);

	switch (rc)
	{
	case RETURN_TYPE_OK:
		_current_state = PARSE_STATE_HEADER;
		return (RETURN_TYPE_IN_PROCESS);

	case RETURN_TYPE_AGAIN:
		return (RETURN_TYPE_AGAIN);

	default:
		return (RETURN_TYPE_IN_PROCESS);
	}
}

int Request::parseHeader(std::string &buffer)
{
	int rc = consumeHeader(buffer);
	ASYNC_LOG_DEBUG(_logger, "Got return code " << rc);

	switch (rc)
	{
	case RETURN_TYPE_OK:
		parseHeaderEnsureHostHeaderField();
		parseHeaderEnsureTrailerHeaderField();
		parseHeaderEnsureCorrectHeadersForPostPutMethod();
		if (_header.hasValue("Transfer-Encoding", "chunked"))
		{
			_logger << async::verbose << "Transfer-Encoding is chunked";
			parseHeaderHandleTransferEncodingChunked();
		}
		else if (_header.hasValue("Content-Length"))
		{
			_logger << async::verbose << "header has Content-Length";
			parseHeaderHandlerContentLength();
		}
		else
			return (RETURN_TYPE_OK);
		break;

	case RETURN_TYPE_AGAIN:
		return (RETURN_TYPE_AGAIN);
	}
	ASYNC_LOG_DEBUG(_logger, "parsing in progress...");
	return (RETURN_TYPE_IN_PROCESS);
}

int Request::parseBody(std::string &buffer)
{
	int rc = consumeBody(buffer);
	ASYNC_LOG_DEBUG(_logger, "Got return code " << rc);
	return (rc);
}

int Request::parseChunk(std::string &buffer)
{
	int rc = consumeChunk(buffer);
	ASYNC_LOG_DEBUG(_logger, "Got return code " << rc);
	ASYNC_LOG_DEBUG(_logger, "total content length " << _content_length
													 << "client max body size "
													 << _max_body_size);
	if (_content_length > _max_body_size)
		throw(HTTP::InvalidSize());

	switch (rc)
	{
	case RETURN_TYPE_OK:
		if (_header.hasValue("Trailer"))
		{
			_logger << async::verbose << "header has Trailer";
			_current_state = PARSE_STATE_TRAILER;
			return (RETURN_TYPE_IN_PROCESS);
		}
		else
			return (RETURN_TYPE_OK);

	case RETURN_TYPE_AGAIN:
		return (RETURN_TYPE_AGAIN);
	}

	return (RETURN_TYPE_IN_PROCESS);
}

int Request::parseTrailer(std::string &buffer)
{
	int rc = consumeTrailer(buffer);
	ASYNC_LOG_DEBUG(_logger, "Got return code " << rc);
	return (rc);
}

int Request::parse(std::string &buffer)
{
	while (true)
	{
		int rc;
		ASYNC_LOG_DEBUG(_logger,
						__func__ << ": current state is " << _current_state);
		switch (_current_state)
		{
		case PARSE_STATE_STARTLINE:
			rc = parseStartLine(buffer);
			if (rc != RETURN_TYPE_IN_PROCESS)
				return (rc);
			break;

		case PARSE_STATE_HEADER:
			rc = parseHeader(buffer);
			if (rc != RETURN_TYPE_IN_PROCESS)
				return (rc);
			break;

		case PARSE_STATE_BODY:
			rc = parseBody(buffer);
			if (rc != RETURN_TYPE_IN_PROCESS)
				return (rc);
			break;

		case PARSE_STATE_CHUNK:
			rc = parseChunk(buffer);
			if (rc != RETURN_TYPE_IN_PROCESS)
				return (rc);
			break;

		case PARSE_STATE_TRAILER:
			rc = parseTrailer(buffer);
			if (rc != RETURN_TYPE_IN_PROCESS)
				return (rc);
			break;
		}
	}
}
