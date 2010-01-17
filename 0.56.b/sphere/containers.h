#ifndef __CONTAINERS_H__
#define __CONTAINERS_H__
#pragma once

// a thread-safe implementation of a queue container that doesn't use any locks
// this only works as long as there is only a single reader thread and writer thread
template<class T>
class ThreadSafeQueue
{
public:
	typedef std::list<T> list;
	typedef typename std::list<T>::iterator iterator;
	typedef typename std::list<T>::const_iterator const_iterator;

private:
	list m_list;
	iterator m_head;
	iterator m_tail;

public:
	ThreadSafeQueue()
	{
		m_list.push_back(T()); // at least one element must be in the queue
		m_head = m_list.begin();
		m_tail = m_list.end();
	}

	// Append an element to the end of the queue (writer)
	void push(const T& value)
	{
		m_list.push_back(value);
		m_tail = m_list.end();
		m_list.erase(m_list.begin(), m_head);
	}

	// Retrieve the number of elements in the queue (reader/writer)
	size_t size(void) const
	{
		if (empty())
			return 0;

		int toSkip = 1;
		for (const_iterator it = m_list.begin(); it != m_head && it != m_list.end(); it++)
		{
			if (it == m_list.end())
				break;

			toSkip++;
		}

		return m_list.size() - toSkip;
	}

	// Determine if the queue is empty (reader/writer)
	bool empty(void) const
	{
		iterator next = m_head;
		next++;

		return (next == m_tail);
	}

	// Remove the first element from the queue (reader)
	void pop(void)
	{
		if (empty())
			throw new CException(LOGL_ERROR, 0, "No elements to read from queue.");

		iterator next = m_head;
		next++;

		if (next != m_tail)
			m_head = next;
	}

	// Retrieve the first element in the queue (reader)
	T front(void) const
	{
		if (empty() == false)
		{
			iterator next = m_head;
			next++;

			if (next != m_tail)
				return *next;
		}

		// this should never happen
		throw new CException(LOGL_ERROR, 0, "No elements to read from queue.");
	}
};

#endif
