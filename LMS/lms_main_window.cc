#include "lms_main_window.h"
#include <qdebug.h>
#include <qjsondocument.h>
#include <qjsonarray.h>
#include <qjsonobject.h>
#include <qfile.h>
#include <regex>
#include <qmessagebox.h>
#include <qdatetimeedit.h>

#include "cjs/mine_hash.hpp"
#include "lms_user_singleton.h"
#include "lms_book_dialog.h"
#include "lms_user_dialog.h"
#include "lms_borrow_dialog.h"

namespace {

void PagePreImpl(int* page_num, QPushButton* pre_btn, QPushButton* next_btn) {
    --(*page_num);
    if (!next_btn->isEnabled()) {
        next_btn->setEnabled(true);
    }
    // 判断是否为第一页
    if (*page_num == 1) {
        pre_btn->setDisabled(true);
    }
}

void PageNextImpl(int* page_num, QPushButton* pre_btn, QPushButton* next_btn,
        QTableWidget *table) {
    ++(*page_num);
    if (!pre_btn->isEnabled()) {
        pre_btn->setEnabled(true);
    }
}

void PageRestartImpl(int* page_num, QPushButton* pre_btn, QPushButton* next_btn) {
    *page_num = 1;
    pre_btn->setDisabled(true);
    next_btn->setEnabled(true);
}

void EditBookFuncImpl(QTableWidget* table, lms::LMSMainWindow *p,
        const lms::BookInfo &info) {
    QJsonDocument json_doc;
    json_doc.setObject(
        {
            {"action", "update"},
            {"book_id", info.id},
            {"book_type", info.type},
            {"book_name", info.name.c_str()},
            {"book_auther", info.auther.c_str()},
            {"book_press", info.press.c_str()},
            {"book_price", info.price},
            {"book_num", info.num}
        }
    );
    auto reply = p->SendPost(lms::kBookURL, json_doc);
    p->connect(reply, &QNetworkReply::readyRead, p,
        [reply = reply, p = p, table = table]() {
            auto json = QJsonDocument::fromJson(reply->readAll());
            if (json["is_updated"].toBool()) {
                p->ShowBookList(table, false);
            } else {
                QMessageBox box;
                box.setText("编辑失败");
                box.exec();
                // 只有当成功时才自动释放
                reply->deleteLater();
            }
        });
}

void EditReaderFuncImpl(QTableWidget *table, lms::LMSMainWindow *p,
        const lms::UserInfo &info) {
    QJsonDocument json_doc;
    json_doc.setObject(
        {
            {"action", "update"},
            {"user_id", info.id},
            {"user_login_name", info.login_name.c_str()},
            {"user_name", info.name.c_str()},
            {"user_pwd", info.pwd.c_str()},
            {"user_tel", info.tel.c_str()}
        }
    );
    auto reply = p->SendPost(lms::kUserURL, json_doc);
    p->connect(reply, &QNetworkReply::readyRead,
        [reply = reply, table = table, p = p]() {
            auto json_doc = QJsonDocument::fromJson(reply->readAll());
            QMessageBox box{p};
            if (json_doc["is_updated"].toBool()) {
                box.setText("修改成功");
                p->ShowUserList(table);
            } else {
                box.setText("修改失败");
            }
            box.exec();
            reply->deleteLater();
        });
}

void DelImpl(int row, QTableWidget *table, lms::LMSMainWindow *p,
        const char *id_text, const char *url, bool is_book_table) {
    QJsonDocument json_doc;
    json_doc.setObject(
        {
            {"action", "delete"},
            {id_text, table->item(row, 0)->text().toInt()}
        }
    );
    auto reply = p->SendPost(url, json_doc);
    p->connect(reply, &QNetworkReply::readyRead,
        [reply = reply, p = p, table = table, type = is_book_table]() {
            auto json_doc = QJsonDocument::fromJson(reply->readAll());
            QMessageBox box{ p };
            if (json_doc["is_deleted"].toBool()) {
                box.setText("删除成功");
                if (type) {
                    p->ShowBookList(table, false);
                } else {
                    p->ShowUserList(table);
                }
            }
            else {
                box.setText("删除失败");
            }
            box.exec();
            reply->deleteLater();
        });
}

void ReturnBookFuncImpl(int row, QTableWidget *table, lms::LMSMainWindow *p, bool is_reader) {
    QJsonDocument json_doc;
    auto info = p->BorrowAt(is_reader, row);
    if (is_reader) {
        info.user_id = lms::UserSingleton::Instance()->GetId();
    }
    json_doc.setObject(
        {
            {"action", "return"},
            {"book_class_id", info.book_id},
            {"reader_id", info.user_id},
            {"borrow_deadline", info.borrow_deadline.c_str()}
        }
    );
    auto reply = p->SendPost(lms::kBorrowURL, json_doc);
    p->connect(reply, &QNetworkReply::readyRead, p,
        [p = p, reply = reply, table = table, is_reader = is_reader]() {
            auto json_doc = QJsonDocument::fromJson(reply->readAll());
            QMessageBox box{ p };
            if (json_doc["is_returned"].toBool()) {
                box.setText("还书成功");
                p->ShowBorrowList(table, is_reader);
            }
            else {
                box.setText("还书失败, 请检查是否付款");
            }
            box.exec();
            reply->deleteLater();
        });
}

void BorrowPayFuncImpl(int row, QTableWidget* table, lms::LMSMainWindow* p) {
    QJsonDocument json_doc;
    auto info = p->BorrowAt(true, row);
    json_doc.setObject(
        {
            {"action", "pay"},
            {"book_class_id", info.book_id},
            {"reader_id", lms::UserSingleton::Instance()->GetId()},
            {"borrow_deadline", info.borrow_deadline.c_str()}
        }
    );
    auto reply = p->SendPost(lms::kBorrowURL, json_doc);
    p->connect(reply, &QNetworkReply::readyRead, p,
        [p = p, reply = reply, table = table]() {
            auto json_doc = QJsonDocument::fromJson(reply->readAll());
            QMessageBox box{ p };
            if (json_doc["is_payed"].toBool()) {
                box.setText("还款成功");
                p->ShowBorrowList(table, true);
            }
            else {
                box.setText("还款失败");
            }
            box.exec();
            reply->deleteLater();
        });
}


void BorrowErrorImpl(int row, QTableWidget *table, lms::LMSMainWindow *p) {
    lms::BorrowDialog dialog{false, p};
    dialog.SetBorrowInfo(p->BorrowAt(false, row));
    dialog.exec();
    if (dialog.result()) {
        auto info = dialog.GetBorrowInfo();
        QJsonDocument json_doc;
        json_doc.setObject(
            {
                {"action", "error"},
                {"book_class_id", info.book_id},
                {"reader_id", info.user_id},
                {"borrow_price", info.price},
                {"borrow_date", info.borrow_date.c_str()},
                {"borrow_deadline", info.borrow_deadline.c_str()},
                {"error_text", dialog.GetErrorText().c_str()}
            }
        );
        auto reply = p->SendPost(lms::kBorrowURL, json_doc);
        p->connect(reply, &QNetworkReply::readyRead, p,
            [p = p, reply = reply, table = table]() {
                auto json_doc = QJsonDocument::fromJson(reply->readAll());
                QMessageBox box{p};
                if (json_doc["is_solved"].toBool()) {
                    box.setText("异常上报成功");
                    p->ShowBorrowList(table, false);
                } else {
                    box.setText("异常上报失败");
                }
                box.exec();
                reply->deleteLater();
            });
    }
}

void DelBookFuncImpl(int row, QTableWidget *table, lms::LMSMainWindow *p) {
    ::DelImpl(row, table, p, "book_id", lms::kBookURL, true);
}

void DelUserFuncImpl(int row, QTableWidget *table, lms::LMSMainWindow *p) {
    ::DelImpl(row, table, p, "user_id", lms::kUserURL, false);
}

void ShowBookListReplyCB(QNetworkReply *reply, QTableWidget *table,
        lms::LMSMainWindow *p, bool flag) {
    using namespace lms;
    QJsonDocument json_doc = QJsonDocument::fromJson(reply->readAll());
    // 删除之前的数据
    table->setRowCount(0);
    if (!json_doc.isEmpty()) {
        QJsonArray jarr = json_doc.array();
        auto arr_size = jarr.size();
        table->setRowCount(arr_size);
        // 禁止翻下页
        if (arr_size < 10) {
            if (flag) {
                p->SetNextBtnDisable(PageType::kNormalBookSearchPage);
            } else {
                p->SetNextBtnDisable(PageType::kAdministratorBookPage);
            }
        }
        for (int i = 0; auto value : jarr) {
            int j = 0;
            QJsonObject book = value.toObject();
            table->setItem(i, j,
                new QTableWidgetItem{ QString::number(book["book_id"].toInt()) });
            ++j;
            QComboBox* combo_box = new QComboBox(p);
            combo_box->addItem("教育");
            combo_box->addItem("童话");
            combo_box->addItem("文学");
            combo_box->addItem("科幻");
            combo_box->setCurrentIndex(book["book_type"].toInt() - 1);
            combo_box->setDisabled(true);
            table->setCellWidget(i, j, combo_box);
            ++j;
            table->setItem(i, j,
                new QTableWidgetItem{ book["book_name"].toString() });
            ++j;
            table->setItem(i, j,
                new QTableWidgetItem{ book["book_auther"].toString() });
            ++j;
            table->setItem(i, j,
                new QTableWidgetItem{ book["book_press"].toString() });
            ++j;
            table->setItem(i, j,
                new QTableWidgetItem{ QString::number(book["book_price"].toDouble()) });
            ++j;
            table->setItem(i, j,
                new QTableWidgetItem{ QString::number(book["book_num"].toInt()) });
            if (!flag) {
                QPushButton* btn = nullptr;
                btn = new QPushButton{ p };
                btn->setStyleSheet("border-image: url(:/LMS_BUTTON/picture/edit_btn.svg)");
                table->setCellWidget(i, kAdministratorBookTableEditColumn, btn);
                p->connect(btn, &QPushButton::clicked, p,
                    [table = table, t = p, row = i]() {
                        BookDialog dialog{ false, t };
                        dialog.SetBookInfo(t->BookAt(row));
                        dialog.exec();
                        if (dialog.result()) {
                            ::EditBookFuncImpl(table, t, dialog.GetBookInfo());
                        }
                    });
                btn = new QPushButton{ p };
                btn->setStyleSheet("border-image: url(:/LMS_BUTTON/picture/delete_btn.svg)");
                table->setCellWidget(i, kAdministratorBookTableDelColumn, btn);
                p->connect(btn, &QPushButton::clicked, p,
                    [row = i, table = table, p = p]() {
                        auto choice = 
                            QMessageBox::question(p, "确认",  "你确定要执行该操作吗？", 
                                QMessageBox::Yes | QMessageBox::No);
                        if (choice == QMessageBox::Yes) {
                            ::DelBookFuncImpl(row, table, p);
                        }
                    });
            }
            ++i;
        }
    }
    reply->deleteLater();
}

void ShowBorrowListReplyCB(QNetworkReply* reply, QTableWidget* table_widget,
    lms::LMSMainWindow* p, bool is_reader) {
    using namespace lms;
    QJsonDocument json_doc = QJsonDocument::fromJson(reply->readAll());
    // 删除之前的数据
    table_widget->setRowCount(0);
    if (!json_doc.isEmpty()) {
        QJsonArray jarr = json_doc.array();
        auto arr_size = jarr.size();
        table_widget->setRowCount(arr_size);
        // 禁止翻下页
        if (arr_size < 10) {
            if (is_reader) {
                p->SetNextBtnDisable(PageType::kNormalUserPage);
            } else {
                p->SetNextBtnDisable(PageType::kAdministratorBorrowPage);
            }
        }
        QPushButton* btn = nullptr;
        QDateEdit* time_edit = nullptr;
        for (int i = 0; auto value : jarr) {
            int j = 0;
            QJsonObject item = value.toObject();
            table_widget->setItem(i, j,
                new QTableWidgetItem{ QString::number(item["book_id"].toInt()) });
            ++j;
            table_widget->setItem(i, j,
                new QTableWidgetItem{ item["book_name"].toString() });
            ++j;
            if (!is_reader) { // 用于管理员页面
                table_widget->setItem(i, j,
                    new QTableWidgetItem{ QString::number(item["reader_id"].toInt()) });
                ++j;
                table_widget->setItem(i, j,
                    new QTableWidgetItem{ item["reader_login_name"].toString() });
                ++j;
                time_edit = new QDateEdit(p);
                time_edit->setDate(QDate::fromString(item["borrow_date"].toString(), kDateFormat));
                time_edit->setDisabled(true);
                table_widget->setCellWidget(i, j, time_edit);
                ++j;
            }
            time_edit = new QDateEdit(p);
            time_edit->setDate(QDate::fromString(item["borrow_deadline"].toString(), kDateFormat));
            time_edit->setDisabled(true);
            table_widget->setCellWidget(i, j, time_edit);
            ++j;
            table_widget->setItem(i, j,
                new QTableWidgetItem{ QString::number(item["book_price"].toDouble()) });
            if (!is_reader) { // 用于管理员页面
                btn = new QPushButton{ "还书", p };
                table_widget->setCellWidget(
                    i, kAdministratorBorrowTableReturnColumn, btn);
                p->connect(btn, &QPushButton::clicked, p,
                    [row = i, table = table_widget, p = p]() {
                        ::ReturnBookFuncImpl(row ,table ,p, false);
                    });
                btn = new QPushButton{ "异常", p };
                table_widget->setCellWidget(
                    i, kAdministratorBorrowTableErrorColumn, btn);
                p->connect(btn, &QPushButton::clicked, p,
                    [row = i, table = table_widget, p = p]() {
                        ::BorrowErrorImpl(row, table, p);
                    });
            } else {
                btn = new QPushButton{ "还书", p };
                table_widget->setCellWidget(
                    i, kUserBorrowTableReturnColumn, btn);
                p->connect(btn, &QPushButton::clicked, p,
                    [row = i, table = table_widget, p = p]() {
                        ::ReturnBookFuncImpl(row ,table, p, true);
                    });
                btn = new QPushButton{ "还款", p };
                table_widget->setCellWidget(
                    i, kUserBorrowTablePayColumn, btn);
                p->connect(btn, &QPushButton::clicked, p,
                    [row = i, table = table_widget, p = p]() {
                        ::BorrowPayFuncImpl(row, table, p);
                    });
            }
            ++i;
        }
    }
    reply->deleteLater();
}

void ShowUserListReplyCB(QNetworkReply* reply, QTableWidget *table,
        lms::LMSMainWindow *p) {
    using namespace lms;
    QJsonDocument json_doc = QJsonDocument::fromJson(reply->readAll());
    // 删除之前的数据
    table->setRowCount(0);
    if (!json_doc.isEmpty()) {
        QJsonArray jarr = json_doc.array();
        QPushButton* btn = nullptr;
        auto arr_size = jarr.size();
        table->setRowCount(arr_size);
        // 禁止翻下页
        if (arr_size < 10) {
            p->SetNextBtnDisable(PageType::kAdministratorUserPage);
        }
        for (int i = 0; auto value : jarr) {
            int j = 0;
            QJsonObject user = value.toObject();
            table->setItem(i, j,
                new QTableWidgetItem{ QString::number(user["reader_id"].toInt()) });
            ++j;
            table->setItem(i, j,
                new QTableWidgetItem{ user["reader_login_name"].toString() });
            ++j;
            table->setItem(i, j,
                new QTableWidgetItem{ user["reader_name"].toString() });
            ++j;
            table->setItem(i, j,
                new QTableWidgetItem{ user["reader_tel"].toString() });
            btn = new QPushButton{ p };
            btn->setStyleSheet("border-image: url(:/LMS_BUTTON/picture/edit_btn.svg)");
            table->setCellWidget(
                i, kAdministratorUserTableEditColumn, btn);
            p->connect(btn, &QPushButton::clicked, p,
                [row = i, p = p, table = table]() {
                    UserDialog dialog{p};
                    dialog.SetUserInfo(p->UserAt(row));
                    dialog.exec();
                    if (dialog.result()) {
                        ::EditReaderFuncImpl(table, p, dialog.GetUserInfo());
                    }
                });
            btn = new QPushButton{ p };
            btn->setStyleSheet("border-image: url(:/LMS_BUTTON/picture/delete_btn.svg)");
            table->setCellWidget(
                i, kAdministratorUserTableDelColumn, btn);
            p->connect(btn, &QPushButton::clicked, p,
                [row = i, table = table, p = p]() {
                    auto choice =
                        QMessageBox::question(p, "确认", "你确定要执行该操作吗？",
                            QMessageBox::Yes | QMessageBox::No);
                    if (choice == QMessageBox::Yes) {
                        ::DelUserFuncImpl(row, table, p);
                    }
                });
            ++i;
        }
    }
    reply->deleteLater();
}


} // !namespace::

namespace lms {

LMSMainWindow::LMSMainWindow(QWidget *parent)
        : QMainWindow(parent) {
    ui_.setupUi(this);
    ui_.administrator_book_table_widget->setColumnWidth(7, kEditOrDeleteBtnLen);
    ui_.administrator_book_table_widget->setColumnWidth(8, kEditOrDeleteBtnLen);
    ui_.administrator_user_table_widget->setColumnWidth(4, kEditOrDeleteBtnLen);
    ui_.administrator_user_table_widget->setColumnWidth(5, kEditOrDeleteBtnLen);
    InitPageLabel();
    connect(this, &LMSMainWindow::ShowBookListReplyCBSignal, ::ShowBookListReplyCB);
    connect(this, &LMSMainWindow::ShowUserListReplyCBSignal, ::ShowUserListReplyCB);
    connect(this ,&LMSMainWindow::ShowBorrowListReplyCBSignal, ::ShowBorrowListReplyCB);
    ShowBookList(ui_.administrator_book_table_widget, false);
    ShowUserList(ui_.administrator_user_table_widget);
    ShowBorrowList(ui_.administrator_borrow_table_widget, false);
}

LMSMainWindow::~LMSMainWindow() {
    qDebug() << "LMSMainWindow 析构";
}

// 下列为页面切换

void LMSMainWindow::on_menu_search_book_btn_clicked() {
    ::PageRestartImpl(&s_n_book_page_num_, ui_.search_pre_btn, ui_.search_next_btn);
    ui_.stacked_widget->setCurrentIndex(PageIndex::kNormalBookSearchPage);
    ShowBookList(ui_.search_book_table_widget, true);
    ui_.search_page_num_lable->setText(
        QString("第") + QString::number(s_n_book_page_num_) + QString("页"));
}

void LMSMainWindow::on_search_back_btn_clicked() {
    ui_.stacked_widget->setCurrentIndex(PageIndex::kMenuPage);
}

void LMSMainWindow::on_menu_sign_in_btn_clicked() {
    ui_.stacked_widget->setCurrentIndex(PageIndex::kUserSignInPage);
}

void LMSMainWindow::on_user_login_sign_up_btn_clicked() {
    ui_.stacked_widget->setCurrentIndex(PageIndex::kUserSignUpPage);
}

void LMSMainWindow::on_user_login_back_btn_clicked() {
    ui_.stacked_widget->setCurrentIndex(PageIndex::kMenuPage);
}

void LMSMainWindow::on_user_sign_up_back_btn_clicked() {
    ui_.stacked_widget->setCurrentIndex(PageIndex::kMenuPage);
}

// 以下为管理员页面切换

void LMSMainWindow::on_administrator_book_btn_clicked() {
    ::PageRestartImpl(&s_a_book_page_num_, ui_.administrator_book_pre_btn, ui_.administrator_book_next_btn);
    ui_.administrator_stack_widget->setCurrentIndex(PageIndex::kAdministratorBookPage);
    ShowBookList(ui_.administrator_book_table_widget, false);
}

void LMSMainWindow::on_administrator_user_btn_clicked() {
    ::PageRestartImpl(&s_a_user_page_num_, ui_.administrator_user_pre_btn, ui_.administrator_user_next_btn);
    ui_.administrator_stack_widget->setCurrentIndex(PageIndex::kAdministratorUserPage);
    ShowUserList(ui_.administrator_user_table_widget);
}

void LMSMainWindow::on_administrator_borrow_or_return_btn_clicked() {
    ::PageRestartImpl(&s_a_borrow_page_num_, ui_.administrator_borrow_pre_btn, ui_.administrator_borrow_next_btn);
    ui_.administrator_stack_widget->setCurrentIndex(PageIndex::kAdministratorBorrowPage);
    ShowBorrowList(ui_.administrator_borrow_table_widget, false);
}

void LMSMainWindow::on_administrator_back_btn_clicked() {
    ui_.stacked_widget->setCurrentIndex(PageIndex::kMenuPage);
}

void LMSMainWindow::on_user_back_btn_clicked() {
    ui_.stacked_widget->setCurrentIndex(PageIndex::kMenuPage);
}

// 下列为登录注册

void LMSMainWindow::on_user_login_normal_btn_clicked() {
    auto reply = UserLogin(true);
    connect(reply, &QNetworkReply::readyRead, this,
        [reply = reply, stack_widget = ui_.stacked_widget, p = this]() {
            auto json_doc = QJsonDocument::fromJson(reply->readAll());
            qDebug() << json_doc;
            if (json_doc["is_login"].toBool()) {
                stack_widget->setCurrentIndex(kNormalUserPage);
                UserSingleton::Instance()->SetId(json_doc["user_id"].toInt());
            } else {
                QMessageBox box{p};
                box.setText("登录失败");
                box.exec();
            }
            reply->deleteLater();
        });
    auto user = UserSingleton::Instance();
    user->SetLoginName(ui_.user_login_account_line_edit->text().toStdString());
    user->SetType(UserType::kBookReader);
    ui_.user_login_name_show_label->setText(user->GetLoginName().c_str());
    ShowBorrowList(ui_.user_borrow_table_widget, true);
}

void LMSMainWindow::on_user_login_administrator_btn_clicked() {
    auto reply = UserLogin(false);
    connect(reply, &QNetworkReply::readyRead, this,
        [reply = reply, stack_widget = ui_.stacked_widget, p = this]() {
            auto json_doc = QJsonDocument::fromJson(reply->readAll());
            if (json_doc["is_login"].toBool()) {
                stack_widget->setCurrentIndex(kAdministratorPage);
                UserSingleton::Instance()->SetId(json_doc["user_id"].toInt());
            } else {
                QMessageBox box{p};
                box.setText("登录失败");
                box.exec();
            }
            reply->deleteLater();
        });
    auto user = UserSingleton::Instance();
    user->SetLoginName(ui_.user_login_account_line_edit->text().toStdString());
    user->SetType(UserType::kManager);
}

void LMSMainWindow::on_user_sign_up_yse_btn_clicked() {
    std::regex re{R"cpp(^\S{1,20}$)cpp"};
    QMessageBox *box = new QMessageBox(this);
    if (!std::regex_match(ui_.user_sign_up_account_line_edit->text().toStdString(),
         re)) {
        box->setText("请输入1~20个非空白字符作为登录名");
        box->exec();
        return;
    }
    re = R"cpp(^\S{6,}$)cpp";
    if (!std::regex_match(ui_.user_sign_up_pwd_line_edit->text().toStdString(),
         re)) {
        box->setText("请输入至少6个非空白字符作为密码");
        box->exec();
        return;
    }
    if (!(ui_.user_sign_up_pwd_line_edit->text() ==
        ui_.user_sign_up_re_pwd_edit->text())) {
        box->setText("请输入相同的密码");
        box->exec();
        return;
    }
    box->deleteLater();
    QJsonDocument json_doc;
    cjs::openssl::Hash h{ cjs::openssl::HashType::kSHA256Type };
    h.Update(ui_.user_sign_up_pwd_line_edit->text().toStdString());
    json_doc.setObject(
        {
            {"action", "insert"},
            {"user_login_name", ui_.user_sign_up_account_line_edit->text()},
            {"user_name", ui_.user_sign_up_name_edit->text()},
            {"user_pwd", h.Final().c_str()},
            {"user_tel", ui_.user_sign_up_tel_edit->text()}
        }
    );
    auto reply = SendPost(kUserURL, json_doc);
    connect(reply, &QNetworkReply::readyRead,
        this, [reply = reply]() {
            QJsonDocument json_doc = QJsonDocument::fromJson(reply->readAll());
            QMessageBox box;
            if (json_doc["is_inserted"].toBool()) {
                box.setText("注册成功");
            } else {
                box.setText("注册失败");
            }
            box.exec();
            reply->deleteLater();
        });
}

void LMSMainWindow::on_administrator_borrow_sign_btn_clicked() {
    BorrowDialog dialog{ true, this };
    dialog.exec();
    if (dialog.result()) {
        auto info = dialog.GetBorrowInfo();
        QJsonDocument json_doc;
        json_doc.setObject(
            {
                {"action", "borrow"},
                {"book_class_id", info.book_id},
                {"reader_id", info.user_id},
                {"borrow_deadline", info.borrow_deadline.c_str()}
            }
        );
        auto reply = SendPost(kBorrowURL, json_doc);
        connect(reply, &QNetworkReply::readyRead, this,
            [reply = reply, p = this, table = ui_.administrator_borrow_table_widget]() {
                auto json_doc = QJsonDocument::fromJson(reply->readAll());
                QMessageBox box{p};
                if (json_doc["is_borrowed"].toBool()) {
                    box.setText("借阅登记成功");
                    p->ShowBorrowList(table, false);
                } else {
                    box.setText("借阅登记失败");
                }
                box.exec();
                reply->deleteLater();
            });
    }
}

// 下列为翻页

void LMSMainWindow::on_administrator_book_pre_btn_clicked() {
    ::PagePreImpl(&s_a_book_page_num_, ui_.administrator_book_pre_btn, ui_.administrator_book_next_btn);
    InitPageLabel();
    ShowBookList(ui_.administrator_book_table_widget, false);
}

void LMSMainWindow::on_administrator_book_next_btn_clicked() {
    ::PageNextImpl(&s_a_book_page_num_, ui_.administrator_book_pre_btn, ui_.administrator_book_next_btn, ui_.administrator_book_table_widget);
    InitPageLabel();
    ShowBookList(ui_.administrator_book_table_widget, false);
}

void LMSMainWindow::on_administrator_user_pre_btn_clicked() {
    ::PagePreImpl(&s_a_user_page_num_, ui_.administrator_user_pre_btn, ui_.administrator_user_next_btn);
    InitPageLabel();
    ShowUserList(ui_.administrator_user_table_widget);
}

void LMSMainWindow::on_administrator_user_next_btn_clicked() {
    ::PageNextImpl(&s_a_user_page_num_, ui_.administrator_user_pre_btn, ui_.administrator_user_next_btn, ui_.administrator_user_table_widget);
    InitPageLabel();
    ShowUserList(ui_.administrator_user_table_widget);
}

void LMSMainWindow::on_administrator_borrow_pre_btn_clicked() {
    ::PagePreImpl(&s_a_borrow_page_num_, ui_.administrator_borrow_pre_btn, ui_.administrator_borrow_next_btn);
    InitPageLabel();
    ShowBorrowList(ui_.administrator_borrow_table_widget, false);
}

void LMSMainWindow::on_administrator_borrow_next_btn_clicked() {
    ::PageNextImpl(&s_a_borrow_page_num_, ui_.administrator_borrow_pre_btn, ui_.administrator_borrow_next_btn, ui_.administrator_borrow_table_widget);
    InitPageLabel();
    ShowBorrowList(ui_.administrator_borrow_table_widget, false);
}

void LMSMainWindow::on_user_pre_btn_clicked() {
    ::PagePreImpl(&s_n_borrow_page_num_, ui_.user_pre_btn, ui_.user_next_btn);
    InitPageLabel();
    ShowBorrowList(ui_.user_borrow_table_widget, true);
}

void LMSMainWindow::on_user_next_btn_clicked() {
    ::PageNextImpl(&s_n_borrow_page_num_, ui_.user_pre_btn, ui_.user_next_btn, ui_.user_borrow_table_widget);
    InitPageLabel();
    ShowBorrowList(ui_.user_borrow_table_widget, true);
}

void LMSMainWindow::on_search_pre_btn_clicked() {
    ::PagePreImpl(&s_n_book_page_num_, ui_.search_pre_btn, ui_.search_next_btn);
    InitPageLabel();
    ShowBookList(ui_.search_book_table_widget, true);
}

void LMSMainWindow::on_search_next_btn_clicked() {
    ::PageNextImpl(&s_n_book_page_num_, ui_.search_pre_btn, ui_.search_next_btn, ui_.search_book_table_widget);
    InitPageLabel();
    ShowBookList(ui_.search_book_table_widget, true);
}

// 工作按钮
// 管理员书籍搜索
void LMSMainWindow::on_administrator_book_do_search_btn_clicked() {
    ShowBookList(ui_.administrator_book_table_widget, false);
}
// 管理员添加书籍
void LMSMainWindow::on_administrator_book_btn_insert_btn_clicked() {
    BookDialog dialog{true, this};
    dialog.exec();
    if (dialog.result()) {
        QJsonDocument json_doc;
        auto info = dialog.GetBookInfo();
        json_doc.setObject(
            {
                {"action", "insert"},
                {"book_type", info.type},
                {"book_name", info.name.c_str()},
                {"book_auther", info.auther.c_str()},
                {"book_press", info.press.c_str()},
                {"book_price", info.price},
                {"book_num", info.num}
            }
        );
        auto reply = SendPost(kBookURL, json_doc);
        connect(reply, &QNetworkReply::readyRead,
            [reply = reply, p = this]() {
                auto json_doc = QJsonDocument::fromJson(reply->readAll());
                qDebug() << json_doc;
                QMessageBox box{p};
                if (json_doc["is_inserted"].toBool()) {
                    box.setText("添加成功");
                } else {
                    box.setText("添加失败");
                }
                box.exec();
                reply->deleteLater();
            });
    }
}
// 书籍搜索
void LMSMainWindow::on_search_do_search_btn_clicked() {
    ShowBookList(ui_.search_book_table_widget, true);
}
// 用户搜索
void LMSMainWindow::on_administrator_user_do_search_btn_clicked() {
    ShowUserList(ui_.administrator_user_table_widget);
}
// 借阅搜索
void LMSMainWindow::on_administrator_borrow_do_search_btn_clicked() {
    ShowBorrowList(ui_.administrator_borrow_table_widget, false);
}

BookInfo LMSMainWindow::BookAt(int row) const {
    BookInfo info;
    qDebug() << ui_.administrator_book_table_widget->item(row, 0);
    info.id = ui_.administrator_book_table_widget->item(row, 0)->text().toInt();
    QComboBox* combo_box = dynamic_cast<QComboBox*>(
        ui_.administrator_book_table_widget->cellWidget(row, 1));
    info.type = static_cast<BookType>(combo_box->currentIndex());
    info.name = ui_.administrator_book_table_widget->item(row, 2)->text().toStdString();
    info.auther = ui_.administrator_book_table_widget->item(row, 3)->text().toStdString();
    info.press = ui_.administrator_book_table_widget->item(row, 4)->text().toStdString();
    info.price = ui_.administrator_book_table_widget->item(row, 5)->text().toDouble();
    info.num = ui_.administrator_book_table_widget->item(row, 6)->text().toInt();
    return info;
}

UserInfo LMSMainWindow::UserAt(int row) const {
    UserInfo info;
    info.id = ui_.administrator_user_table_widget->item(row, 0)->text().toInt();
    info.login_name = ui_.administrator_user_table_widget->item(row, 1)->text().toStdString();
    info.name = ui_.administrator_user_table_widget->item(row, 2)->text().toStdString();
    info.tel = ui_.administrator_user_table_widget->item(row, 3)->text().toStdString();
    return info;
}

lms::BorrowInfo LMSMainWindow::BorrowAt(bool is_reader, int row) const {
    lms::BorrowInfo info;
    if (is_reader) {
        info.book_id = ui_.user_borrow_table_widget->item(row, 0)->text().toInt();
        info.book_name = ui_.user_borrow_table_widget->item(row, 1)->text().toStdString();
        info.user_login_name = UserSingleton::Instance()->GetLoginName();
        info.borrow_deadline = dynamic_cast<QDateEdit*>(
            ui_.user_borrow_table_widget->cellWidget(row, 2)
            )->date().toString(kDateFormat).toStdString();
        info.price = ui_.user_borrow_table_widget->item(row, 3)->text().toDouble();
    } else {
        info.book_id = ui_.administrator_borrow_table_widget->item(row, 0)->text().toInt();
        info.book_name = ui_.administrator_borrow_table_widget->item(row, 1)->text().toStdString();
        info.user_id = ui_.administrator_borrow_table_widget->item(row, 2)->text().toInt();
        info.user_login_name = ui_.administrator_borrow_table_widget->item(row, 3)->text().toStdString();
        info.borrow_date = dynamic_cast<QDateEdit*>(
                ui_.administrator_borrow_table_widget->cellWidget(row, 4)
            )->date().toString(kDateFormat).toStdString();
        info.borrow_deadline = dynamic_cast<QDateEdit*>(
                ui_.administrator_borrow_table_widget->cellWidget(row, 5)
            )->date().toString(kDateFormat).toStdString();
        info.price = ui_.administrator_borrow_table_widget->item(row, 6)->text().toDouble();
    }
    return info;
}

QNetworkReply* LMSMainWindow::SendPost(std::string_view url_type,
    const QJsonDocument& json_doc) {
    qDebug() << json_doc;
    QString url = QString(kHttpInitStr).arg(kHostName).arg(url_type.data());
    QNetworkRequest request{ url };
    request.setHeader(QNetworkRequest::ContentTypeHeader,
        QVariant("application/json"));
    QNetworkReply* reply = net_manager_->post(request, json_doc.toJson());
    return reply;
}

void LMSMainWindow::ShowBookList(QTableWidget* table_widget, bool is_reader) {
    QJsonDocument json_request_doc;
    if (is_reader) {
        // 检查数据是否合法
        if ((ui_.search_text_edit->toPlainText() != "") &&
            !(ui_.search_auther_check_box->isChecked() ||
                ui_.search_id_check_box->isChecked() ||
                ui_.search_name_check_box->isChecked())) {
            QMessageBox msg{ this };
            msg.setText("若要输入搜索内容则必须勾选匹配方式");
            msg.exec();
            return;
        }
        json_request_doc.setObject({
                {"action", "search"},
                {"is_book_id", ui_.search_id_check_box->isChecked()},
                {"is_book_name", ui_.search_name_check_box->isChecked()},
                {"is_auther", ui_.search_auther_check_box->isChecked()},
                {"text", ui_.search_text_edit->toPlainText()},
                {"type", ui_.search_type_combo_box->currentIndex()},
                {"page", s_n_book_page_num_}
            }
        );
    }
    else {
        if ((ui_.administrator_book_text_edit->toPlainText() != "") &&
            !(ui_.administrator_book_auther_check_box->isChecked() ||
                ui_.administrator_book_id_check_box->isChecked() ||
                ui_.administrator_book_name_check_box->isChecked())) {
            QMessageBox msg{ this };
            msg.setText("若要输入搜索内容则必须勾选匹配方式");
            msg.exec();
            return;
        }
        json_request_doc.setObject({
                {"action", "search"},
                {"is_book_id", ui_.administrator_book_id_check_box->isChecked()},
                {"is_book_name", ui_.administrator_book_name_check_box->isChecked()},
                {"is_auther", ui_.administrator_book_auther_check_box->isChecked()},
                {"text", ui_.administrator_book_text_edit->toPlainText()},
                {"type", ui_.administrator_book_type_combo_box->currentIndex()},
                {"page", s_a_book_page_num_}
            }
        );
    }
    auto reply = SendPost(kBookURL, json_request_doc);
    connect(reply, &QNetworkReply::readyRead, this,
        [reply = reply, table = table_widget, p = this, flag = is_reader]() {
            emit p->ShowBookListReplyCBSignal(reply, table, p, flag);
        });
}

void LMSMainWindow::ShowUserList(QTableWidget* table) {
    // 发出请求
    if ((ui_.administrator_user_text_edit->toPlainText() != "") &&
        !(ui_.administrator_user_login_name_check_box->isChecked() ||
            ui_.administrator_user_name_check_box->isChecked() ||
            ui_.administrator_user_tel_check_box->isChecked())) {
        QMessageBox msg{ this };
        msg.setText("若要输入搜索内容则必须勾选匹配方式");
        msg.exec();
        return;
    }
    QJsonDocument json_doc;
    json_doc.setObject({
            {"action", "search"},
            {"is_reader_login_name", ui_.administrator_user_login_name_check_box->isChecked()},
            {"is_reader_name", ui_.administrator_user_name_check_box->isChecked()},
            {"is_reader_tel", ui_.administrator_user_tel_check_box->isChecked()},
            {"text", ui_.administrator_user_text_edit->toPlainText()},
            {"page", s_a_user_page_num_}
        }
    ); 
    auto reply = SendPost(kUserURL, json_doc);
    connect(reply, &QNetworkReply::readyRead, this,
        [reply = reply, table = table, p = this]() {
            emit p->ShowUserListReplyCBSignal(reply, table, p);
        });
}

void LMSMainWindow::ShowBorrowList(QTableWidget* table_widget, bool is_reader) {
    if ((ui_.administrator_borrow_text_edit->toPlainText() != "") &&
        !(ui_.administrator_borrow_user_login_name_check_box->isChecked() ||
            ui_.administrator_borrow_book_name_check_box->isChecked())) {
        QMessageBox msg{ this };
        msg.setText("若要输入搜索内容则必须勾选匹配方式");
        msg.exec();
        return;
    }
    // 发出请求
    QJsonDocument json_doc;
    if (is_reader) {
        json_doc.setObject(
            {
                {"action", "search"},
                {"is_reader_login_name", false},
                {"is_reader_name", false},
                {"is_book_name", false},
                {"text", ""},
                {"page", s_n_borrow_page_num_},
                {"is_reader", is_reader}
            }
        );
    } else {
        json_doc.setObject(
            {
                {"action", "search"},
                {"is_reader_login_name", ui_.administrator_borrow_user_login_name_check_box->isChecked()},
                {"is_book_name", ui_.administrator_borrow_book_name_check_box->isChecked()},
                {"text", ui_.administrator_user_text_edit->toPlainText()},
                {"page", s_a_borrow_page_num_},
                {"is_reader", is_reader}
            }
        );
    }
    auto reply = SendPost(kBorrowURL, json_doc);
    connect(reply, &QNetworkReply::readyRead, this,
        [reply = reply, table = table_widget, p = this, flag = is_reader]() {
            emit p->ShowBorrowListReplyCBSignal(reply, table, p, flag);
        });
}

void LMSMainWindow::SetNextBtnDisable(const PageType& page) {
    switch (page) {
    case PageType::kAdministratorBookPage:
        ui_.administrator_book_next_btn->setDisabled(true);
        break;
    case PageType::kAdministratorUserPage:
        ui_.administrator_user_next_btn->setDisabled(true);
        break;
    case PageType::kNormalBookSearchPage:
        ui_.search_next_btn->setDisabled(true);
        break;
    case PageType::kAdministratorBorrowPage:
        ui_.administrator_borrow_next_btn->setDisabled(true);
        break;
    case PageType::kNormalUserPage:
        ui_.user_next_btn->setDisabled(true);
        break;
    }
}

// 私有工作函数

QNetworkReply* LMSMainWindow::UserLogin(bool is_reader) {
    QJsonDocument json_doc;
    cjs::openssl::Hash h{ cjs::openssl::HashType::kSHA256Type };
    h.Update(ui_.user_login_pwd_line_edit->text());
    json_doc.setObject(
        {
            {"action", "login"},
            {"is_reader", is_reader},
            {"user_login_name", ui_.user_login_account_line_edit->text()},
            {"user_pwd", h.Final().c_str()}
        }
    );
    auto reply = SendPost(kUserURL, json_doc);
    return reply;
}

void LMSMainWindow::InitPageLabel() {
    ui_.administrator_book_page_num_lable->setText(
        QString("第") + QString::number(s_a_book_page_num_) + QString("页")
    );
    ui_.administrator_user_page_num_lable->setText(
        QString("第") + QString::number(s_a_user_page_num_) + QString("页")
    );
    ui_.administrator_borrow_page_num_lable->setText(
        QString("第") + QString::number(s_a_borrow_page_num_) + QString("页")
    );
    ui_.search_page_num_lable->setText(
        QString("第") + QString::number(s_n_book_page_num_) + QString("页")
    );
    ui_.user_page_num_lable->setText(
        QString("第") + QString::number(s_n_borrow_page_num_) + QString("页")
    );
}

}
