#include "async/FileIOProcessor.hpp"

using namespace async;

FileWriter::FileWriter() : FileIOProcessor()
{
}

FileWriter::FileWriter(const FileWriter &orig) : FileIOProcessor(orig)
{
}

FileWriter &FileWriter::operator=(const FileWriter &orig)
{
	FileIOProcessor::operator=(orig);
	return (*this);
}

FileWriter::FileWriter(unsigned int timeout_ms, int fd,
					   const std::string &content)
	: FileIOProcessor(timeout_ms, fd)
{
	_writer.setWriteBuf(content);
}

FileWriter::FileWriter(unsigned int timeout_ms, const std::string &path,
					   const std::string &content)
	: FileIOProcessor(timeout_ms, path)
{
	_writer.setWriteBuf(content);
}

FileWriter::~FileWriter()
{
}

int FileWriter::task(void)
{
	if (_status == LOAD_STATUS_OK)
		return (LOAD_STATUS_OK);
	checkTimeout();
	_writer.task();
	if (_writer.writeDone())
	{
		_status = LOAD_STATUS_OK;
		return (LOAD_STATUS_OK);
	}
	return (LOAD_STATUS_AGAIN);
}