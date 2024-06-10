//===-----------------------------------------------------------===//
// 
//                        WebServer                                 
// 
// block_queue.h
// Identification: src/include/log/block_queue.h
//
//===-----------------------------------------------------------===//

#pragma once

#include <iostream>
#include <cstdlib>
#include <mutex>
#include <condition_variable>
#include <list>
#include <memory>
#include <chrono>

namespace WebServer {

template<class T>
class BlockQueue {
 public:
	BlockQueue() = default;

	~BlockQueue() = default;

	void clear();

	auto empty() -> bool;

	auto front(T &value) -> bool;

	auto back(T &value) -> bool;

	auto size() -> size_t { return queue_.size(); }

	auto push(const T &item) -> bool;

	auto pop(T &item) -> bool;

	auto pop(T &item, int ms_timeout) -> bool;

	
 private:
	std::mutex m_;
	std::condition_variable cv_;

	std::list<std::shared_ptr<T>> queue_;


};

} // WebServer