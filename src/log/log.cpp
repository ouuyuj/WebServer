#include "log/log.h"

#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <thread>
#include <chrono>
#include <string_view>


namespace WebServer {

Log::~Log() {
  if (fs_) {
    fs_.close();
  }
}

auto Log::init(const char *file_name, int close_log, int log_buf_size = 8192, 
                int split_lines = 5000000, bool is_async) -> bool {
  if (is_async) {
    is_async_ = is_async;
    log_queue_ = std::make_unique<BlockQueue<std::string>>();

    std::thread t(flush_log_thread, nullptr);
  }

  close_log_ = close_log;
  log_buf_size_ = log_buf_size;
  buf_ = new char[log_buf_size_];
  memset(buf_, 0, log_buf_size_);
  split_lines_ = split_lines;

  time_t t = time(NULL);
  struct tm *sys_tm = localtime(&t);
  struct tm my_tm = *sys_tm;
  
  const char* p = strrchr(file_name, '/');
  char log_full_name[256] = {0};

  if (p == nullptr) {
    snprintf(log_full_name, 255, "%d_%02d_%02d_%s", my_tm.tm_year + 1900, my_tm.tm_mon + 1, my_tm.tm_mday, file_name);
  } else {
    strcpy(log_name_, p + 1);
    strncpy(dir_name_, file_name, p - file_name + 1);
    snprintf(log_full_name, 255, "%s%d_%02d_%02d_%s", dir_name_, my_tm.tm_year + 1900, my_tm.tm_mon + 1, my_tm.tm_mday, log_name_);
  }

  today_ = my_tm.tm_mday;

  fs_.open(log_full_name, std::ios::in | std::ios::app);
  if (fs_) {
    return true;
  }

  return false;
}

auto Log::get_instance() -> Log * {
  static Log log;
  return &log;
}

auto Log::flush_log_thread(void *args) -> void * {
  Log::get_instance()->async_write_log();
}

void Log::flush(void) {
  std::lock_guard<std::mutex> guard(m_);
  fs_.flush();
}

auto Log::async_write_log() -> void * {
  std::string single_log;
  while (log_queue_->pop(single_log)) {
    std::lock_guard<std::mutex> guard(m_);
    fs_.write(single_log.c_str(), single_log.length());
  }
}

void Log::write_log(LogLevel level, const char *format, ...) {
  struct timeval now = {0, 0};
  gettimeofday(&now, nullptr);

  time_t t = now.tv_sec;
  struct tm *sys_tm = localtime(&t);
  struct tm my_tm = *sys_tm;
  char s[16] = {0};

  switch (level) {
    case LogLevel::DEBUG: 
      strcpy(s, "[debug]:");
      break;
    case LogLevel::INFO:
      strcpy(s, "[info]:");
      break;
    case LogLevel::WARN:
      strcpy(s, "[warn]:");
      break;
    case LogLevel::ERROR:
      strcpy(s, "[error]:");
      break;
    default:
      strcpy(s, "[info]:");
      break;
  }

  // 写入一个log，对m_count++, m_split_lines最大行数
  std::unique_lock<std::mutex> lock(m_);
  count_++;

  if (today_ != my_tm.tm_mday || count_ % split_lines_ == 0) { // everyday log
    char new_log[256] = {0};
    fs_.flush();
    fs_.close();
    char tail[16] = {0};

    snprintf(tail, 16, "%d_%02d_%02d_", my_tm.tm_year + 1900, my_tm.tm_mon + 1, my_tm.tm_mday);

    if (today_ != my_tm.tm_mday) {
      snprintf(new_log, 255, "%s%s%s", dir_name_, tail, log_name_);
      today_ = my_tm.tm_mday;
      count_ = 0;
    } else {
      snprintf(new_log, 255, "%s%s%s.%lld", dir_name_, tail, log_name_, count_ / split_lines_);
    }

    fs_.open(new_log, std::ios::in | std::ios::app);
  }

  lock.unlock();

  va_list valst;
  va_start(valst, format);

  std::string log_str;
  lock.lock();

  // 写入的具体时间内容格式
  int n = snprintf(buf_, 48, "%d-%02d-%02d %02d:%02d:%02d.%06ld %s ",
                   my_tm.tm_year + 1900, my_tm.tm_mon + 1, my_tm.tm_mday,
                   my_tm.tm_hour, my_tm.tm_min, my_tm.tm_sec, now.tv_usec, s);
  
  int m = vsnprintf(buf_ + n, log_buf_size_ - n - 1, format, valst);

  buf_[n + m] = '\n';
  buf_[n + m + 1] = '\0';
  log_str = buf_;

  lock.unlock();

  if (is_async_) {
    log_queue_->push(log_str);
  } else {
    lock.lock();
    fs_.write(log_str.c_str(), log_str.length());
    lock.unlock();
  }

  va_end(valst);
}

} // WebServer

