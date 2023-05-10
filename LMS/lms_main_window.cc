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

namespace lms {

LMSMainWindow::LMSMainWindow(QWidget *parent)
        : QMainWindow(parent) {
    ui_.setupUi(this);
}

LMSMainWindow::~LMSMainWindow() {
    qDebug() << "LMSMainWindow 析构";
}

void LMSMainWindow::on_menu_search_book_btn_clicked() {
    ui_.stackedWidget->setCurrentIndex(PageIndex::kNormalBookSearchPage);
    ShowBookList(ui_.search_book_table_widget);
    ui_.search_page_num_lable->setText(
        QString("第") + QString::number(s_n_book_page_num_) + QString("页"));
}

void LMSMainWindow::on_search_back_btn_clicked() {
    ui_.stackedWidget->setCurrentIndex(PageIndex::kMenuPage);
}

void LMSMainWindow::on_menu_sign_in_btn_clicked() {
    ui_.stackedWidget->setCurrentIndex(PageIndex::kUserSignInPage);
}

void LMSMainWindow::on_user_login_sign_up_btn_clicked() {
    ui_.stackedWidget->setCurrentIndex(PageIndex::kUserSignUpPage);
}

void LMSMainWindow::on_user_login_back_btn_clicked() {
    ui_.stackedWidget->setCurrentIndex(PageIndex::kMenuPage);
}

void LMSMainWindow::on_user_sign_up_back_btn_clicked() {
    ui_.stackedWidget->setCurrentIndex(PageIndex::kMenuPage);
}

void LMSMainWindow::on_administrator_book_btn_clicked() {
    ui_.administrator_stack_widget->setCurrentIndex(PageIndex::kAdministratorBookPage);
    ShowBookList(ui_.administrator_book_table_widget);
    int row = ui_.administrator_book_table_widget->rowCount() - 1;
    QPushButton *btn = nullptr;
    for (; row != -1; --row) {
        btn = new QPushButton{"编辑", this};
        ui_.administrator_book_table_widget->setCellWidget(
            row, kAdministratorBookTableEditColumn, btn);
        BookInfo book_info = BookAt(row);
        connect(btn, &QPushButton::clicked, this,
            [row = row, table = ui_.administrator_book_table_widget,
             t = this, info = book_info]() {
                BookDialog dialog{false, t};
                dialog.SetBookInfo(info);
                dialog.exec();
                if (dialog.result()) {
                    qDebug() << dialog.GetBookInfo().name.c_str();
                }
            });
        btn = new QPushButton{"删除", this};
        ui_.administrator_book_table_widget->setCellWidget(
            row, kAdministratorBookTableDelColumn, btn);
        connect(btn, &QPushButton::clicked, this,
            [row = row, table = ui_.administrator_book_table_widget]() {
            });
    }
    ui_.administrator_book_page_num_lable->setText(
        QString("第") + QString::number(s_a_book_page_num_) + QString("页"));
}

void LMSMainWindow::on_administrator_user_btn_clicked() {
    ui_.administrator_stack_widget->setCurrentIndex(PageIndex::kAdministratorUserPage);
    ShowUserList();
    ui_.administrator_user_page_num_lable->setText(
        QString("第") + QString::number(s_a_user_page_num_) + QString("页"));
}

void LMSMainWindow::on_administrator_borrow_or_return_btn_clicked() {
    ui_.administrator_stack_widget->setCurrentIndex(PageIndex::kAdministratorBorrowPage);
    ShowBorrowList(false, ui_.administrator_borrow_table_widget);
    ui_.administrator_borrow_page_num_lable->setText(
        QString("第") + QString::number(s_a_borrow_page_num_) + QString("页"));
}

void LMSMainWindow::on_administrator_back_btn_clicked() {
    ui_.stackedWidget->setCurrentIndex(PageIndex::kMenuPage);
}

void LMSMainWindow::on_user_back_btn_clicked() {
    ui_.stackedWidget->setCurrentIndex(PageIndex::kMenuPage);
}

void LMSMainWindow::on_user_login_normal_btn_clicked() {
    auto reply = UserLogin(true);
    connect(reply, &QNetworkReply::readyRead, this,
        [reply = reply, stack_widget = ui_.stackedWidget, p = this]() {
            auto json_doc = QJsonDocument::fromJson(reply->readAll());
            qDebug() << json_doc;
            if (json_doc["is_login"].toBool()) {
                stack_widget->setCurrentIndex(kNormalUserPage);
                qDebug() << "登录成功";
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
    ShowBorrowList(true, ui_.user_borrow_table_widget);
}

void LMSMainWindow::on_user_login_administrator_btn_clicked() {
    /*auto reply = UserLogin(false);
    connect(reply, &QNetworkReply::readyRead, this,
        [reply = reply, stack_widget = ui_.stackedWidget]() {
            auto json_doc = QJsonDocument::fromJson(reply->readAll());
            if (json_doc["is_login"].toBool()) {
                stack_widget->setCurrentIndex(kAdministratorPage);
            } else {
                QMessageBox box;
                box.setText("登录失败");
            }
        });*/
    auto user = UserSingleton::Instance();
    user->SetLoginName(ui_.user_login_account_line_edit->text().toStdString());
    user->SetType(UserType::kManager);
    ui_.stackedWidget->setCurrentIndex(PageIndex::kAdministratorPage);
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
    h.Update(ui_.user_sign_up_account_line_edit->text().toStdString());
    QJsonObject json_ob;
    json_ob.insert("action", "insert");
    json_ob.insert("user_login_name",
        ui_.user_sign_up_account_line_edit->text());
    json_ob.insert("user_pwd", h.Final().c_str());
    json_ob.insert("user_tel", "");
    json_doc.setObject(json_ob);
    auto reply = SendPost("text", json_doc);
    connect(reply, &QNetworkReply::readyRead,
        this, [reply = reply]() {
            qDebug() << reply->readAll();
            reply->deleteLater();
        });
}

void LMSMainWindow::on_administrator_borrow_sign_btn_clicked() {
    BorrowDialog dialog{ true, this };
    dialog.exec();
}

void LMSMainWindow::on_administrator_book_pre_btn_clicked() {

}

void LMSMainWindow::on_administrator_book_next_btn_clicked() {

}

void LMSMainWindow::on_administrator_user_pre_btn_clicked() {

}

void LMSMainWindow::on_administrator_user_next_btn_clicked() {

}

void LMSMainWindow::on_administrator_borrow_pre_btn_clicked() {

}

void LMSMainWindow::on_administrator_borrow_next_btn_clicked() {

}

QNetworkReply* LMSMainWindow::UserLogin(bool is_reader) {
    QJsonDocument json_doc;
    cjs::openssl::Hash h{cjs::openssl::HashType::kSHA256Type};
    h.Update(ui_.user_login_pwd_line_edit->text());
    json_doc.setObject(
        {
            {"action", "login"},
            {"is_reader", is_reader},
            {"user_login_name", ui_.user_login_account_line_edit->text()},
            {"user_pwd", h.Final().c_str()}
        }
    );
    qDebug() << json_doc;
    auto reply = SendPost("user_action", json_doc);
    //auto reply = SendPost("text", json_doc);
    return reply;
}

BookInfo LMSMainWindow::BookAt(int row) const {
    BookInfo info;
    qDebug() << ui_.administrator_book_table_widget->item(row, 0);
    info.id = ui_.administrator_book_table_widget->item(row, 0)->text().toInt();
    QComboBox * combo_box = dynamic_cast<QComboBox*>(
        ui_.administrator_book_table_widget->cellWidget(row, 1));
    info.type = static_cast<BookType>(combo_box->currentIndex() + 1);
    info.name = ui_.administrator_book_table_widget->item(row, 2)->text().toStdString();
    info.auther = ui_.administrator_book_table_widget->item(row, 3)->text().toStdString();
    info.press = ui_.administrator_book_table_widget->item(row, 4)->text().toStdString();
    info.price =ui_.administrator_book_table_widget->item(row, 5)->text().toDouble();
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


void LMSMainWindow::ShowBookList(QTableWidget *table_widget) {
    // 发出请求
    QFile file{R"cpp(for_text/book_list.json)cpp"};
    file.open(QIODevice::ReadOnly);
    QJsonDocument json_doc = QJsonDocument::fromJson(file.readAll());
    file.close();
    QJsonArray jarr = json_doc.array();
    table_widget->setRowCount(jarr.size());
    for (int i = 0; auto value : jarr) {
        int j = 0;
        QJsonObject book = value.toObject();
        table_widget->setItem(i, j, 
            new QTableWidgetItem{QString::number(book["book_id"].toInt())});
        ++j;
        QComboBox *combo_box = new QComboBox(this);
        combo_box->addItem("教育");
        combo_box->addItem("童话");
        combo_box->addItem("文学");
        combo_box->addItem("科幻");
        combo_box->setCurrentIndex(book["book_type"].toInt() - 1);
        combo_box->setDisabled(true);
        table_widget->setCellWidget(i, j, combo_box);
        ++j;
        table_widget->setItem(i, j, 
            new QTableWidgetItem{book["book_name"].toString()});
        ++j;
        table_widget->setItem(i, j, 
            new QTableWidgetItem{book["book_auther"].toString()});
        ++j;
        table_widget->setItem(i, j, 
            new QTableWidgetItem{book["book_press"].toString()});
        ++j;
        table_widget->setItem(i, j, 
            new QTableWidgetItem{QString::number(book["book_price"].toDouble())});
        ++j;
        table_widget->setItem(i, j, 
            new QTableWidgetItem{QString::number(book["book_num"].toInt())});
        ++i;
    }
}

void LMSMainWindow::ShowUserList() {
    // 发出请求
    QFile file{ R"cpp(for_text/user_list.json)cpp" };
    file.open(QIODevice::ReadOnly);
    QJsonDocument json_doc = QJsonDocument::fromJson(file.readAll());
    file.close();
    QJsonArray jarr = json_doc.array();
    QPushButton *btn = nullptr;
    ui_.administrator_user_table_widget->setRowCount(jarr.size());
    for (int i = 0; auto value : jarr) {
        int j = 0;
        QJsonObject user = value.toObject();
        ui_.administrator_user_table_widget->setItem(i, j,
            new QTableWidgetItem{QString::number(user["reader_id"].toInt())});
        ++ j;
        ui_.administrator_user_table_widget->setItem(i, j,
            new QTableWidgetItem{user["reader_login_name"].toString()});
        ++ j;
        ui_.administrator_user_table_widget->setItem(i, j,
            new QTableWidgetItem{user["reader_name"].toString()});
        ++ j;
        ui_.administrator_user_table_widget->setItem(i, j,
            new QTableWidgetItem{user["reader_tel"].toString()});
        btn = new QPushButton{ "编辑", this };
        ui_.administrator_user_table_widget->setCellWidget(
            i, kAdministratorUserTableEditColumn, btn);
        connect(btn, &QPushButton::clicked, this,
            [row = i, table = ui_.administrator_user_table_widget]() {
            });
        btn = new QPushButton{ "删除", this };
        ui_.administrator_user_table_widget->setCellWidget(
            i, kAdministratorUserTableDelColumn, btn);
        connect(btn, &QPushButton::clicked, this,
            [row = i, table = ui_.administrator_user_table_widget]() {
            });
        ++i;
    }
}

void LMSMainWindow::ShowBorrowList(bool is_reader, QTableWidget* table_widget) {
    QFile file{ R"cpp(for_text/borrow_list.json)cpp" };
    file.open(QIODevice::ReadOnly);
    QJsonDocument json_doc = QJsonDocument::fromJson(file.readAll());
    file.close();
    QJsonArray jarr = json_doc.array();
    table_widget->setRowCount(jarr.size());
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
            time_edit = new QDateEdit(this);
            time_edit->setDate(QDate::fromString(item["borrow_deadline"].toString(), kDateFormat));
            time_edit->setDisabled(true);
            table_widget->setCellWidget(i, j, time_edit);
            ++j;
        }
        time_edit = new QDateEdit(this);
        time_edit->setDate(QDate::fromString(item["borrow_deadline"].toString(), kDateFormat));
        time_edit->setDisabled(true);
        table_widget->setCellWidget(i, j, time_edit);
        ++j;
        table_widget->setItem(i, j,
            new QTableWidgetItem{ QString::number(item["book_price"].toDouble()) });
        if (!is_reader) { // 用于管理员页面
            btn = new QPushButton{ "还书", this };
            table_widget->setCellWidget(
                i, kAdministratorBorrowTableReturnColumn, btn);
            connect(btn, &QPushButton::clicked, this,
                [row = i, table = table_widget]() {
                });
            btn = new QPushButton{ "异常", this };
            table_widget->setCellWidget(
                i, kAdministratorBorrowTableErrorColumn, btn);
            connect(btn, &QPushButton::clicked, this,
                [row = i, table = table_widget]() {
                });
        }
        else {
            btn = new QPushButton{ "还书", this };
            table_widget->setCellWidget(
                i, kUserBorrowTableReturnColumn, btn);
            connect(btn, &QPushButton::clicked, this,
                [row = i, table = table_widget]() {
                });
            btn = new QPushButton{ "还款", this };
            table_widget->setCellWidget(
                i, kUserBorrowTablePayColumn, btn);
            connect(btn, &QPushButton::clicked, this,
                [row = i, table = table_widget]() {
                });
        }
        ++i;
    }
}

QNetworkReply* LMSMainWindow::SendPost(std::string_view url_type,
        const QJsonDocument &json_doc) {
    QString url = QString(kHttpInitStr).arg(kHostName).arg(url_type.data());
    QNetworkRequest request{url};
    request.setHeader(QNetworkRequest::ContentTypeHeader,
        QVariant("application/json"));
    QNetworkReply* reply = net_manager_->post(request, json_doc.toJson());
    return reply;
}

}
