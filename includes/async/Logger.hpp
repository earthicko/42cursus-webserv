#ifndef ASYNC_LOGGER_HPP
#define ASYNC_LOGGER_HPP

#include "async/SingleIOProcessor.hpp"
#include "utils/shared_ptr.hpp"
#include <map>
#include <sstream>
#include <string>
#include <vector>

namespace async
{
class Logger
{
  private:
	typedef ft::shared_ptr<SingleIOProcessor> _SingleIOPtr;
	typedef std::map<int, _SingleIOPtr> _Procs;

	static std::map<std::string, ft::shared_ptr<Logger> > _loggers;
	static _Procs _target;
	static const std::string _name_default;
	static int _log_level;
	static const char *_level_names[];
	static const char *_level_prefixes[];
	static bool _active;

	const std::string _name;

	Logger(void);
	Logger(const std::string &name);
	Logger(const Logger &orig);
	Logger &operator=(const Logger &orig);

  public:
	enum debug_level_e
	{
		DEBUG = 0,
		VERBOSE,
		INFO,
		WARNING,
		ERROR
	};
	class EndMarker
	{
	  public:
		int level;
		EndMarker(int _level = INFO);
		EndMarker(const EndMarker &orig);
		EndMarker &operator=(const EndMarker &orig);
		~EndMarker();
	};

	~Logger();
	std::string getPrefix(int level);
	void log(const std::string &content);
	static bool isActive(void);
	static void activate(void);
	static void deactivate(void);
	static void registerFd(int fd);
	static void setLogLevel(int log_level);
	static void setLogLevel(const std::string &log_level);
	static int getLogLevel(void);
	static Logger &getLogger(const std::string &name);
	static void doAllTasks(void);
	static void blockingWriteAll(void);
};

extern const Logger::EndMarker debug;
extern const Logger::EndMarker verbose;
extern const Logger::EndMarker info;
extern const Logger::EndMarker warning;
extern const Logger::EndMarker error;
} // namespace async

async::Logger &operator<<(async::Logger &io,
						  const async::Logger::EndMarker mark);

template <typename T>
inline async::Logger &operator<<(async::Logger &io, T content)
{
	if (!io.isActive())
		return (io);
	std::stringstream buf;
	buf << content;
	io.log(buf.str());
	return (io);
}

// _logger라는 이름의 로거 객체가 있을때 사용 가능
#define LOG_DEBUG(message)                                                     \
	do                                                                         \
	{                                                                          \
		if (async::Logger::DEBUG >= async::Logger::getLogLevel())              \
			_logger << async::debug << message;                                \
	} while (0)

// _logger라는 이름의 로거 객체가 있을때 사용 가능
#define LOG_VERBOSE(message)                                                   \
	do                                                                         \
	{                                                                          \
		if (async::Logger::VERBOSE >= async::Logger::getLogLevel())            \
			_logger << async::verbose << message;                              \
	} while (0)

// _logger라는 이름의 로거 객체가 있을때 사용 가능
#define LOG_INFO(message)                                                      \
	do                                                                         \
	{                                                                          \
		if (async::Logger::INFO >= async::Logger::getLogLevel())               \
			_logger << async::info << message;                                 \
	} while (0)

// _logger라는 이름의 로거 객체가 있을때 사용 가능
#define LOG_WARNING(message)                                                   \
	do                                                                         \
	{                                                                          \
		if (async::Logger::WARNING >= async::Logger::getLogLevel())            \
			_logger << async::warning << message;                              \
	} while (0)

// _logger라는 이름의 로거 객체가 있을때 사용 가능
#define LOG_ERROR(message)                                                     \
	do                                                                         \
	{                                                                          \
		if (async::Logger::ERROR >= async::Logger::getLogLevel())              \
			_logger << async::error << message;                                \
	} while (0)

#endif
