#include "dbc_parser.h"
#include <QFile>
#include <QTextStream>
#include <QRegularExpression>

bool DBCParser::loadFile(const QString &filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return false;

    QTextStream in(&file);

    Message currentMsg;
    bool hasMsg = false;

    QRegularExpression reMsg(
        R"(BO_\s+(\d+)\s+(\w+):\s+(\d+)\s+(\w+))");

    QRegularExpression reSig(
        R"(SG_\s+([^\s:]+)(?:\s+(m\d+|M))?\s*:\s*(\d+)\|(\d+)@(\d)([+-])\s*\(([-\d\.]+),([-\d\.]+)\)\s*\[([-\d\.]+)\|([-\d\.]+)\]\s*\"([^\"]*)\"\s+(.+))");

    QString line;
    messages.clear();
    while (in.readLineInto(&line))
    {
        line = line.trimmed();

        // -------- Message --------
        auto matchMsg = reMsg.match(line);
        if (matchMsg.hasMatch())
        {
            if (hasMsg)
                messages.push_back(currentMsg);

            currentMsg = Message{};
            currentMsg.id = matchMsg.captured(1).toInt();
            currentMsg.name = matchMsg.captured(2);
            currentMsg.dlc = matchMsg.captured(3).toInt();

            hasMsg = true;
            continue;
        }

        // -------- Signal --------
        auto matchSig = reSig.match(line);
        if (matchSig.hasMatch() && hasMsg)
        {
            Signal sig;
            sig.name = matchSig.captured(1);
            sig.multiplex=matchSig.captured(2);
            sig.startBit = matchSig.captured(3).toInt();
            sig.length = matchSig.captured(4).toInt();
            sig.isIntel = matchSig.captured(5) == "1" ? true: false;
            sig.isSigned = matchSig.captured(6) == "-";
            sig.factor = matchSig.captured(7).toDouble();
            sig.offset = matchSig.captured(8).toDouble();
            sig.minValue = matchSig.captured(9).toDouble();
            sig.maxValue = matchSig.captured(10).toDouble();
            sig.unit = matchSig.captured(11);
            sig.receivers=matchSig.captured(12);

            currentMsg.signal.push_back(sig);
        }
    }

    if (hasMsg)
        messages.push_back(currentMsg);

    return true;
}

QVector<Message> DBCParser::getMessages() const {
    return messages;
}