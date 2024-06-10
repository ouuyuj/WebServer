#include "log/block_queue.h"

namespace WebServer {

template<class T>
void BlockQueue<T>::clear()  {
	std::lock_guard<std::mutex> guard(m_);
	queue_.clear();
}

template<class T>
auto BlockQueue<T>::empty() -> bool {
	std::lock_guard<std::mutex> guard(m_);
	return queue_.empty();
}

template<class T>
auto BlockQueue<T>::front(T &value) -> bool {
	std::lock_guard<std::mutex> guard(m_);
	if (!queue_.empty()) {
		value = *queue_.front();
		return true;
	}
	return false;
}

template<class T>
auto BlockQueue<T>::back(T &value) -> bool {
	std::lock_guard<std::mutex> guard(m_);
	if (!queue_.empty()) {
		value = *queue_.back();
		return true;
	}
	return false;
}

template<class T>
auto BlockQueue<T>::push(const T &item) -> bool {
	std::lock_guard<std::mutex> guard(m_);
	queue_.push_back(std::make_shared<T>(item));
	cv_.notify_all();
	return true;
}

template<class T>
auto BlockQueue<T>::pop(T &item) -> bool {
	std::unique_lock<std::mutex> lock(m_);
	while (queue_.empty()) {
		cv_.wait(lock);
	}

	if (queue_.empty()) {
		return false;
	}

	item = *queue_.front();
	queue_.pop_front();
	cv_.notify_all();

	return true;
}

template<class T>
auto BlockQueue<T>::pop(T &item, int ms_timeout) -> bool {
	std::unique_lock<std::mutex> lock(m_);

	auto timeout_time = std::chrono::steady_clock::now() + std::chrono::milliseconds(ms_timeout);

	while (queue_.empty()) {
		// 等待条件变量，直到超时时间点或被通知
		if (!cv_.wait_until(lock, timeout_time, [this] { return !queue_.empty(); })) {
			// 超时时间到达，且队列为空
			return false;
		}
	}

	// 执行弹出操作
	item = *queue_.front();
	queue_.pop_front();

	return true;
}


} // WebServer