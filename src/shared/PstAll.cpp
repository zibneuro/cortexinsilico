#include "PstAll.h"
#include <QFile>
#include <QTextStream>

void PstAll::load(QString filename) {
  QFile file(filename);
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    const QString msg =
        QString("Error reading features file. Could not open file %1")
            .arg(filename);
    throw std::runtime_error(qPrintable(msg));
  }

  QTextStream in(&file);
  in.readLine();
  QString line = in.readLine();
  line = in.readLine();
  while (!line.isNull()) {
    QStringList parts = line.split(',');
    PstAllProps props;
    props.SID = parts[0].toInt();    
    props.exc = parts[1].toFloat();
    props.inh = parts[2].toFloat();
    mPstAll[props.SID] = props;
    line = in.readLine();
  }
}

PstAllProps PstAll::get(int SID) {
    return mPstAll[SID];
}