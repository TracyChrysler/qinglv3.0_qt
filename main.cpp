#include "dialog.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    Dialog *w = new Dialog(nullptr);
    w->show();
    return a.exec();
}
