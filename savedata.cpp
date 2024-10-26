#include "savedata.h"

SaveData::SaveData(QObject *parent) : QObject(parent)
{

}

SaveData * SaveData::Singleton()
{
    static SaveData inst(nullptr);
    return &inst;
}

bool SaveData::SaveDataToLocal(QString Path, QString SaveName, QVariantMap Result)
{
    try {

        QString FilePath = Path + "/" + SaveName;

        QDir dir(Path);
        if (!dir.exists())
        {
            dir.mkpath(Path);
        }

        QFile SaveFile(FilePath);
        QFileInfo f(FilePath);
        if (!f.isFile())
        {
            QString Title = "";
            SaveFile.open(QIODevice::WriteOnly);
            for (QString key: Result.keys())
            {
                Title += key + ",";
            }

//            QVariantMap::iterator it;
//            for (it = Result.begin(); it != Result.end(); it++)
//            {
//                Title += it.key() + ",";
//            }

            Title += "\r\n";
            SaveFile.write(Title.toUtf8());
            SaveFile.close();
        }

        SaveFile.open(QIODevice::WriteOnly | QIODevice::Append);
        foreach(auto it, Result)
        {
            SaveFile.write((it.toString().toStdString() + ",").c_str());
        }
        SaveFile.write("\r\n");
        SaveFile.close();
        return true;

    } catch (const std::exception&) {
        return false;
    }
}
