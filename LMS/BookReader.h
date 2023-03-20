#pragma once

#include <QMainWindow>
#include "ui_BookReader.h"

namespace lms {

class BookReader : public QMainWindow {
    Q_OBJECT

public:
    explicit BookReader(QWidget *parent = nullptr);
    ~BookReader();

private:
    Ui::BookReaderClass ui_;
};

}
