#include <fastcgi/fcgi_stdio.h>
#include <string>
#include <fstream>
#include <json/json.h>
#include <mysqlx/xdevapi.h>

#include "cjs/mine_hash.hpp"

void LoginAction(const Json::Value &json_value) {
    FCGI_printf("Content-type: application/json\r\n"); // 传出数据格式 必须
    FCGI_printf("\r\n"); // 标志响应头结束 必须
    if (json_value["is_reader"].asBool()) {
        mysqlx::Session mysql_session{"mysqlx://num:123@127.0.0.1"};
        mysqlx::string sql_str{"select reader_id from lms.T_READER where reader_login_name = ? and reader_psw = ?"};
        mysqlx::SqlStatement sql_state = mysql_session.sql(sql_str);
        sql_state.bind(json_value["user_login_name"].asString(), json_value["user_pwd"].asCString());
        Json::Value json_respond;
        try {
            mysqlx::SqlResult sql_result = sql_state.execute();
            auto sql_list = sql_result.fetchAll();
            for (auto row : sql_list) {
                if (!row.isNull()) {
                    json_respond["is_login"] = true;
                    FCGI_printf(json_respond.toStyledString().c_str());
                }
            }
        } catch (...) {
            json_respond["is_login"] = false;
            FCGI_printf(json_respond.toStyledString().c_str());
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
        if (action == "login") {
            LoginAction(json_request_data);
        }
	}
	return 0;
}
