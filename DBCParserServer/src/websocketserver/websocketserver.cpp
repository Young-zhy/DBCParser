#include "WebSocketServer.h"
#include "src/protobuf/can_frame.pb.h"
#include <QDateTime>
#include <QWebSocket>
#include <QWebSocketServer>

QString formatCanFrame(const CanFrame &frame) {
  QString result;

  // 1. 时间
  QString timeStr =
      QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");

  // 2. 头部
  result += QString("[%1] [ID: 0x%2] [Name: %3]\n")
                .arg(timeStr)
                .arg(frame.message_id(), 0, 16) // 16进制
                .arg(QString::fromStdString(frame.message_name()));

  // 3. Signal 列表
  for (const auto &sig : frame.signal()) {
    result += QString("  - %1: %2 %3 (raw: %4)\n")
                  .arg(QString::fromStdString(sig.name()))
                  .arg(sig.physical_value())
                  .arg(QString::fromStdString(sig.unit()))
                  .arg(sig.raw_value());
  }

  return result;
}

WebSocketServer::WebSocketServer(QObject *parent) : QObject(parent) {}

WebSocketServer::~WebSocketServer() { stop(); }

bool WebSocketServer::start(quint16 port, const QHostAddress &addr) {
  if (m_server)
    return true;

  m_server = new QWebSocketServer(QStringLiteral("Qt WS Server"),
                                  QWebSocketServer::NonSecureMode, this);

  if (!m_server->listen(addr, port)) {
    Logger::error("Listen failed: {}", m_server->errorString().toStdString());
    delete m_server;
    m_server = nullptr;
    return false;
  }

  connect(m_server, &QWebSocketServer::newConnection, this,
          &WebSocketServer::onNewConnection);

  Logger::info("Listening on {}:{}", addr.toString().toStdString(), port);
  return true;
}

void WebSocketServer::stop() {
  if (!m_server)
    return;

  for (auto *c : m_clients) {
    c->close();
    c->deleteLater();
  }
  m_clients.clear();

  m_server->close();
  m_server->deleteLater();
  m_server = nullptr;

  emit clientCountChanged(0);
  Logger::info("Server stopped");
}

void WebSocketServer::onNewConnection() {
  QWebSocket *socket = m_server->nextPendingConnection();

  connect(socket, &QWebSocket::textMessageReceived, this,
          &WebSocketServer::onTextMessageReceived);

  connect(socket, &QWebSocket::binaryMessageReceived, this,
          &WebSocketServer::onBinaryMessageReceived);

  connect(socket, &QWebSocket::disconnected, this,
          &WebSocketServer::onDisconnected);

  m_clients.insert(socket);

  emit clientCountChanged(m_clients.size());
  Logger::info("Client connected: {}",
               socket->peerAddress().toString().toStdString());
}

void WebSocketServer::onTextMessageReceived(const QString &msg) {
  auto *socket = qobject_cast<QWebSocket *>(sender());
  if (!socket)
    return;

  Logger::info("Recv[{}]: {}", socket->peerAddress().toString().toStdString(),
               msg.toStdString());

  // 示例：回显 + 广播
  socket->sendTextMessage("echo: " + msg);
  for (auto *c : m_clients) {
    if (c != socket)
      c->sendTextMessage(msg);
  }
}

void WebSocketServer::onBinaryMessageReceived(const QByteArray &data) {
  auto *socket = qobject_cast<QWebSocket *>(sender());
  if (!socket)
    return;

  // emit logMessage(QString("RecvBin[%1]: %2 bytes")
  //                     .arg(socket->peerAddress().toString())
  //                     .arg(data.size()));

  CanFrame frame;
  frame.ParseFromArray(data.data(), data.size());

  emit dbcMessage(formatCanFrame(frame));
}

void WebSocketServer::onDisconnected() {
  auto *socket = qobject_cast<QWebSocket *>(sender());
  if (!socket)
    return;

  m_clients.remove(socket);
  socket->deleteLater();

  emit clientCountChanged(m_clients.size());
  Logger::info("Client disconnected");
}

void WebSocketServer::broadcastText(const QString &msg) {
  for (auto *c : m_clients)
    c->sendTextMessage(msg);
}
