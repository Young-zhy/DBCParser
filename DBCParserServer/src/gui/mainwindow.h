#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "websocketserver/websocketserver.h"
#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
  Q_OBJECT

public:
  explicit MainWindow(QWidget *parent = nullptr);
  ~MainWindow() override;

private slots:

private:
  Ui::MainWindow *ui;
  WebSocketServer server;
};
#endif // MAINWINDOW_H
