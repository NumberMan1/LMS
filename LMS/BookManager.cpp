#include "BookManager.h"
#include <qdebug.h>

using namespace lms;

BookManager::BookManager(QWidget *parent)
    : QMainWindow(parent) {
    ui_.setupUi(this);
    qDebug() << "图书管理员构造";
}

BookManager::~BookManager() {
    qDebug() << "图书管理员析构";
}

void BookManager::on_backBtn_clicked() {
    emit BackBtnSignal(this);
}
