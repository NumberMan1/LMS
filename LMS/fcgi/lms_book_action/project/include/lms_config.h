#ifndef LMS_LMS_CONFIG_H
#define LMS_LMS_CONFIG_H

#include <string>

namespace lms {

enum class UserType {
    kManager,
    kBookReader,
};
enum BookType {
    kEducation = 1,
    kTale = 2,
    kLiterature = 3,
    kScienceFiction = 4
};

constexpr auto kDateFormat{"yyyy-MM-dd"};
constexpr auto kHttpInitStr{"http://%1:80/%2"};
constexpr auto kMysqlxURL{"mysqlx://num:123@127.0.0.1"};
constexpr auto kHostName{"10.62.0.135"};
constexpr auto kBookURL{"book_action"};
constexpr auto kUserURL{"user_action"};
constexpr auto kBorrowURL{"borrow_action"};
constexpr auto kDataBaseName{"lms"};
constexpr auto kDataBaseAdministerTableName{"T_ADMINISTER"};
constexpr auto kDataBaseBookTableName{"T_BOOK"};
constexpr auto kDataBaseBorrowRecordTableName{"T_BORROW_RECORD"};
constexpr auto kDataBaseReaderTableName{"T_READER"};
// 默认搜索的信息数量
constexpr int kSearchItemNum{10};
// 编辑和删除按钮的对应所在的table的列下标
constexpr int kAdministratorBookTableEditColumn{7};
constexpr int kAdministratorBookTableDelColumn{
    kAdministratorBookTableEditColumn + 1};
constexpr int kAdministratorBorrowTableReturnColumn{7};
constexpr int kAdministratorBorrowTableErrorColumn{
    kAdministratorBorrowTableReturnColumn + 1};
constexpr int kAdministratorUserTableEditColumn{4};
constexpr int kAdministratorUserTableDelColumn{
    kAdministratorUserTableEditColumn + 1};
constexpr int kUserBorrowTableReturnColumn{4};
constexpr int kUserBorrowTablePayColumn{
    kUserBorrowTableReturnColumn + 1};

struct BookInfo {
    int id{0};
    std::string name;
    BookType type;
    std::string auther;
    std::string press;
    double price{0};
    int num{0};
};

struct UserInfo {
    int id{0};
    std::string login_name; 
    std::string name;    
    std::string tel;
    std::string pwd; // 此为SHA256后的结果
};

struct BorrowInfo {
    int book_id{0};
    std::string book_name;
    int user_id{0};
    std::string user_login_name;
    std::string borrow_date;
    std::string borrow_deadline;
    double price{0};
};

}

struct BorrowInfo {
    int book_id{ 0 };
    std::string book_name;
    int user_id{ 0 };
    std::string user_login_name;
    std::string borrow_date;
    std::string borrow_deadline;
    double price{ 0 };
};

#endif // !LMS_LMS_CONFIG_H