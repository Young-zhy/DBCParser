#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include "../dbc_parser.h"
#include <QByteArray>
#include <QFileDialog>
#include <QMainWindow>
#include <QMap>
#include <QStandardItemModel>
#include <QWebSocket>

inline QByteArray buildCanFrame(const Message &msg,
                                const QMap<QString, double> &physicalValues) {
  uint8_t data[8] = {0};

  for (const auto &sig : msg.signal) {
    if (!physicalValues.contains(sig.name))
      continue;

    double physical = physicalValues[sig.name];

    // 范围校验（加分）
    if (physical < sig.minValue || physical > sig.maxValue)
      continue;

    int64_t raw = physicalToRaw(sig, physical);

    if (sig.isIntel)
      packIntel(data, sig, raw);
    else
      packMotorola(data, sig, raw);
  }

  return QByteArray(reinterpret_cast<char *>(data), msg.dlc);
}

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
  Q_OBJECT

public:
  enum ItemType { Item_Message = 1000, Item_Signal = 2000 };
  explicit MainWindow(QWidget *parent = nullptr);
  ~MainWindow() override;

private slots:
  void on_pushButton_file_clicked();

  void on_pushButton_websocket_connect_clicked();

  void on_pushButton_websocket_send_clicked();

  void on_pushButton_file_load_clicked();

private:
  QMap<QString, double> collectSignalValues(QStandardItem *msgItem);
  QWebSocket webSocket;
  DBCParser dbcParser;

  Ui::MainWindow *ui;
  QStandardItemModel *model;
};
#endif // MAIN_WINDOW_H
