#include "main_window.h"
#include "../logger/logger.h"
#include "src/protobuf/can_frame.pb.h"
#include "ui_main_window.h"
#include <QDebug>
#include <QMessageBox>

void PrintCanFrame(const CanFrame &frame) {
  QString result;

  //  头部
  result += QString("[ID: 0x%2] [Name: %3]\n")
                .arg(frame.message_id(), 0, 16) // 16进制
                .arg(QString::fromStdString(frame.message_name()));

  // Signal 列表
  for (const auto &sig : frame.signal()) {
    result += QString("  - %1: %2 %3 (raw: %4)\n")
                  .arg(QString::fromStdString(sig.name()))
                  .arg(sig.physical_value())
                  .arg(QString::fromStdString(sig.unit()))
                  .arg(sig.raw_value());
  }

  Logger::info("{}", result.toStdString());
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), model(new QStandardItemModel(this)),
      ui(new Ui::MainWindow) {
  ui->setupUi(this);

  this->setWindowTitle("DBC解析工具");

  Logger::instance().init(ui->textEdit_log);

  ui->treeView->setModel(model);
  ui->treeView->setEditTriggers(QAbstractItemView::DoubleClicked);
  ui->treeView->setSelectionBehavior(QAbstractItemView::SelectRows);

  ui->pushButton_websocket_connect->setEnabled(true);
  ui->pushButton_websocket_send->setEnabled(false);

  // 创建 QWebSocket 实例
  ui->label_websocket_state->setText("未连接");
  ui->label_websocket_state->setAlignment(Qt::AlignCenter);
  ui->label_websocket_state->setStyleSheet("color: gray;");

  connect(&webSocket, &QWebSocket::connected, this, [this]() {
    ui->pushButton_websocket_send->setEnabled(true);
    Logger::info("WebSocket[{}] 连接成功",
                 ui->lineEdit_websocket->text().toStdString());
  });
  connect(&webSocket, &QWebSocket::disconnected, this, [this]() {
    ui->pushButton_websocket_send->setEnabled(false);
    Logger::info("WebSocket[{}] 连接断开",
                 ui->lineEdit_websocket->text().toStdString());
  });
  connect(&webSocket, &QWebSocket::stateChanged, this,
          [this](QAbstractSocket::SocketState state) {
            switch (state) {
            case QAbstractSocket::UnconnectedState:
              ui->label_websocket_state->setText("断开连接");
              ui->label_websocket_state->setStyleSheet("color: red;");
              break;
            case QAbstractSocket::ConnectingState:
              ui->label_websocket_state->setText("重连中...");
              ui->label_websocket_state->setStyleSheet("color: orange;");
              break;
            case QAbstractSocket::ConnectedState:
              ui->label_websocket_state->setText("已连接");
              ui->label_websocket_state->setStyleSheet("color: green;");
              break;
            default:
              break;
            }
          });
  connect(&webSocket, &QWebSocket::errorOccurred, this,
          [this](QAbstractSocket::SocketError error) {
            Logger::error("WebSocket[{}] 连接发生错误:{}",
                          ui->lineEdit_websocket->text().toStdString(),
                          static_cast<int>(error));
          });
}

MainWindow::~MainWindow() { delete ui; }

void MainWindow::on_pushButton_file_clicked() {
  QString filePath =
      QFileDialog::getOpenFileName(this, "选择文件", "", "DBC Files (*.dbc)");

  if (!filePath.isEmpty()) {
    ui->lineEdit_file->setText(filePath);
  }

  ui->lineEdit_file->setText(filePath);
}

void MainWindow::on_pushButton_websocket_connect_clicked() {

  // Get URL from user input
  QString urlString = ui->lineEdit_websocket->text();
  QUrl url(urlString);

  // If URL is valid, connect
  if (url.isValid()) {
    webSocket.open(url);
  } else {
    Logger::warn("Invalid URL.");
  }
}

void MainWindow::on_pushButton_websocket_send_clicked() {
  // Send message if connected
  if (webSocket.state() == QAbstractSocket::ConnectedState) {

    auto messages = dbcParser.getMessages();

    // 构建can frame

    QModelIndex index = ui->treeView->currentIndex();
    if (!index.isValid())
      return;
    // 强制取第0列（关键）
    QModelIndex firstColIndex = index.sibling(index.row(), 0);
    QStandardItem *item = model->itemFromIndex(firstColIndex);
    // 如果点的是 Signal，往上找 Message
    if (item->data(Qt::UserRole).toInt() == Item_Signal)
      item = item->parent();

    if (!item || item->data(Qt::UserRole).toInt() != Item_Message)
      return;
    // 获取 Message
    int msgIndex = item->data(Qt::UserRole + 1).toInt();
    const Message &msg = messages[msgIndex];

    // 收集该 Message 下所有 Signal 的物理值
    QMap<QString, double> physicalValues = collectSignalValues(item);

    // 打包
    QByteArray data = buildCanFrame(msg, physicalValues);

    // pb序列化
    CanFrame frame;
    frame.set_message_id(msg.id);
    frame.set_message_name(msg.name.toStdString());
    frame.set_data(data.data(), data.size());

    // 添加 signal 信息
    for (const auto &sig : msg.signal) {
      if (!physicalValues.contains(sig.name))
        continue;

      double physical = physicalValues[sig.name];
      int64_t raw = physicalToRaw(sig, physical);

      auto *s = frame.add_signal();
      s->set_name(sig.name.toStdString());
      s->set_physical_value(physical);
      s->set_unit(sig.unit.toStdString());
      s->set_raw_value(raw);
    }

    // 序列化
    std::string out;
    frame.SerializeToString(&out);
    PrintCanFrame(frame);
    webSocket.sendBinaryMessage(QByteArray::fromStdString(out));

  } else {
    Logger::info("Not connected to WebSocket.");
  }
}

QMap<QString, double> MainWindow::collectSignalValues(QStandardItem *msgItem) {
  QMap<QString, double> values;

  int rowCount = msgItem->rowCount();

  for (int i = 0; i < rowCount; ++i) {
    // Signal 行
    QStandardItem *sigNameItem = msgItem->child(i, 0);
    QStandardItem *physicalItem = msgItem->child(i, 12); // 第13列

    if (!sigNameItem || !physicalItem)
      continue;

    QString sigName = sigNameItem->text();
    double physical = physicalItem->text().toDouble();

    values[sigName] = physical;
  }

  return values;
}

void MainWindow::on_pushButton_file_load_clicked() {
  dbcParser.loadFile(ui->lineEdit_file->text());

  auto messages = dbcParser.getMessages();

  model->clear();

  // model->setHorizontalHeaderLabels({
  //     "Name","Multiplex", "StartBit", "Length","ByteOrder","Sign", "Factor",
  //     "Offset", "Min","Max","Unit","Receivers","PhysicalValue"
  // });
  model->setHorizontalHeaderLabels(
      {"名称", "复用标识", "起始位", "长度", "字节序", "符号类型", "比例因子",
       "偏移量", "最小值", "最大值", "单位", "接收节点", "物理值(用户输入)"});

  for (qsizetype msgIndex = 0; msgIndex < messages.size(); ++msgIndex) {
    const auto &msg = messages[msgIndex];

    // Message 节点
    QString msgText =
        QString("ID:%1 %2 (DLC:%3)").arg(msg.id).arg(msg.name).arg(msg.dlc);

    QStandardItem *msgItem = new QStandardItem(msgText);

    msgItem->setData(Item_Message, Qt::UserRole);
    // 保存 Message 索引（推荐）
    msgItem->setData(msgIndex, Qt::UserRole + 1);
    msgItem->setEditable(false);

    // 占位列
    QList<QStandardItem *> msgRow;
    msgRow << msgItem;
    for (int i = 1; i < model->columnCount(); ++i) {
      auto item = new QStandardItem("");
      item->setEditable(false);
      msgRow << item;
    }

    model->appendRow(msgRow);

    // Signal 子节点
    for (const auto &sig : msg.signal) {
      QList<QStandardItem *> sigRow;

      QStandardItem *sigItem = new QStandardItem(sig.name);
      sigItem->setData(Item_Signal, Qt::UserRole);
      // 保存 signal 名字（或索引）
      sigItem->setData(sig.name, Qt::UserRole + 1);

      sigRow << sigItem;
      sigRow << new QStandardItem(sig.multiplex);
      sigRow << new QStandardItem(QString::number(sig.startBit));
      sigRow << new QStandardItem(QString::number(sig.length));
      sigRow << new QStandardItem(QString::number(sig.isIntel));
      sigRow << new QStandardItem(QString::number(sig.isSigned));
      sigRow << new QStandardItem(QString::number(sig.factor));
      sigRow << new QStandardItem(QString::number(sig.offset));
      sigRow << new QStandardItem(QString::number(sig.minValue));
      sigRow << new QStandardItem(QString::number(sig.maxValue));
      sigRow << new QStandardItem(sig.unit);
      sigRow << new QStandardItem(sig.receivers);
      sigItem = new QStandardItem("");
      sigItem->setData(Item_Signal, Qt::UserRole);
      sigItem->setData(sig.name, Qt::UserRole + 1);
      sigRow << sigItem;

      for (int col = 0; col < sigRow.size(); ++col) {
        sigRow.at(col)->setEditable(col == sigRow.size() - 1);
      }
      msgItem->appendRow(sigRow);
    }
  }

  ui->treeView->expandAll();
  connect(model, &QStandardItemModel::dataChanged, this,
          [this](const QModelIndex &topLeft, const QModelIndex &bottomRight,
                 const QVector<int> &roles) {
            Q_UNUSED(roles);

            for (int row = topLeft.row(); row <= bottomRight.row(); ++row) {
              for (int col = topLeft.column(); col <= bottomRight.column();
                   ++col) {
                // 只处理第12列
                if (col != 12)
                  continue;

                // 关键：必须用 topLeft.parent()
                QModelIndex idx = model->index(row, col, topLeft.parent());

                if (!idx.isValid())
                  continue;

                bool ok = false;
                int value = model->data(idx).toDouble(&ok);
                if (!ok)
                  return;

                // 同一 parent 下取 min/max
                QModelIndex minIndex = model->index(row, 8, topLeft.parent());
                QModelIndex maxIndex = model->index(row, 9, topLeft.parent());

                int minVal = model->data(minIndex).toDouble();
                int maxVal = model->data(maxIndex).toDouble();

                if (value < minVal || value > maxVal) {
                  QSignalBlocker blocker(model);

                  // 回滚
                  model->setData(idx, ""); // 或 minVal / 或原值缓存

                  // 弹窗提示
                  auto err = QString("当前值 %1 超出范围 [%2, %3]")
                                 .arg(value)
                                 .arg(minVal)
                                 .arg(maxVal);
                  QMessageBox::warning(nullptr, "数据校验失败", err);

                  Logger::error("当前值 {} 超出范围 [{}, {}]", value, minVal,
                                maxVal);
                }
              }
            }
          });
}
