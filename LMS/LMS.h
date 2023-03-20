#pragma once

#include <QtWidgets/QMainWindow>
#include <variant>
#include "ui_LMS.h"
#include "BookReader.h"
#include "BookManager.h"
#include "SysManager.h"

namespace lms {

class LMS : public QMainWindow {
    Q_OBJECT

public:
    LMS(QWidget *parent = nullptr);
    ~LMS();

private:
    Ui::LMSClass ui_;
    std::variant<BookReader, BookManager, SysManager> user_;
};

}
