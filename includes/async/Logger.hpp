#ifndef ASYNCLOGGER_HPP
#define ASYNCLOGGER_HPP

#include "async//SingleIOProcessor.hpp"
#include <map>
#include <sstream>
#include <string>
#include <vector>

namespace async
{
class Logger
{
  private:
	typedef std::map<int, SingleIOProcessor *> _Procs;

	static std::map<std::string, Logger *> _loggers;
	static _Procs _target;
	static const std::string _name_default;
	static int _log_level;
	static const char *_level_names[];

	const std::string _name;
	std::string _buf;

	Logger(void);
	Logger(const std::string &name);
	Logger(const Logger &orig);
	Logger &operator=(const Logger &orig);

	std::string getPrefix(int level);

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
	void registerLog(const std::string &content);
	void log(int level);
	static void registerFd(int fd);
	static void setLogLevel(int log_level);
	static void setLogLevel(const std::string &log_level);
	static Logger &getLogger(const std::string &name);
	static void task(void);
	static void blockingWrite(void);
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
	std::stringstream buf;
	buf << content;
	io.registerLog(buf.str());
	return (io);
}

#endif