#include "SysManager.h"
#include <qdebug.h>

using namespace lms;

SysManager::SysManager(QWidget *parent)
    : QMainWindow(parent) {
    ui_.setupUi(this);
    qDebug() << "系统管理员构造";
}

SysManager::~SysManager() {
    qDebug() << "系统管理员析构";
}

void SysManager::on_backBtn_clicked() {
    emit BackBtnSignal(this);

}
