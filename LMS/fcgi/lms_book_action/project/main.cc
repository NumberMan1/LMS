#include <fastcgi/fcgi_stdio.h>
#include <string>
#include <fstream>
#include <json/json.h>
#include <mysqlx/xdevapi.h>

#include "lms_config.h"
#include "lms_func.h"

namespace {

void SearchBookSqlTextInit(const Json::Value json_value, mysqlx::string *mysql_sql, int *text_num) {
    using mstring = mysqlx::string;
    bool flag = false;
    if (json_value["is_book_id"].asBool()) {
        ++(*text_num);
        flag = true;
        (*mysql_sql) += static_cast<mstring>(R"cpp( book_class_id like ?)cpp");
    }
    if (json_value["is_book_name"].asBool()) {
        ++(*text_num);
        if (flag) {
            (*mysql_sql) += static_cast<mstring>(" or");
        }
        flag = true;
        (*mysql_sql) += static_cast<mstring>(R"cpp( book_name like ?)cpp");
    }
    if (json_value["is_auther"].asBool()) {
        ++(*text_num);
        if (flag) {
            (*mysql_sql) += static_cast<mstring>(" or");
        }
        (*mysql_sql) += static_cast<mstring>(R"cpp( book_auther like ?)cpp");
    }
}

void SearchBookImpl(const Json::Value &json_value) {
    using mstring = mysqlx::string;
    // std::ofstream out{"/home/num/logs/log.txt"};
    mysqlx::Session mysql_session{lms::kMysqlxURL};
    mstring mysql_sql{"select * from lms.T_BOOK"};
    bool type_is_all;
    int args_num = 1; // 必定有limit
    if (json_value["type"].asInt() == 0) { // 表示所有类型
        type_is_all = true;
        if (!json_value["text"].asString().empty()) { // 搜索内容不为空
            mysql_sql += static_cast<mstring>(" where");
            ::SearchBookSqlTextInit(json_value, &mysql_sql, &args_num);
        }
    } else {
        if (!json_value["text"].asString().empty()) {
            mysql_sql += static_cast<mstring>(" where book_type = ?");
            ::SearchBookSqlTextInit(json_value, &mysql_sql, &args_num);
        }
        type_is_all = false;
    }
    mysql_sql += static_cast<mstring>(" limit ?, 10");
    mysqlx::SqlStatement sql_state = mysql_session.sql(mysql_sql);
    lms::SearchBindArgs(json_value, &sql_state, type_is_all, args_num);
    mysqlx::SqlResult sql_rt = sql_state.execute();
    auto row_list = sql_rt.fetchAll();
    Json::Value json_respond;
    for (int i = 0; auto row : row_list) {
        json_respond[i]["book_id"] = static_cast<int>(row[0]);
        json_respond[i]["book_name"] = static_cast<std::string>(row[1]);
        json_respond[i]["book_type"] = static_cast<int>(row[2]);
        json_respond[i]["book_auther"] = static_cast<std::string>(row[3]);
        json_respond[i]["book_press"] = static_cast<std::string>(row[4]);
        json_respond[i]["book_price"] = static_cast<double>(row[5]);
        json_respond[i]["book_num"] = static_cast<int>(row[6]);
        ++i;
    }
    FCGI_printf("Content-type: application/json\r\n"); // 传出数据格式 必须
    FCGI_printf("\r\n"); // 标志响应头结束 必须
    FCGI_printf(json_respond.toStyledString().c_str());
}

void UpdateBookImpl(const Json::Value &json) {
    mysqlx::Session mysql_session{lms::kMysqlxURL};
    mysqlx::string sql_str{"update lms.T_BOOK set book_name = ?, book_type = ?, book_auther = ?, book_press = ?,  book_price = ?, book_num = ? where book_class_id = ?"};
    auto sql_st = mysql_session.sql(sql_str);
    sql_st.bind(json["book_name"].asString(), json["book_type"].asInt(),
        json["book_auther"].asString(), json["book_press"].asString(),
        json["book_price"].asDouble(), json["book_num"].asInt(), json["book_id"].asInt());
    Json::Value respond;
    FCGI_printf("Content-type: application/json\r\n"); // 传出数据格式 必须
    FCGI_printf("\r\n"); // 标志响应头结束 必须
    try {
        auto sql_rt = sql_st.execute();
        respond["is_updated"] = true;
        FCGI_printf(respond.toStyledString().c_str());
    } catch (...) {
        respond["is_updated"] = false;
        FCGI_printf(respond.toStyledString().c_str());
    }
}

void InsertBookImpl(const Json::Value &value) {
    mysqlx::Session mysql_ses{lms::kMysqlxURL};
    // 检查是否有重复的书籍
    mysqlx::string sql_str{"select * from lms.T_BOOK where book_name = ? and book_type = ? and book_auther = ? and book_press = ?"};
    mysqlx::SqlStatement sql_state = mysql_ses.sql(sql_str);
    sql_state.bind(value["book_name"].asString(), value["book_type"].asInt(), value["book_auther"].asString(), value["book_press"].asString());
    auto sql_rt = sql_state.execute();
    Json::Value respond;
    FCGI_printf("Content-type: application/json\r\n"); // 传出数据格式 必须
    FCGI_printf("\r\n"); // 标志响应头结束 必须
    if (sql_rt.count()) { // 返回错误: 不为0
        respond["is_inserted"] = false;
        FCGI_printf(respond.toStyledString().c_str());
        return;
    }
    sql_str = "insert into lms.T_BOOK (book_name, book_type, book_auther, book_press, book_price, book_num) VALUE (?, ?, ?, ?, ?, ?)";
    sql_state = mysql_ses.sql(sql_str);
    sql_state.bind(value["book_name"].asString(), value["book_type"].asInt(),
        value["book_auther"].asString(), value["book_press"].asString(),
        value["book_price"].asDouble(), value["book_num"].asInt());
    try {
        sql_rt = sql_state.execute();
        respond["is_inserted"] = true;
        FCGI_printf(respond.toStyledString().c_str());
    } catch (...) {
        respond["is_inserted"] = false;
        FCGI_printf(respond.toStyledString().c_str());
    }
}

void DeleteBookImpl(const Json::Value &value) {
    mysqlx::Session mysql_ses{lms::kMysqlxURL};
    mysqlx::string sql_str{"delete from lms.T_BOOK where book_class_id = ?"};
    mysqlx::SqlStatement sql_state = mysql_ses.sql(sql_str);
      
    sql_state.bind(value["book_id"].asInt());
    Json::Value respond;
    FCGI_printf("Content-type: application/json\r\n"); // 传出数据格式 必须
    FCGI_printf("\r\n"); // 标志响应头结束 必须
    try {
        auto sql_rt = sql_state.execute();
        respond["is_deleted"] = true;
        FCGI_printf(respond.toStyledString().c_str());
    } catch (...) {
        respond["is_deleted"] = false;
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
            ::SearchBookImpl(json_request_data);
        } else if (action == "update") {
            ::UpdateBookImpl(json_request_data);
        } else if (action == "insert") {
            ::InsertBookImpl(json_request_data);
        } else if (action == "delete") {
            ::DeleteBookImpl(json_request_data);
        }
	}
	return 0;
}
