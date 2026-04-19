#ifndef WEBSOCKET_CLIENT_H
#define WEBSOCKET_CLIENT_H

#include <QWebSocket>

class WebSocketClient : public QObject {
    Q_OBJECT
public:
    WebSocketClient();
    void connectToServer(const QString &url);
    void sendMessage(const QString &message);

private:
    QWebSocket *webSocket;
};

#endif // WEBSOCKET_CLIENT_H