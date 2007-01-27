#ifndef STD_REGEXP_HPP__
#define STD_REGEXP_HPP__

#include <string>
#include <stdexcept>
#include <vector>

#include "regexp.h"

class regular_expression_error : public std::runtime_error
{
public:
	regular_expression_error(int error_code, regexp* re);
	~regular_expression_error() throw() {};

	int code() const;
	const char* message() const;

private:
	int code_;
	std::string message_;
};

class regular_expression
{
public:
#ifdef REGEXP_UNICODE
	typedef wchar_t CharT;
	typedef std::wstring string_type;
#else
	typedef char CharT;
	typedef std::string string_type;
#endif

	typedef string_type::size_type size_type;
	typedef string_type::const_iterator const_iterator;

	regular_expression(const CharT* pattern);
	regular_expression(const string_type& pattern);
	~regular_expression();

	bool exec(const CharT* match);
	bool exec(const string_type& match);

	bool matched(size_type sub_exp = 0) const;
	const_iterator begin(size_type sub_exp = 0) const;
	const_iterator end(size_type sub_exp = 0) const;
	string_type operator[] (size_type sub_exp) const;

	size_type size() const;

private:
	class regexp_wrapper
	{
	public:
		regexp_wrapper();
		~regexp_wrapper();

		regexp* get();
		regexp** operator&();

	private:
		regexp* wrapped_;

		regexp_wrapper(const regexp_wrapper&);
		regexp_wrapper& operator=(const regexp_wrapper&);
	};
	
	regexp_wrapper compiled_;
	string_type match_;
	std::vector<regmatch> matches_;

	// internal utilities
	void common_init(const CharT* pattern);
	void reset();
	
	// disable copying
	regular_expression(const regular_expression&);
	regular_expression& operator=(const regular_expression&);
};

#endif /* STD_REGEXP_HPP__ */

