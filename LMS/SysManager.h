#pragma once

#include <QMainWindow>
#include "ui_SysManager.h"

namespace lms {

class SysManager : public QMainWindow {
    Q_OBJECT

public:
    SysManager(QWidget *parent = nullptr);
    ~SysManager();

private:
    Ui::SysManagerClass ui_;
};

}
