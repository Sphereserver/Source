#ifndef STRINGS_H_
#define STRINGS_H_

// Base abstract class for strings, provides basic information of what should be available
// NOTE: The destructor is NOT virtual for a reason. Children should override destroy() instead
class AbstractString
{
public:
	AbstractString();
	virtual ~AbstractString();

private:
	AbstractString(const AbstractString& copy);
	AbstractString& operator=(const AbstractString& other);

public:
	// information
	int length();
	int realLength();
	bool isEmpty();
	const char *toBuffer();

	// character operations
	char charAt(int index);
	void setAt(int index, char c);

	// modification
	void append(const char *s);
	void replace(char what, char toWhat);

	// comparison
	int compareTo(const char *s);			// compare with
	int compareToIgnoreCase(const char *s);	// compare with [case ignored]
	bool equals(const char *s);				// equals?
	bool equalsIgnoreCase(const char *s);	// equals? [case ignored]
	bool startsWith(const char *s);			// starts with this? [case ignored]
	bool startsWithHead(const char *s);		// starts with this and separator [case ignored]

	// search
	int indexOf(char c);
	int indexOf(const char *s);
	int lastIndexOf(char c);

	// operator
	operator LPCTSTR() const;       // as a C string
	operator char*();				// as a C string
	operator const char*&() const;	// as a C string

protected:
	// not implemented, should take care that newLength should fit in the buffer
	virtual void ensureLength(int newLength);
	// not implemented, should free up occupied resources
	virtual void destroy();

protected:
	char	*m_buf;
	int		m_length;
	int		m_realLength;
};

// Common string implementation, implementing AbstractString and working on heap
class String : public AbstractString
{
public:
	String();
	virtual ~String() { };

private:
	String(const String& copy);
	String& operator=(const String& other);

protected:
	void ensureLength(int newLength);
	void destroy();
};

#define MAX_TEMP_LINES_NO_CONTEXT	512

// Temporary string implementation. Works with thread-safe string
// To create such string:
// TemporaryString str;
// it could be also created via new TemporaryString but whats the point if we still use memory allocation? :)
class TemporaryString : public String
{
public:
	TemporaryString();
	TemporaryString(char *buffer, char *state);
	~TemporaryString();

private:
	TemporaryString(const TemporaryString& copy);
	TemporaryString& operator=(const TemporaryString& other);

public:
	// should not really be used, made for use of AbstractSphereThread *only*
	void init(char *buffer, char *state);

protected:
	void ensureLength(int newLength);
	void destroy();

private:
	bool m_useHeap;					// a mark whatever we are in heap (String) or stack (ThreadLocal) mode
	char *m_state;					// a pointer to thread local state of the line we occupy

	// static buffer to allow similar operations for non-threaded environment
	// NOTE: this buffer have no protection against overrun, so beware
	static int m_tempPosition;
	static char m_tempStrings[MAX_TEMP_LINES_NO_CONTEXT][THREAD_STRING_LENGTH];
};

#endif
