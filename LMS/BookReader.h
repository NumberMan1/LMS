#pragma once

#include <QMainWindow>
#include "ui_BookReader.h"

namespace lms {

class BookReader : public QMainWindow {
    Q_OBJECT

public:
    BookReader(QWidget *parent = nullptr);
    ~BookReader();

private:
    Ui::BookReaderClass ui_;
};

}
