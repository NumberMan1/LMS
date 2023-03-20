#include "LMS.hpp"

using namespace lms;

LMS::LMS(QWidget *parent)
    : QMainWindow(parent) {
    ui_.setupUi(this);
}

LMS::~LMS() {

}

void LMS::on_SysManagerBtn_clicked() {
    this->ChangeWidgetOnMenu<SysManager>();
}

void LMS::on_BookManagerBtn_clicked() {
    this->ChangeWidgetOnMenu<BookManager>();
}

void LMS::on_BookReaderBtn_clicked() {
    this->ChangeWidgetOnMenu<BookReader>();
}
