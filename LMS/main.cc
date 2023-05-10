#include "lms_main_window.h"
#include <QtWidgets/QApplication>

using namespace lms;

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);
    LMSMainWindow w;
    w.show();
    return a.exec();
}
