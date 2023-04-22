#include "lms_main_window.h"
#include <qdebug.h>

namespace lms {

LMS::LMS(QWidget *parent)
    : QMainWindow(parent) {
    ui_.setupUi(this);
}

LMS::~LMS() {

}

}
