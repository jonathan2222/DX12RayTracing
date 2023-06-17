#pragma once

#include <queue>
#include <mutex>

namespace RS
{
	template<typename T>
	class ThreadSafeQueue
	{
	public:
		ThreadSafeQueue();
		ThreadSafeQueue(const ThreadSafeQueue& copy);
	
		/*
		* Push a value into the back of the queue.
		*/
		void Push(T value);

		/*
		* Try to pop a value from the front of the queue.
		* @returns false if the queue is empty.
		*/
		bool TryPop(T& value);

		/*
		* Check to see if there are any items in the queue.
		*/
		bool Empty() const;

		/*
		* Retrieve the number of items in the queue.
		*/
		size_t Size() const;

	private:
		std::queue<T> m_Queue;
		mutable std::mutex m_Mutex;
	};

	template<typename T>
	inline ThreadSafeQueue<T>::ThreadSafeQueue()
	{}

	template<typename T>
	inline ThreadSafeQueue<T>::ThreadSafeQueue(const ThreadSafeQueue& copy)
	{
		std::lock_guard<std::mutex> lock(m_Queue);
		std::lock_guard<std::mutex> lock(copy.m_Mutex);
		m_Queue = copy.m_Queue;
	}

	template<typename T>
	inline void ThreadSafeQueue<T>::Push(T value)
	{
		std::lock_guard<std::mutex> lock(m_Mutex);
		m_Queue.push(std::move(value));
	}

	template<typename T>
	inline bool ThreadSafeQueue<T>::TryPop(T& value)
	{
		std::lock_guard<std::mutex> lock(m_Mutex);
		if (m_Queue.empty())
			return false;

		value = m_Queue.front();
		m_Queue.pop();

		return true;
	}

	template<typename T>
	inline bool ThreadSafeQueue<T>::Empty() const
	{
		std::lock_guard<std::mutex> lock(m_Mutex);
		return m_Queue.empty();
	}

	template<typename T>
	inline size_t ThreadSafeQueue<T>::Size() const
	{
		std::lock_guard<std::mutex> lock(m_Mutex);
		return m_Queue.size();
	}
}