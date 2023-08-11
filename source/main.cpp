#include "FuncWatcherDialog.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
    system("chcp 65001");

    QApplication a(argc, argv);
    FuncWatcherDialog w;
    w.show();
    return a.exec();
}
