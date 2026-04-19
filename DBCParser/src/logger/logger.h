// logger/logger.h
#pragma once
#include <QString>
#include <memory>
#include <source_location>
#include <spdlog/common.h>
#include <spdlog/logger.h>

class QTextEdit;

namespace spdlog {
class logger;
}

class Logger {
public:
  static Logger &instance();

  // 初始化（在 UI 创建后调用一次）
  void init(QTextEdit *textEdit, const QString &logFile = "dbcparser.log",
            size_t maxFileSize = 5 * 1024 * 1024, size_t maxFiles = 3);

  // 推荐接口（无宏）
  template <typename... Args>
  static void info(fmt::format_string<Args...> fmt, Args &&...args) {
    log(spdlog::level::info, fmt, std::forward<Args>(args)...);
  }

  template <typename... Args>
  static void warn(fmt::format_string<Args...> fmt, Args &&...args) {
    log(spdlog::level::warn, fmt, std::forward<Args>(args)...);
  }

  template <typename... Args>
  static void error(fmt::format_string<Args...> fmt, Args &&...args) {
    log(spdlog::level::err, fmt, std::forward<Args>(args)...);
  }

  template <typename... Args>
  static void debug(fmt::format_string<Args...> fmt, Args &&...args) {
    log(spdlog::level::debug, fmt, std::forward<Args>(args)...);
  }

  void setLevel(int level); // 见 spdlog::level::level_enum

  // 直接暴露底层 logger（必要时）
  std::shared_ptr<spdlog::logger> get();

private:
  Logger() = default;
  ~Logger() = default;
  Logger(const Logger &) = delete;
  Logger &operator=(const Logger &) = delete;

  template <typename... Args>
  static void log(spdlog::level::level_enum lvl,
                  fmt::format_string<Args...> fmt, Args &&...args) {
    auto &t_logger = Logger::instance().m_logger;
    if (!t_logger)
      return;

    t_logger->log(lvl, fmt, std::forward<Args>(args)...);
  }

  template <typename... Args>
  static void log_with_loc(spdlog::level::level_enum lvl,
                           const std::source_location &loc,
                           std::string_view fmt, Args &&...args) {

    auto &t_logger = Logger::instance().m_logger;
    if (!t_logger)
      return;

    spdlog::source_loc sloc{loc.file_name(), static_cast<int>(loc.line()),
                            loc.function_name()};

    t_logger->log(sloc, lvl, fmt, std::forward<Args>(args)...);
  }

private:
  std::shared_ptr<spdlog::logger> m_logger;
};