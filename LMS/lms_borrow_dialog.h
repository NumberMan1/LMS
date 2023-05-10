#ifndef LMS_BORROW_DIALOG_H
#define LMS_BORROW_DIALOG_H

#include <qdialog.h>
#include <ui_lms_borrow_edit_dialog.h>

#include "lms_config.h"

namespace lms {

class BorrowDialog : public QDialog {
public:
    explicit BorrowDialog(bool is_add, QWidget *parent = nullptr);
    ~BorrowDialog();
    void SetBorrowInfo(const BorrowInfo& info);
    // 调用前要保证数据合法
    BorrowInfo GetBorrowInfo() const;
signals:

public slots:
    void accept() override;
private slots:
private:
    bool flag_;
    Ui::BorrowEditDialog ui_;
};

}

#endif // !LMS_BORROW_DIALOG_H