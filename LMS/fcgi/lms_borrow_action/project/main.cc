#include <fastcgi/fcgi_stdio.h>
#include <string>
#include <fstream>
#include <json/json.h>
#include <mysqlx/xdevapi.h>
#include <chrono>

#include "lms_config.h"
#include "lms_func.h"

namespace {

void SearchBorrowSqlTextInit(const Json::Value &json_value,
        mysqlx::string *sql, int *args_num) {
    using mstring = mysqlx::string;
    bool flag = false;
    if (json_value["is_reader_login_name"].asBool()) {
        ++(*args_num);
        flag = true;
        (*sql) += static_cast<mstring>(" reader_login_name like ?");
    }
    if (json_value["is_book_name"].asBool()) {
        ++(*args_num);
        if (flag) {
            (*sql) += static_cast<mstring>(" or");
        }
        (*sql) += static_cast<mstring>(" book_name like ?");
    }
}

void SearchBorrowListImpl(const Json::Value &data) {
    using mstring = mysqlx::string;
    mysqlx::Session sess{lms::kMysqlxURL};
    mysqlx::string sql{R"cpp(select book_class_id, reader_id, date_format(borrow_date, '%Y-%m-%d'), date_format(borrow_deadline, '%Y-%m-%d'), borrow_price from lms.T_BORROW_RECORD)cpp"};
    int args_num = 1;
    if (!data["is_reader"].asBool()) {
        if (!data["text"].asString().empty()) { // 不为空
            sql += static_cast<mstring>(" where");
            ::SearchBorrowSqlTextInit(data, &sql, &args_num);
        }
    }
    sql += static_cast<mstring>(" limit ?, 10");
    auto state = sess.sql(sql);
    lms::SearchBindArgs(data, &state, args_num);
    auto rt = state.execute();
    auto row_list = rt.fetchAll();
    Json::Value json_respond;
    for (int i = 0; auto borrow_row : row_list) {
        int book_id = static_cast<int>(borrow_row[0]);
        int reader_id = static_cast<int>(borrow_row[1]);
        auto book_name_state = sess.sql(std::string("select book_name from lms.T_BOOK where book_class_id = ") + std::to_string(book_id));
        auto book_name = static_cast<std::string>(book_name_state.execute().fetchOne()[0]);
        auto reader_login_name_state = sess.sql(std::string("select reader_login_name from lms.T_READER where reader_id = ") + std::to_string(reader_id));
        auto reader_login_name = static_cast<std::string>(reader_login_name_state.execute().fetchOne()[0]);
        json_respond[i]["book_id"] = book_id;
        json_respond[i]["book_name"] = book_name;
        json_respond[i]["reader_id"] = reader_id;
        json_respond[i]["reader_login_name"] = reader_login_name;
        json_respond[i]["borrow_date"] = static_cast<std::string>(borrow_row[2]);
        json_respond[i]["borrow_deadline"] = static_cast<std::string>(borrow_row[3]);
        json_respond[i]["book_price"] = static_cast<double>(borrow_row[4]);
        ++i;
    }
    FCGI_printf("Content-type: application/json\r\n"); // 传出数据格式 必须
    FCGI_printf("\r\n"); // 标志响应头结束 必须
    FCGI_printf(json_respond.toStyledString().c_str());
}

double CalculateBorrowPrice(int book_class_id, const std::string &deadline) {
    mysqlx::Session sess{lms::kMysqlxURL};
    mysqlx::string sql{"select book_price from lms.T_BOOK where book_class_id = ?"};
    auto state = sess.sql(sql);
    state.bind(book_class_id);
    double book_price = static_cast<double>(state.execute().fetchOne().get(0));
    sql = "select datediff(?, now())";
    auto day_state = sess.sql(sql);
    day_state.bind(deadline);
    int borrow_day_num = static_cast<int>(day_state.execute().fetchOne().get(0));
    double borrow_price = (book_price / 100) * borrow_day_num;
    return borrow_price;
}

void BorrowBookImpl(const Json::Value &data) {
    mysqlx::Session sess{lms::kMysqlxURL};
    sess.startTransaction();
    mysqlx::string save_point = sess.setSavepoint();
    mysqlx::string sql{"insert into lms.T_BORROW_RECORD(book_class_id, reader_id, borrow_date, borrow_deadline, borrow_price) VALUES (?, ?, now(), ?, ?)"};
    auto state = sess.sql(sql);
    int book_class_id = data["book_class_id"].asInt();
    int reader_id = data["reader_id"].asInt();
    std::string deadline = data["borrow_deadline"].asString();
    Json::Value respond;
    FCGI_printf("Content-type: application/json\r\n"); // 传出数据格式 必须
    FCGI_printf("\r\n"); // 标志响应头结束 必须
    try {
        state.bind(book_class_id, reader_id,
            deadline, ::CalculateBorrowPrice(book_class_id, deadline));
        state.execute();
        sess.commit();
        respond["is_borrowed"] = true;
        FCGI_printf(respond.toStyledString().c_str());
    } catch (...) {
        sess.rollbackTo(save_point);
        respond["is_borrowed"] = false;
        FCGI_printf(respond.toStyledString().c_str());
    }
}

void ErrorBorrowImpl(const Json::Value &data) {
    mysqlx::Session sess{lms::kMysqlxURL};
    mysqlx::string sql{"update lms.T_BORROW_RECORD set borrow_price = ?, borrow_deadline = ?, borrow_text = ? where book_class_id = ? and reader_id = ? and borrow_date = ?"};
    auto state = sess.sql(sql);
    state.bind(data["borrow_price"].asDouble(), data["borrow_deadline"].asString(),
        data["error_text"].asString(), data["book_class_id"].asInt(),
        data["reader_id"].asInt(), data["borrow_date"].asString());
    Json::Value respond;
    FCGI_printf("Content-type: application/json\r\n"); // 传出数据格式 必须
    FCGI_printf("\r\n"); // 标志响应头结束 必须
    try {
        state.execute();
        respond["is_solved"] = true;
        FCGI_printf(respond.toStyledString().c_str());
    } catch (mysqlx::Error &err) {
        respond["is_solved"] = false;
        FCGI_printf(respond.toStyledString().c_str());
    }
}

}

int main(int argc, char *argv[]) {
	//阻塞等待并监听某个端口，等待Nginx将数据发过来
	while (FCGI_Accept() >= 0) {
		//如果想得到数据，可从stdin去读，实际上从Nginx上去读
		//如果想上传数据，可从stdout写，实际上是给Nginx写数据
        //前端的输入输出流为实际为FCGI_stdin FCGI_stdout
        //原理为undef和define标准头文件
		// FCGI_printf("Content-type: application/json\r\n"); // 传出数据格式 必须
		// FCGI_printf("\r\n"); // 标志响应头结束 必须
        // FCGI_printf(s.data()); // 回复数据 必须
        // std::printf(s.data()); // 前端并不会收到, 前端的输入输出流为FCGI_stdin FCGI_stdout
        //如果需要前端传入的请求参数可从环境变量获得如getenv()
        std::string request_data;
        char c = FCGI_getchar();
        while (c != '}') {
            request_data += c;
            c = FCGI_getchar();
        }
        request_data += c;
        Json::Reader reader;
        Json::Value json_request_data;
        reader.parse(request_data.data(), json_request_data);
        std::string action = json_request_data["action"].asString();
        if (action == "search") {
            ::SearchBorrowListImpl(json_request_data);
        } else if (action == "borrow") {
            ::BorrowBookImpl(json_request_data);
        } else if (action == "error") {
            ::ErrorBorrowImpl(json_request_data);
        }
	}
	return 0;
}
