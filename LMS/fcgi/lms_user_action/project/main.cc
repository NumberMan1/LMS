#include <fastcgi/fcgi_stdio.h>
#include <string>
#include <fstream>
#include <json/json.h>
#include <mysqlx/xdevapi.h>

#include "lms_config.h"
#include "lms_func.h"

namespace {

void SearchReaderSqlTextInit(const Json::Value &json_value,
        mysqlx::string *sql, int *args_num) {
    using mstring = mysqlx::string;
    bool flag = false;
    if (json_value["is_reader_login_name"].asBool()) {
        ++(*args_num);
        flag = true;
        (*sql) += static_cast<mstring>(" reader_login_name like ?");
    }
    if (json_value["is_reader_name"].asBool()) {
        ++(*args_num);
        if (flag) {
            (*sql) += static_cast<mstring>(" or");
        }
        flag = true;
        (*sql) += static_cast<mstring>(" reader_name like ?");
    }
    if (json_value["is_reader_tel"].asBool()) {
        ++(*args_num);
        if (flag) {
            (*sql) += static_cast<mstring>(" or");
        }
        (*sql) += static_cast<mstring>(" reader_tel like ?");
    }
}

void LoginActionImpl(const Json::Value &json_value) {
    // std::ofstream out{"/home/num/logs/log.txt"};
    mysqlx::Session mysql_session{lms::kMysqlxURL};
    mysqlx::string sql_str;
    if (json_value["is_reader"].asBool()) {
        sql_str = "select reader_id from lms.T_READER where reader_login_name = ? and reader_psw = ?";
    } else {
        sql_str = "select administer_id from lms.T_ADMINISTER where administer_login_name = ? and administer_pwd = ?";
    }
    mysqlx::SqlStatement sql_state = mysql_session.sql(sql_str);
    sql_state.bind(json_value["user_login_name"].asString(), json_value["user_pwd"].asString());
    Json::Value json_respond;
    FCGI_printf("Content-type: application/json\r\n"); // 传出数据格式 必须
    FCGI_printf("\r\n"); // 标志响应头结束 必须
    try {
        mysqlx::SqlResult sql_result = sql_state.execute();
        auto row = sql_result.fetchOne();
        if (row[0].isNull()) {
            json_respond["is_login"] = false;
        } else {
            json_respond["is_login"] = true;
            json_respond["user_id"] = static_cast<int>(row[0]);
        }
        FCGI_printf(json_respond.toStyledString().c_str());
    } catch (...) {
        json_respond["is_login"] = false;
        FCGI_printf(json_respond.toStyledString().c_str());
    }
}

void SignUpImpl(const Json::Value &json_value) {
    // std::ofstream out{"/home/num/logs/log.txt"};
    mysqlx::Session mysql_session{lms::kMysqlxURL};
    mysqlx::string sql_str{"insert into lms.T_READER(reader_login_name, reader_name, reader_psw, reader_tel) values (?, ?, ?, ?)"};
    mysqlx::SqlStatement sql_state = mysql_session.sql(sql_str);
    sql_state.bind(json_value["user_login_name"].asString(), json_value["user_name"].asString(),
        json_value["user_pwd"].asString(), json_value["user_tel"].asString());
    Json::Value json_respond;
    FCGI_printf("Content-type: application/json\r\n"); // 传出数据格式 必须
    FCGI_printf("\r\n"); // 标志响应头结束 必须
    try {
        auto sql_re = sql_state.execute();
        json_respond["is_inserted"] = true;
        FCGI_printf(json_respond.toStyledString().c_str());
    } catch (...) {
        json_respond["is_inserted"] = false;
        FCGI_printf(json_respond.toStyledString().c_str());
    }
}

void SearchReaderImpl(const Json::Value &json_value) {
    // std::ofstream out{"/home/num/logs/log.txt"};
    mysqlx::Session mysql_session{lms::kMysqlxURL};
    using mstring = mysqlx::string;
    mstring sql_str{"select reader_id, reader_login_name, reader_name, reader_tel from lms.T_READER"};
    int args_num = 1; // 必定有limit
    if (!json_value["text"].asString().empty()) {
        sql_str += static_cast<mstring>(" where");
        ::SearchReaderSqlTextInit(json_value, &sql_str, &args_num);
    }
    sql_str += static_cast<mstring>(" limit ?, 10");
    mysqlx::SqlStatement sql_state = mysql_session.sql(sql_str);
    lms::SearchBindArgs(json_value, &sql_state, args_num);
    mysqlx::SqlResult sql_rt = sql_state.execute();
    auto row_list = sql_rt.fetchAll();
    Json::Value json_respond;
    for (int i = 0; auto row : row_list) {
        json_respond[i]["reader_id"] = static_cast<int>(row[0]);
        json_respond[i]["reader_login_name"] = static_cast<std::string>(row[1]);
        json_respond[i]["reader_name"] = static_cast<std::string>(row[2]);
        json_respond[i]["reader_tel"] = static_cast<std::string>(row[3]);
        ++i;
    }
    FCGI_printf("Content-type: application/json\r\n"); // 传出数据格式 必须
    FCGI_printf("\r\n"); // 标志响应头结束 必须
    FCGI_printf(json_respond.toStyledString().c_str());
}

void UpdateReaderImpl(const Json::Value &value) {
    mysqlx::Session mysql_session{lms::kMysqlxURL};
    mysqlx::string sql_str{"update lms.T_READER set reader_login_name = ?, reader_name = ?, reader_tel = ?, reader_psw = ? where reader_id = ?"};
    mysqlx::SqlStatement sql_state = mysql_session.sql(sql_str);
    sql_state.bind(value["user_login_name"].asString(), value["user_name"].asString(),
        value["user_tel"].asString(), value["user_pwd"].asString(), value["user_id"].asInt());
    Json::Value respond;
    FCGI_printf("Content-type: application/json\r\n"); // 传出数据格式 必须
    FCGI_printf("\r\n"); // 标志响应头结束 必须
    try {
        auto rt = sql_state.execute();
        respond["is_updated"] = true;
        FCGI_printf(respond.toStyledString().c_str());
    } catch (...) {
        respond["is_updated"] = false;
        FCGI_printf(respond.toStyledString().c_str());
    }
}

// void DeleteReaderImpl(const Json::Value &value) {
//     mysqlx::Session mysql_session{lms::kMysqlxURL};
//     mysqlx::string sql_str{"delete from lms.T_READER where reader_id = ?"};
//     mysqlx::SqlStatement sql_state = mysql_session.sql(sql_str);
//     sql_state.bind(value["user_id"].asInt());
//     Json::Value respond;
//     FCGI_printf("Content-type: application/json\r\n"); // 传出数据格式 必须
//     FCGI_printf("\r\n"); // 标志响应头结束 必须
//     try {
//         auto rt = sql_state.execute();
//         respond["is_deleted"] = true;
//         FCGI_printf(respond.toStyledString().c_str());
//     } catch (...) {
//         respond["is_deleted"] = false;
//         FCGI_printf(respond.toStyledString().c_str());
//     }
// }

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
            ::SearchReaderImpl(json_request_data);
        } else if (action == "login") {
            ::LoginActionImpl(json_request_data);
        } else if (action == "insert") {
            ::SignUpImpl(json_request_data);
        } else if (action == "update") {
            ::UpdateReaderImpl(json_request_data);
        }
	}
	return 0;
}
