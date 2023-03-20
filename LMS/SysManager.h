#pragma once

#include <QMainWindow>
#include "ui_SysManager.h"

namespace lms {

class SysManager : public QMainWindow {
    Q_OBJECT

public:
    explicit SysManager(QWidget *parent = nullptr);
    ~SysManager();

signals:
    // 用于返回到菜单界面
    void BackBtnSignal(QMainWindow *widget);

private:
    Ui::SysManagerClass ui_;
private slots:
    void on_backBtn_clicked();
};

}
