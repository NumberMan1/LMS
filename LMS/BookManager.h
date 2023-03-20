#pragma once

#include <QMainWindow>
#include "ui_BookManager.h"

namespace lms {

class BookManager : public QMainWindow {
    Q_OBJECT

public:
    explicit BookManager(QWidget *parent = nullptr);
    ~BookManager();

private:
    Ui::BookManagerClass ui_;
};

}
