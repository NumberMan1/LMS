#include "LMS.hpp"

using namespace lms;

LMS::LMS(QWidget *parent)
    : QMainWindow(parent) {
    ui_.setupUi(this);
}

LMS::~LMS() {

}

void LMS::on_sysManagerBtn_clicked() {
    this->ChangeWidgetMenuToUser<SysManager>();
}

void LMS::on_bookManagerBtn_clicked() {
    this->ChangeWidgetMenuToUser<BookManager>();
}

void LMS::on_bookReaderBtn_clicked() {
    this->ChangeWidgetMenuToUser<BookReader>();
}

void LMS::BackToMenu(QMainWindow* widget) {
    widget->close();
    this->show();
}
