#include "lms_borrow_dialog.h"
#include <regex>
#include <qmessagebox.h>

namespace lms {

BorrowDialog::BorrowDialog(bool is_add, QWidget* parent)
        : QDialog(parent),
        flag_{is_add} {
    ui_.setupUi(this);
    if (flag_) {
        ui_.stack_page->setCurrentIndex(0);
    } else {
        ui_.stack_page->setCurrentIndex(1);
    }
}

BorrowDialog::~BorrowDialog() {
    qDebug() << "BorrowDialog 析构";
}

void BorrowDialog::SetBorrowInfo(const BorrowInfo& info) {
    if (flag_) {
        ui_.add_book_id_edit->setText(QString::number(info.book_id));
        ui_.add_user_id_edit->setText(QString::number(info.user_id));
    } else {
        ui_.err_book_id_edit->setText(QString::number(info.book_id));
        ui_.err_book_name_edit->setText(info.book_name.c_str());
        ui_.err_user_id_edit->setText(QString::number(info.user_id));
        ui_.err_user_login_name_edit->setText(info.user_login_name.c_str());
        ui_.err_borrow_date_edit->setDate(QDate::fromString(info.borrow_date.c_str(), kDateFormat));
        ui_.err_borrow_deadline_edit->setDate(QDate::fromString(info.borrow_deadline.c_str(), kDateFormat));
        ui_.err_price_edit->setText(QString::number(info.price));
    }
}

BorrowInfo BorrowDialog::GetBorrowInfo() const {
    BorrowInfo info;
    if (flag_) {
        info.book_id = ui_.add_book_id_edit->text().toInt();
        info.user_id = ui_.add_user_id_edit->text().toInt();
        info.borrow_deadline = QDate::currentDate().addDays(
                ui_.add_borrow_day_num_spin_box->value()
            ).toString(kDateFormat).toStdString();
    } else {
        info.book_id = ui_.err_book_id_edit->text().toInt();
        info.book_name = ui_.err_book_name_edit->text().toStdString();
        info.user_id = ui_.err_user_id_edit->text().toInt();
        info.user_login_name = ui_.err_user_login_name_edit->text().toStdString();
        info.borrow_date = ui_.err_borrow_date_edit->date().toString(kDateFormat).toStdString();
        info.borrow_deadline = ui_.err_borrow_deadline_edit->date().toString(kDateFormat).toStdString();
        info.price = ui_.err_price_edit->text().toDouble();
    }
    return info;
}

void BorrowDialog::accept() {
    std::regex re;
    QMessageBox *box = nullptr;
    if (flag_) {
        re = R"cpp(^\d+$)cpp";
        if (!std::regex_match(ui_.add_book_id_edit->text().toStdString(), re)) {
            box->setText("请输入正确数字作为书籍ID");
            box->exec();
            return;
        }
        if (!std::regex_match(ui_.add_user_id_edit->text().toStdString(), re)) {
            box->setText("请输入正确数字作为用户ID");
            box->exec();
            return;
        }
    }
    else {
        re = R"cpp(^\d+(\.?\d+)?$)cpp";
        if (!std::regex_match(ui_.err_price_edit->text().toStdString(), re)) {
            box->setText("请输入正确数字作为价格");
            box->exec();
            return;
        }
    }
    QDialog::accept();
}

}
