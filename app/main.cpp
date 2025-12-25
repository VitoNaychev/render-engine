#include "app.h"
#include <iostream>
#include <exception>
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    DragonApp app;
    assert(app.init());
    return a.exec();
}
