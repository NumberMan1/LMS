#pragma once

#include <QMainWindow>
#include "ui_BookReader.h"

namespace lms {

class BookReader : public QMainWindow {
    Q_OBJECT

public:
    explicit BookReader(QWidget *parent = nullptr);
    ~BookReader();

signals:
    // 用于返回到菜单界面
    void BackBtnSignal(QMainWindow *widget);

private:
    Ui::BookReaderClass ui_;
private slots:
    void on_backBtn_clicked();
};

}
