#ifndef LMS_USER_DIALOG
#define LMS_USER_DIALOG

#include <ui_lms_user_edit_dialog.h>

#include "lms_config.h"

namespace lms {

class UserDialog : public QDialog {
public:
    explicit UserDialog(QWidget *parent = nullptr);
    ~UserDialog();
    // 调用前要保证数据合法
    void SetUserInfo(const UserInfo &info);
    UserInfo GetUserInfo() const;
    
signals:
    
public slots:
    void accept() override;
private slots:
private:
    Ui::UserEditDialog ui_;
};

}

#endif // !LMS_USER_DIALOG
