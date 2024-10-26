#ifndef SAVEDATA_H
#define SAVEDATA_H

#include <QObject>
#include <QMap>
#include <QVariantMap>
#include <QDateTime>
#include <QDir>

class SaveData : public QObject
{
    Q_OBJECT
public:
    explicit SaveData(QObject *parent = nullptr);

    static SaveData * Singleton();


    bool SaveDataToLocal(QString Path, QString SaveName, QVariantMap Result);

signals:

};

#endif // SAVEDATA_H
