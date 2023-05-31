#ifndef LMS_LMS_MAIN_WINDOW_H
#define LMS_LMS_MAIN_WINDOW_H

#include <QtWidgets/QMainWindow>
#include <qnetworkaccessmanager.h>
#include <qnetworkreply.h>
#include <ui_lms_main_window.h>

#include "lms_config.h"

namespace lms {

enum PageIndex {
    kMenuPage = 0,
    kAdministratorBookPage = 0,
    kAdministratorUserPage = 1,
    kNormalBookSearchPage = 1,
    kAdministratorBorrowPage = 2,
    kUserSignInPage = 2,
    kUserSignUpPage = 3,
    kAdministratorPage = 4,
    kNormalUserPage = 5
};

class LMSMainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit LMSMainWindow(QWidget *parent = nullptr);
    ~LMSMainWindow();
signals:
    void ShowBookListReplyCBSignal(QNetworkReply *reply, QTableWidget *table,
            lms::LMSMainWindow *p, bool is_reader);
    void ShowUserListReplyCBSignal(QNetworkReply *reply, QTableWidget *table,
            lms::LMSMainWindow *p);
    void ShowBorrowListReplyCBSignal(QNetworkReply *reply, QTableWidget *table,
            lms::LMSMainWindow *p, bool is_reader);
public slots:
private slots:
    // 下列为页面切换
    // 由默认页面切换到普通搜索书籍页面
    void on_menu_search_book_btn_clicked();
    // 由默认页面切换到登录页面
    void on_menu_sign_in_btn_clicked();
    // 由登录页面切换到注册页面
    void on_user_login_sign_up_btn_clicked();
    // 由普通搜索书籍页面切换到默认页面
    void on_search_back_btn_clicked();
    // 由登录页面切换到默认页面
    void on_user_login_back_btn_clicked();
    // 由注册页面切换到默认页面
    void on_user_sign_up_back_btn_clicked();
    // 以下为管理员页面切换
    // 书籍管理页面
    void on_administrator_book_btn_clicked();
    // 用户管理页面
    void on_administrator_user_btn_clicked();
    // 借阅登记页面
    void on_administrator_borrow_or_return_btn_clicked();
    // 返回默认页面
    void on_administrator_back_btn_clicked();
    // 返回用户页面返回默认页面
    void on_user_back_btn_clicked();

    // 下列为登录注册
    // 读者登录
    void on_user_login_normal_btn_clicked();
    // 管理员登录
    void on_user_login_administrator_btn_clicked();
    // 注册
    void on_user_sign_up_yse_btn_clicked();

    // 借阅登记
    void on_administrator_borrow_sign_btn_clicked();

    // 下列为翻页
    // 管理员书籍上一页
    void on_administrator_book_pre_btn_clicked();
    // 管理员书籍下一页
    void on_administrator_book_next_btn_clicked();
    // 管理员用户上一页
    void on_administrator_user_pre_btn_clicked();
    // 管理员用户下一页
    void on_administrator_user_next_btn_clicked();
    // 管理员借书上一页
    void on_administrator_borrow_pre_btn_clicked();
    // 管理员借书下一页
    void on_administrator_borrow_next_btn_clicked();
    // 读者借书上一页
    void on_user_pre_btn_clicked();
    // 读者借书下一页
    void on_user_next_btn_clicked();
    // 普通书籍上一页
    void on_search_pre_btn_clicked();
    // 普通书籍下一页
    void on_search_next_btn_clicked();

    // 工作按钮
    // 管理员书籍搜索
    void on_administrator_book_do_search_btn_clicked();
    // 管理员添加书籍
    void on_administrator_book_btn_insert_btn_clicked();
    // 书籍搜索
    void on_search_do_search_btn_clicked();
    // 用户搜索
    void on_administrator_user_do_search_btn_clicked();
    // 借阅搜索
    void on_administrator_borrow_do_search_btn_clicked();
public:
    BookInfo BookAt(int row) const;
    UserInfo UserAt(int row) const;
    BorrowInfo BorrowAt(bool is_reader, int row) const;
    QNetworkReply* SendPost(std::string_view url_type, const QJsonDocument &json_doc);
    void ShowBookList(QTableWidget *table_widget, bool is_reader);
    void ShowUserList(QTableWidget *table_widget);
    void ShowBorrowList(QTableWidget* table_widget, bool is_reader);
private: // 私有工作函数
    QNetworkReply* UserLogin(bool is_reader);
    void InitPageLabel();

private:
    int s_n_book_page_num_ = 1;
    int s_n_borrow_page_num_ = 1;
    int s_a_book_page_num_ = 1;
    int s_a_user_page_num_ = 1;
    int s_a_borrow_page_num_ = 1;
    QNetworkAccessManager *net_manager_ = new QNetworkAccessManager(this);
    Ui::LMSMainWindow ui_;
};

}

#endif // !LMS_LMS_MAIN_WINDOW_H