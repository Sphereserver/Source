#ifndef STRINGS_H_
#define STRINGS_H_

// Base abstract class for strings, provides basic information of what should be available
class IString
{
public:
	IString();
	~IString();

	// information
	int length();

//	// modification
//	void append(const char *s);
//	void replace(char what, char toWhat);
//
//	// comparison
//	int compareTo(const char *s);
//	int compareToIgnoreCase(const char *s);
//	int equals(const char *s);
//	int equalsIgnoreCase(const char *s);
//
//	// search
//	int indexOf(char c);
//	int indexOf(char *s);
//	int lastIndexOf(char c);
//	int lastIndexOf(char *s);

protected:
	virtual void ensureLength(int newLength);
	virtual void destroy();

protected:
	char	*m_buf;
	int		m_length;
	int		m_realLength;
};

// Common string implementation, implementing IString and working on heap
class String : public IString
{
public:
	String();

protected:
	virtual void ensureLength(int newLength);
	virtual void destroy();
};

// Temporary string implementation. Works with thread-safe string
// There are 2 ways to create such string:
// 1. TemporaryString str;
// 2. String str = some_thread_variable->allocateString();
class TemporaryString : public String
{
public:
	TemporaryString();
	TemporaryString(char *buffer, char *state);

	// should not really be used, made for use of AbstractSphereThread *only*
	void init(char *buffer, char *state);

protected:
	virtual void ensureLength(int newLength);
	virtual void destroy();

private:
	bool	m_useHeap;					// a mark whatever we are in heap (String) or stack (ThreadLocal) mode
	char	*m_state;					// a pointer to thread local state of the line we occupy
};

#endif
