#include "BookReader.h"
#include <qdebug.h>

using namespace lms;

BookReader::BookReader(QWidget *parent)
    : QMainWindow(parent) {
    ui_.setupUi(this);
    qDebug() << "读者构造";
}

BookReader::~BookReader() {
    qDebug() << "读者析构";
}

void BookReader::on_backBtn_clicked() {
    emit BackBtnSignal(this);

}
