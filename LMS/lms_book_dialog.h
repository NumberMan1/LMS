#ifndef LMS_BOOK_DIALOG
#define LMS_BOOK_DIALOG

#include <ui_lms_book_edit_dialog.h>

#include "lms_config.h"

namespace lms {

class BookDialog : public QDialog {
public:
    explicit BookDialog(const bool is_insert, QWidget *parent = nullptr);
    ~BookDialog();
    void SetDialogType(const bool is_insert);
    void SetBookInfo(const BookInfo &info);
    // 调用前要保证数据合法
    BookInfo GetBookInfo() const;
signals:
    
public slots:
    void accept() override;
private slots:
private:
    explicit BookDialog(QWidget *parent = nullptr);
    bool is_insert_ = false;
    Ui::BookEditDialog ui_;
};


}
#endif // !LMS_BOOK_DIALOG
