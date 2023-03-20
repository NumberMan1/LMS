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
    explicit LMS(QWidget *parent = nullptr);
    ~LMS();

private:
    Ui::LMSClass ui_;
    std::variant<
        std::shared_ptr<BookReader>,
        std::shared_ptr<BookManager>,
        std::shared_ptr<SysManager>
        > user_;
    template<typename T>
    void ChangeWidgetOnMenu() {
        user_ = std::make_shared<T>();
        auto user_ptr = 
            std::get<std::shared_ptr<T>>(user_);
        this->close();
        user_ptr->show();
    }
private slots:
    void on_SysManagerBtn_clicked();
    void on_BookManagerBtn_clicked();
    void on_BookReaderBtn_clicked();
};

}
