// logger/logger.cpp
#include "logger.h"
#include "QtTextEditSink.h"

#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

Logger &Logger::instance() {
  static Logger inst;
  return inst;
}

void Logger::init(QTextEdit *textEdit, const QString &logFile,
                  size_t maxFileSize, size_t maxFiles) {
  if (m_logger)
    return; // 防止重复初始化

  // 1. Qt sink
  auto qt_sink = std::make_shared<QtTextEditSink<std::mutex>>(textEdit);

  // 2. 控制台 sink
  auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();

  // 3. 文件滚动 sink
  auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
      logFile.toStdString(), maxFileSize, maxFiles);

  // 4. 创建 logger（多 sink）
  m_logger = std::make_shared<spdlog::logger>(
      "app", spdlog::sinks_init_list{qt_sink, console_sink, file_sink});

  // 5. 注册为默认 logger
  spdlog::set_default_logger(m_logger);

  // 6. 日志级别
  m_logger->set_level(spdlog::level::debug);
  m_logger->flush_on(spdlog::level::info);

  // 7. 格式（你项目适合带时间）
  spdlog::set_pattern("[%Y-%m-%d %H:%M:%S] [%^%l%$] %v");
}

void Logger::setLevel(int level) {
  if (m_logger)
    m_logger->set_level(static_cast<spdlog::level::level_enum>(level));
}

std::shared_ptr<spdlog::logger> Logger::get() { return m_logger; }