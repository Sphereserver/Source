#include "stdregexp.hpp"
#include <algorithm>

#ifdef REGEXP_UNICODE 

#define re_comp_t re_comp_w
#define re_exec_t re_exec_w

#else

#define re_comp_t re_comp
#define re_exec_t re_exec

#endif

////////////////////////////////////////////////////////////////
// regular_expression_error
//

regular_expression_error::regular_expression_error(int error_code, regexp* re) :
    std::runtime_error("regular_expression_error"),
	code_(error_code)
{
	std::vector<char> buffer(128);
	re_error(code_, re, &buffer[0], buffer.size());
	message_.assign(&buffer[0]);
}

int regular_expression_error::code() const
{
	return code_;
}

const char* regular_expression_error::message() const
{
	return message_.c_str();
}

////////////////////////////////////////////////////////////////
// regular_expression
//

// regular_expression::regexp_wrapper
regular_expression::regexp_wrapper::regexp_wrapper() : wrapped_(NULL)
{}

regular_expression::regexp_wrapper::~regexp_wrapper()
{
	if(wrapped_)
		re_free(wrapped_);
}

regexp* regular_expression::regexp_wrapper::get()
{
	return wrapped_;
}

regexp** regular_expression::regexp_wrapper::operator& ()
{
	return &wrapped_;
}

// internal utility
namespace
{
	void convert_error(int error_code, regexp* re)
	{
		if(error_code < 0)
			throw regular_expression_error(error_code, re);
	}
}

regular_expression::regular_expression(const CharT* pattern)
{
	common_init(pattern);
}

regular_expression::regular_expression(const string_type& pattern)
{
	common_init(pattern.c_str());
}

void regular_expression::common_init(const CharT* pattern)
{
	int error_code = re_comp_t(&compiled_, pattern);
	convert_error(error_code, compiled_.get());

	error_code = re_nsubexp(compiled_.get());
	convert_error(error_code, compiled_.get());

	matches_.resize(error_code);
	reset();
}

void regular_expression::reset()
{
	regmatch dummy;
	dummy.begin = dummy.end = -1;

	std::fill(matches_.begin(), matches_.end(), dummy);
}

regular_expression::~regular_expression()
{
	// handled by regex_wrapper
}

bool regular_expression::exec(const CharT* match)
{
	return exec(string_type(match));
}

bool regular_expression::exec(const string_type& match)
{
	match_ = match;
	reset();
	int error_code = re_exec_t(compiled_.get(),
							   match_.c_str(),
							   matches_.size(),
							   &matches_[0]);
	convert_error(error_code, compiled_.get());
	return error_code > 0;
}

bool regular_expression::matched(size_type sub_exp) const
{
	return (sub_exp < matches_.size() &&
			matches_[sub_exp].begin != -1);
}

regular_expression::const_iterator
regular_expression::begin(size_type sub_exp) const
{
	return (sub_exp >= matches_.size() || matches_[sub_exp].begin == -1) ?
		match_.end() :
		(match_.begin() + matches_[sub_exp].begin);
}

regular_expression::const_iterator
regular_expression::end(size_type sub_exp) const
{
	return (sub_exp >= matches_.size() || matches_[sub_exp].end == -1) ?
		match_.end() :
		(match_.begin() + matches_[sub_exp].end);
}

regular_expression::string_type
regular_expression::operator[] (size_type sub_exp) const
{
	return string_type(begin(sub_exp), end(sub_exp));
}

regular_expression::size_type regular_expression::size() const
{
	return matches_.size();
}

