#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_LMS.h"

class LMS : public QMainWindow
{
    Q_OBJECT

public:
    LMS(QWidget *parent = nullptr);
    ~LMS();

private:
    Ui::LMSClass ui;
};
