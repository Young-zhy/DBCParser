// logger/QtTextEditSink.h
#pragma once
#include <QMetaObject>
#include <QTextEdit>
#include <spdlog/sinks/base_sink.h>

template <typename Mutex>
class QtTextEditSink : public spdlog::sinks::base_sink<Mutex> {
public:
  explicit QtTextEditSink(QTextEdit *widget) : m_widget(widget) {}

protected:
  void sink_it_(const spdlog::details::log_msg &msg) override {
    spdlog::memory_buf_t buf;
    this->formatter_->format(msg, buf);

    QString text = QString::fromStdString(fmt::to_string(buf));

    // 跨线程安全投递到 UI 线程
    QMetaObject::invokeMethod(
        m_widget,
        [w = m_widget, t = std::move(text)]() {
          if (!w)
            return;
          // w->append(t);
          auto cursor = w->textCursor();
          cursor.movePosition(QTextCursor::End);
          cursor.insertText(t);
          w->setTextCursor(cursor);
        },
        Qt::QueuedConnection);
  }

  void flush_() override {}

private:
  QTextEdit *m_widget{nullptr};
};