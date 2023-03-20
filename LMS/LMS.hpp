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
    // 用于进行菜单界面到用户界面的设置
    template<typename T>
    void ChangeWidgetMenuToUser() {
        user_ = std::make_shared<T>();
        auto user_ptr = 
            std::get<std::shared_ptr<T>>(user_);
        this->close();
        this->connect(user_ptr.get(), &T::BackBtnSignal,
            this, &LMS::BackToMenu);
        user_ptr->show();
    }
private slots:
    // 用于登录对应的用户界面
    void on_sysManagerBtn_clicked();
    void on_bookManagerBtn_clicked();
    void on_bookReaderBtn_clicked();
public slots:
    // 其它界面到菜单界面的设置
    void BackToMenu(QMainWindow *widget);
};

}
