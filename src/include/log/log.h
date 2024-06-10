//===-----------------------------------------------------------===//
// 
//                        WebServer                                 
// 
// log.h
// Identification: src/include/log/log.h
//
//===-----------------------------------------------------------===//

#pragma once

#include <stdio.h>
#include <stdarg.h>
#include <fstream>
#include <iostream>
#include <string>
#include <memory>

#include "block_queue.h"

namespace WebServer {

enum class LogLevel {DEBUG, INFO, WARN, ERROR};

class Log {
 public:
	Log(const Log &) = delete;
	Log &operator=(const Log &) = delete;
	// 懒汉模式
	static auto get_instance() -> Log *;

	static auto flush_log_thread(void *args) -> void *;
	// 异步需要设置阻塞队列的长度，同步不需要设置
	auto init(const char *file_name, int close_log, int log_buf_size = 8192, 
						int split_lines = 5000000, bool is_async = false) -> bool;

	void flush(void);
	
	void write_log(LogLevel level, const char *format, ...);
 private:
	Log() = default;
	virtual ~Log() = default;

	auto async_write_log() -> void *;

 private:
	char dir_name_[128]; // 路径名
	char log_name_[128]; // log文件名
	int split_lines_; // 日志最大行数
	int log_buf_size_; // 日志缓冲区大小
	int64_t count_{0}; // 日志行数记录
	int today_; // 因为按天分类,记录当前时间是那一天
	std::fstream fs_;
	char *buf_;
	std::unique_ptr<BlockQueue<std::string>> log_queue_;
	bool is_async_{false};
	std::mutex m_;
	int close_log_;
};

#define LOG_DEBUG(format, ...)                         				 							\
	if (0 == close_log_) {       																							\
		Log::get_instance()->write_log(LogLevel::DEBUG, format, ##__VA_ARGS__); \
		Log::get_intstance()->flush();																					\
	}

#define LOG_INFO(format, ...)                         											\
	if (0 == close_log_) {       																							\
		Log::get_instance()->write_log(LogLevel::INFO, format, ##__VA_ARGS__); 	\
		Log::get_intstance()->flush();																					\
	}

#define LOG_WARN(format, ...)                         											\
	if (0 == close_log_) {       																							\
		Log::get_instance()->write_log(LogLevel::WARN, format, ##__VA_ARGS__); 	\
		Log::get_intstance()->flush();																					\
	}

#define LOG_ERROR(format, ...)                         											\
	if (0 == close_log_) {       																							\
		Log::get_instance()->write_log(LogLevel::ERROR, format, ##__VA_ARGS__); \
		Log::get_intstance()->flush();																					\
	}

} // WebServer