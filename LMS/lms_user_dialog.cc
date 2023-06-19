#include "lms_user_dialog.h"
#include <regex>
#include <qmessagebox.h>

#include "cjs//mine_hash.hpp"

namespace lms {

UserDialog::UserDialog(QWidget* parent)
        : QDialog(parent) {
    ui_.setupUi(this);
}

UserDialog::~UserDialog() {
    qDebug() << "UserDialog 析构";
}

void UserDialog::accept() {
    std::regex re{R"cpp(^\S{1,20}$)cpp"};
    QMessageBox *box = new QMessageBox(this);
    if (!std::regex_match(ui_.login_name_edit->text().toStdString(), re)) {
        box->setText("请输入1~20个非空白字符作为登录名");
        box->exec();
        return;
    }
    re = R"cpp(^\S{1,10}$)cpp";
    if (!std::regex_match(ui_.name_edit->text().toStdString(), re)) {
        box->setText("请输入1~10个非空白字符作为姓名");
        box->exec();
        return;
    }
    re = R"cpp(^(13[0-9]|14[01456879]|15[0-35-9]|16[2567]|17[0-8]|18[0-9]|19[0-35-9])\d{8}$)cpp";
    if (!std::regex_match(ui_.tel_edit->text().toStdString(), re)) {
        box->setText("请输入正确的电话号码");
        box->exec();
        return;
    }
    re = R"cpp(^\S{6,}$)cpp";
    if (!std::regex_match(ui_.pwd_edit->text().toStdString(),
        re)) {
        box->setText("请输入至少6个非空白字符作为密码");
        box->exec();
        return;
    }
    box->deleteLater();
    QDialog::accept();
}

void UserDialog::SetUserInfo(const UserInfo& info) {
    ui_.id_edit->setText(QString::number(info.id));
    ui_.login_name_edit->setText(info.login_name.c_str());
    ui_.name_edit->setText(info.name.c_str());
    ui_.tel_edit->setText(info.tel.c_str());
}

UserInfo UserDialog::GetUserInfo() const {
    using namespace cjs::openssl;
    UserInfo info;
    Hash h{HashType::kSHA256Type};
    h.Update(ui_.pwd_edit->text().toStdString());
    info.login_name = ui_.login_name_edit->text().toStdString();
    info.name = ui_.name_edit->text().toStdString();
    info.pwd = h.Final();
    info.tel = ui_.tel_edit->text().toStdString();
    info.id = ui_.id_edit->text().toInt();
    return info;
}

}
