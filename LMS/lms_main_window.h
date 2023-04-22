#ifndef LMS_LMS_MAIN_WINDOW_H
#define LMS_LMS_MAIN_WINDOW_H

#include <QtWidgets/QMainWindow>
#include <variant>
#include "ui_lms_main_window.h"

namespace lms {

class LMS : public QMainWindow {
    Q_OBJECT

public:
    explicit LMS(QWidget *parent = nullptr);
    ~LMS();
private slots:
public slots:
private:
    Ui::LMSMainWindow ui_;
};

}

#endif // !LMS_LMS_MAIN_WINDOW_H