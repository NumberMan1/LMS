#include "LMS.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    LMS w;
    w.show();
    return a.exec();
}
