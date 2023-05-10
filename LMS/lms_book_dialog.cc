#include "lms_book_dialog.h"
#include <regex>
#include <qmessagebox.h>

namespace lms {
 
BookDialog::BookDialog(QWidget* parent)
        : QDialog(parent) {
    ui_.setupUi(this);
}

BookDialog::BookDialog(const bool is_insert, QWidget* parent)
        : BookDialog(parent){
    SetDialogType(is_insert);
}

BookDialog::~BookDialog() {
    qDebug() << "BookDialog 析构";
}

void BookDialog::SetDialogType(const bool is_insert) {
    is_insert_ = is_insert;
    if (is_insert_) {
        ui_.ok_btn->setText("添加");
    } else {
        ui_.ok_btn->setText("编辑");
    }
}

void BookDialog::SetBookInfo(const BookInfo& info) {
    ui_.name_edit->setText(info.name.c_str());
    ui_.type_combo_box->setCurrentIndex(info.type);
    ui_.auther_edit->setText(info.auther.c_str());
    ui_.press_edit->setText(info.press.c_str());
    ui_.price_edit->setText(QString::number(info.price));
    ui_.num_edit->setText(QString::number(info.num));
}

BookInfo BookDialog::GetBookInfo() const {
    BookInfo info;
    info.auther = ui_.auther_edit->text().toStdString();
    info.name = ui_.name_edit->text().toStdString();
    info.press = ui_.press_edit->text().toStdString();
    info.type = static_cast<BookType>(ui_.type_combo_box->currentIndex() + 1);
    info.price = ui_.price_edit->text().toDouble();
    info.num = ui_.num_edit->text().toInt();
    return info;
}

void BookDialog::accept() {
    // 检查数据合法
    std::regex re{R"cpp(^\S{1,20}$)cpp"};
    QMessageBox *box = new QMessageBox(this);
    if (!std::regex_match(ui_.name_edit->text().toStdString(), re)) {
        box->setText("请输入1~20个非空白字符作为书名");
        box->exec();
        return;
    }
    if (!std::regex_match(ui_.auther_edit->text().toStdString(), re)) {
        box->setText("请输入1~20个非空白字符作为作者");
        box->exec();
        return; 
    }
    re = R"cpp(^\S{1,50}$)cpp";
    if (!std::regex_match(ui_.press_edit->text().toStdString(), re)) {
        box->setText("请输入1~50个非空白字符作为出版社");
        box->exec();
        return;
    }
    re = R"cpp(^\d+(\.?\d+)?$)cpp";
    if (!std::regex_match(ui_.price_edit->text().toStdString(), re)) {
        box->setText("请输入正确数字作为价格");
        box->exec();
        return;
    }
    bool ok;
    ui_.num_edit->text().toUInt(&ok);
    if (!ok) {
        box->setText("请输入非负整数作为当前数量");
        box->exec();
        return;
    }
    box->deleteLater();
    QDialog::accept();
}


}
