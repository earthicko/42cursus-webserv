#include "Header.hpp"

Header::Header()
{
}

Header::~Header()
{
}

Header::Header(const Header &orig)
	: _values(orig._values)
{
}

Header &Header::operator=(const Header &orig)
{
	_values = orig._values;
	return (*this);
}

bool Header::hasValue(std::string const &name) const
{
	Header::const_iterator name_iter = _values.find(name);
	if (name_iter != _values.end())
		return (true);
	return (false);
}

bool Header::hasValue(const std::string &name, const std::string &value) const
{
	Header::const_iterator name_iter = _values.find(name);
	if (name_iter == _values.end())
		return (false);

	const std::vector<std::string> &values = name_iter->second;
	for (size_t i = 0; i < values.size(); i++)
	{
		if (values[i] == value)
			return (true);
	}
	return (false);
}

bool Header::hasValue(const Header::const_iterator &iter,
					  const std::string &value) const
{
	const std::vector<std::string> &values = iter->second;
	for (size_t i = 0; i < values.size(); i++)
	{
		if (values[i] == value)
			return (true);
	}
	return (false);
}

size_t Header::countValue(const std::string &name) const
{
	if (!hasValue(name))
		return (0);
	return (_values.find(name)->second.size());
}

const std::string &Header::getValue(const std::string &name, int idx) const
{
	Header::const_iterator name_iter = _values.find(name);
	if (name_iter == _values.end())
		throw(std::runtime_error("Header name " + name + " not found."));

	return (name_iter->second[idx]);
}

const std::vector<std::string> &Header::getValues(const std::string &name) const
{
	Header::const_iterator name_iter = _values.find(name);
	if (name_iter == _values.end())
		throw(std::runtime_error("Header name " + name + " not found."));
	return (name_iter->second);
}

void Header::assign(const std::string &name,
					const std::vector<std::string> &values)
{
	_values[name] = values;
}

void Header::insert(const std::string &name,
					const std::vector<std::string> &values)
{
	_values[name].insert(_values[name].end(), values.begin(), values.end());
}

void Header::insert(const std::string &name, const std::string &value)
{
	_values[name].insert(_values[name].end(), value);
}

void Header::append(const std::string &name,
					const std::vector<std::string> &values)
{
	if (!hasValue(name))
		assign(name, values);
	else
		insert(name, values);
}

Header::iterator Header::begin(void)
{
	return (_values.begin());
}

Header::iterator Header::end(void)
{
	return (_values.end());
}

Header::const_iterator Header::begin(void) const
{
	return (_values.begin());
}

Header::const_iterator Header::end(void) const
{
	return (_values.end());
}
