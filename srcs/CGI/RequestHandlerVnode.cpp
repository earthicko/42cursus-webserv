#include "CGI/RequestHandler.hpp"
#include "utils/file.hpp"
#include "utils/hash.hpp"
#include "utils/string.hpp"
#include <cstdio>
#include <cstdlib>
#include <sys/wait.h>
#include <unistd.h>

using namespace CGI;

RequestHandlerVnode::RequestHandlerVnode(const Request &request,
										 const std::string &exec_path,
										 const unsigned int timeout_ms,
										 const std::string &temp_dir_path)
	: RequestHandler(request, exec_path, timeout_ms)
	, _input_file_path(temp_dir_path + "/"
					   + generateHash(toStr(clock()) + "input"))
	, _output_file_path(temp_dir_path + "/"
						+ generateHash(toStr(clock()) + "output"))
	, _timeout_ms(timeout_ms)
{
	_writer = new async::FileWriter(
		_timeout_ms, _input_file_path, _request.getMessageBody());
	LOG_VERBOSE("CGI input file: " << _input_file_path);
	LOG_VERBOSE("CGI output file: " << _output_file_path);
}

RequestHandlerVnode::~RequestHandlerVnode()
{
	if (std::remove(_input_file_path.c_str()) == -1)
		LOG_WARNING("Failed to delete file: " << _input_file_path);
	if (std::remove(_output_file_path.c_str()) == -1)
		LOG_WARNING("Failed to delete file: " << _output_file_path);
	delete _writer;
	delete _reader;
}

int RequestHandlerVnode::waitWriteInputOperation(void)
{
	switch (_writer->task())
	{
	case async::status::OK_DONE:
		LOG_DEBUG("Finished writing CGI Request body to file");
		delete _writer;
		_writer = NULL;
		_status = CGI_RESPONSE_INNER_STATUS_FORK_AGAIN;
		return (CGI_RESPONSE_STATUS_AGAIN);
	case async::status::OK_AGAIN:
		LOG_DEBUG("writing CGI Request body");
		return (CGI_RESPONSE_STATUS_AGAIN);
	case async::status::ERROR_TIMEOUT:
		throw(std::runtime_error("Timeout occured while writing CGI Request"));
	default:
		throw(std::runtime_error("failed to write CGI Request"));
	}
	return (CGI_RESPONSE_STATUS_OK);
}

int RequestHandlerVnode::fork()
{
	_pid = ::fork();
	if (_pid < 0)
	{
		throw(std::runtime_error("failed to fork"));
	}
	else if (_pid == 0)
	{
		FILE *input_stream = fopen(_input_file_path.c_str(), "r");
		FILE *output_stream = fopen(_output_file_path.c_str(), "w");
		if (!input_stream || !output_stream)
		{
			LOG_ERROR("Failed to create temporary files");
			async::Logger::blockingWriteAll();
			std::exit(2);
		}
		int input_fd = fileno(input_stream);
		int output_fd = fileno(output_stream);
		if (::dup2(input_fd, STDIN_FILENO) < 0
			|| ::dup2(output_fd, STDOUT_FILENO) < 0)
		{
			LOG_ERROR("failed to dup2");
			async::Logger::blockingWriteAll();
			std::exit(2);
		}
		fclose(input_stream);
		fclose(output_stream);
		LOG_DEBUG("calling execve(" << _exec_path.c_str() << "), good bye!");
		execve(_exec_path.c_str(), getArgv(), _request.getEnv());
		std::exit(2);
	}
	else
	{
		LOG_DEBUG("successed to fork.");
		setTimeout();
		_status = CGI_RESPONSE_INNER_STATUS_WAITPID_AGAIN;
		return (CGI_RESPONSE_STATUS_AGAIN);
	}
	return (CGI_RESPONSE_STATUS_OK);
}

int RequestHandlerVnode::waitExecution()
{
	pid_t pid = ::waitpid(_pid, &_waitpid_status, WNOHANG);

	if (pid < 0)
	{
		throw(std::runtime_error("failed to waitpid"));
	}
	else if (pid == 0)
	{
		LOG_DEBUG("waiting child");
		checkTimeout();
		return (CGI_RESPONSE_STATUS_AGAIN);
	}
	else
	{
		if ((WIFEXITED(_waitpid_status) && (WEXITSTATUS(_waitpid_status) == 2))
			|| WIFSIGNALED(_waitpid_status))
			throw(std::runtime_error("CGI execution failed"));
		LOG_DEBUG("child process done");
		LOG_DEBUG("successed CGI execution");
		_status = CGI_RESPONSE_INNER_STATUS_READ_AGAIN;
		return (CGI_RESPONSE_STATUS_AGAIN);
	}
}

int RequestHandlerVnode::waitReadOutputOperation(void)
{
	if (!_reader)
		_reader = new async::FileReader(_timeout_ms, _output_file_path);
	switch (_reader->task())
	{
	case async::status::OK_DONE: {
		LOG_DEBUG("read status is ok");
		LOG_DEBUG("buffer: " << _reader->retrieve());
		std::string cgi_output = _reader->retrieve();
		_response.makeResponse(cgi_output);
		delete _reader;
		_reader = NULL;
		_status = CGI_RESPONSE_INNER_STATUS_OK;
		return (CGI_RESPONSE_STATUS_OK);
	}
	case async::status::OK_AGAIN: {
		LOG_DEBUG("read status is again");
		LOG_DEBUG("read from file " << _output_file_path);
		return (CGI_RESPONSE_STATUS_AGAIN);
	}
	case async::status::ERROR_TIMEOUT:
		throw(std::runtime_error(
			"Timeout occured while reading from CGI Request"));
	default:
		throw(std::runtime_error("failed to receive CGI response"));
	}
	return (CGI_RESPONSE_STATUS_OK);
}

int RequestHandlerVnode::task(void)
{
	LOG_DEBUG("status : " << _status);
	switch (_status)
	{
	case CGI_RESPONSE_INNER_STATUS_BEGIN:
		return (waitWriteInputOperation());
	case CGI_RESPONSE_INNER_STATUS_FORK_AGAIN:
		return (fork());
	case CGI_RESPONSE_INNER_STATUS_WAITPID_AGAIN:
		return (waitExecution());
	case CGI_RESPONSE_INNER_STATUS_READ_AGAIN:
		return (waitReadOutputOperation());
	default:
		return (CGI_RESPONSE_STATUS_OK);
	}
}
