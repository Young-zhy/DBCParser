#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow) {
  ui->setupUi(this);

  this->setWindowTitle("DBC解析工具服务端");

  Logger::instance().init(ui->textEdit_log);

  QObject::connect(&server, &WebSocketServer::dbcMessage,
                   [&](const QString &s) {
                     qDebug() << s;
                     ui->textEdit_msg->append(s);
                   });

  ui->pushButton_websocket->setText("启动服务");
  ui->pushButton_websocket->setCheckable(true);
  ui->pushButton_websocket->setChecked(false);
  connect(ui->pushButton_websocket, &QPushButton::toggled, this,
          [=](bool checked) {
            if (checked) {
              auto port = ui->lineEdit_websocket->text().toUInt();
              if (server.start(port)) {
                ui->pushButton_websocket->setText("关闭服务");
                ui->lineEdit_websocket->setReadOnly(true);
              } else {
                ui->pushButton_websocket->setChecked(false);
              }
            } else {
              server.stop();
              ui->pushButton_websocket->setText("启动服务");
              ui->lineEdit_websocket->setReadOnly(false);
            }
          });
}

MainWindow::~MainWindow() { delete ui; }
