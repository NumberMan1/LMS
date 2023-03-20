#pragma once

#include <QMainWindow>
#include "ui_BookManager.h"

namespace lms {

class BookManager : public QMainWindow {
    Q_OBJECT

public:
    explicit BookManager(QWidget *parent = nullptr);
    ~BookManager();

signals:
    // 用于返回到菜单界面
    void BackBtnSignal(QMainWindow *widget);

private:
    Ui::BookManagerClass ui_;
private slots:
    void on_backBtn_clicked();
};

}
