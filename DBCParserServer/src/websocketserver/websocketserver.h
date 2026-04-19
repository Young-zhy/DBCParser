#pragma once

#include "logger/logger.h"
#include <QHostAddress>
#include <QObject>
#include <QSet>

class QWebSocketServer;
class QWebSocket;

class WebSocketServer : public QObject {
  Q_OBJECT
public:
  explicit WebSocketServer(QObject *parent = nullptr);
  ~WebSocketServer();

  bool start(quint16 port, const QHostAddress &addr = QHostAddress::Any);
  void stop();

  // 广播
  void broadcastText(const QString &msg);

signals:
  void dbcMessage(const QString &msg);
  void clientCountChanged(int n);

private slots:
  void onNewConnection();
  void onTextMessageReceived(const QString &msg);
  void onBinaryMessageReceived(const QByteArray &data);
  void onDisconnected();

private:
  QWebSocketServer *m_server{nullptr};
  QSet<QWebSocket *> m_clients;
};