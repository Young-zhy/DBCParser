#ifndef DBC_PARSER_H
#define DBC_PARSER_H

#include <QMap>
#include <QString>
#include <QVector>

struct Signal {
  QString name;
  QString multiplex;
  int startBit;
  int length;

  bool isIntel;  // true=Intel(小端), false=Motorola(大端)
  bool isSigned; // true=有符号, false=无符号

  double factor;
  double offset;

  double minValue;
  double maxValue;

  QString unit;
  QString receivers;
};

struct Message {
  int id;
  QString name;
  int dlc;
  QVector<Signal> signal;
};

inline void setBit(uint8_t *data, int bitIndex, bool value) {
  int byteIndex = bitIndex / 8;
  int bitInByte = bitIndex % 8;

  if (value)
    data[byteIndex] |= (1u << bitInByte);
  else
    data[byteIndex] &= ~(1u << bitInByte);
}

inline void packIntel(uint8_t *data, const Signal &sig, uint64_t raw) {
  for (int i = 0; i < sig.length; ++i) {
    bool bitVal = (raw >> i) & 0x1;
    int bitPos = sig.startBit + i;

    setBit(data, bitPos, bitVal);
  }
}

inline void packMotorola(uint8_t *data, const Signal &sig, uint64_t raw) {
  int bitPos = sig.startBit;

  for (int i = 0; i < sig.length; ++i) {
    bool bitVal = (raw >> (sig.length - 1 - i)) & 0x1;

    int byteIndex = bitPos / 8;
    int bitInByte = 7 - (bitPos % 8);

    if (bitVal)
      data[byteIndex] |= (1u << bitInByte);
    else
      data[byteIndex] &= ~(1u << bitInByte);

    // 计算下一个 bitPos
    if ((bitPos % 8) == 7)
      bitPos += 9; // 跳到下一字节最高位
    else
      bitPos -= 1;
  }
}

inline int64_t physicalToRaw(const Signal &sig, double physical) {
  double raw = (physical - sig.offset) / sig.factor;
  int64_t val = static_cast<int64_t>(std::llround(raw));

  // 截断到位宽
  int64_t maxVal = (1LL << sig.length) - 1;
  val &= maxVal;

  return val;
}

inline QByteArray packMessage(const Message &msg,
                              const QMap<QString, double> &physicalValues) {
  uint8_t data[8] = {0};

  for (const auto &sig : msg.signal) {
    if (!physicalValues.contains(sig.name))
      continue;

    double physical = physicalValues[sig.name];

    // 范围校验（加分项）
    // if (physical < sig.minValue || physical > sig.maxValue)
    //   continue;

    int64_t raw = physicalToRaw(sig, physical);

    if (sig.isIntel)
      packIntel(data, sig, raw);
    else
      packMotorola(data, sig, raw);
  }

  return QByteArray(reinterpret_cast<char *>(data), msg.dlc);
}

class DBCParser {
public:
  bool loadFile(const QString &filePath);
  QVector<Message> getMessages() const;

private:
  QVector<Message> messages;
};

#endif // DBC_PARSER_H