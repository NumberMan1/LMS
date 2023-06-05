#include <fastcgi/fcgi_stdio.h>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/daily_file_sink.h>
#include <string>
#include <fstream>
#include <json/json.h>
#include <mysqlx/xdevapi.h>

#include "lms_config.h"
#include "lms_func.h"
#include "cjs/thread_pool.hpp"

namespace {

Json::Value stdout_respond_json; // 用于主线程发送的数据
std::mutex stdout_respond_json_mutex; // 用于保护FCGI_printf用的json数据
std::condition_variable stdout_cv; // 用于同步FCGI_printf

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
    std::scoped_lock lock{::stdout_respond_json_mutex};
    ::stdout_respond_json = json_respond;
    ::stdout_cv.notify_one();
}

void UpdateBookImpl(const Json::Value &json) {
    mysqlx::Session mysql_session{lms::kMysqlxURL};
    mysqlx::string sql_str{"update lms.T_BOOK set book_name = ?, book_type = ?, book_auther = ?, book_press = ?,  book_price = ?, book_num = ? where book_class_id = ?"};
    auto sql_st = mysql_session.sql(sql_str);
    std::string book_name = json["book_name"].asString(),
        book_auther = json["book_auther"].asString(),
        book_press = json["book_press"].asString();
    int book_num = json["book_num"].asInt(),
        book_type = json["book_type"].asInt(),
        book_id = json["book_id"].asInt();
    double book_price = json["book_price"].asDouble();
    sql_st.bind(book_name, book_type, book_auther,
        book_press, book_price, book_num, book_id);
    Json::Value respond;
    std::unique_lock lock{::stdout_respond_json_mutex, std::defer_lock};
    try {
        sql_st.execute();
        respond["is_updated"] = true;
        lock.lock();
        ::stdout_respond_json = respond;
        lock.unlock();
        ::stdout_cv.notify_one();
        auto logger = spdlog::daily_logger_mt("In UpdateBookImpl func",
                lms::kBookInfoLoggerPath);
        logger->info(R"cpp(书籍id为{}其书名 作者名 出版社 价格 数量被更新分别为
{}, {}, {}, {}, {}, )cpp", book_id, book_name, book_auther, book_press, book_price, book_num);
        spdlog::drop(logger->name());
    } catch (std::exception &err) {
        respond["is_updated"] = false;
        lock.lock();
        ::stdout_respond_json = respond;
        lock.unlock();
        ::stdout_cv.notify_one();
        auto error_logger = spdlog::daily_logger_mt("In UpdateBookImpl func",
                lms::kBookErrorLoggerPath);
        error_logger->error(err.what());
        spdlog::drop(error_logger->name());
    } catch (...) {
        respond["is_updated"] = false;
        lock.lock();
        ::stdout_respond_json = respond;
        lock.unlock();
        ::stdout_cv.notify_one();
        auto error_logger = spdlog::daily_logger_mt("In UpdateBookImpl func",
                lms::kBookErrorLoggerPath);
        error_logger->error("未捕获的未知的异常");
        spdlog::drop(error_logger->name());
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
    std::unique_lock lock{::stdout_respond_json_mutex, std::defer_lock};
    if (sql_rt.count()) { // 返回错误: 不为0
        respond["is_inserted"] = false;
        FCGI_printf(respond.toStyledString().c_str());
        return;
    }
    sql_str = "insert into lms.T_BOOK (book_name, book_type, book_auther, book_press, book_price, book_num) VALUE (?, ?, ?, ?, ?, ?)";
    sql_state = mysql_ses.sql(sql_str);
    std::string book_name = value["book_name"].asString(),
        book_auther = value["book_auther"].asString(),
        book_press = value["book_press"].asString();
    int book_num = value["book_num"].asInt(),
        book_type = value["book_type"].asInt();
    double book_price = value["book_price"].asDouble();
    sql_state.bind(book_name, book_type, book_auther,
        book_press, book_price, book_num);
    try {
        sql_state.execute();
        respond["is_inserted"] = true;
        lock.lock();
        ::stdout_respond_json = respond;
        lock.unlock();
        ::stdout_cv.notify_one();
        auto logger = spdlog::daily_logger_mt("In InsertBookImpl func",
                lms::kBookInfoLoggerPath);
        logger->info("添加了新书{}, 作者为{}, 出版社为{}, 价格为{}, 数量为{}",
            book_name, book_auther, book_press, book_price, book_num);
        spdlog::drop(logger->name());
    } catch (std::exception &err) {
        respond["is_inserted"] = false;
        lock.lock();
        ::stdout_respond_json = respond;
        lock.unlock();
        ::stdout_cv.notify_one();
        auto error_logger = spdlog::daily_logger_mt("In InsertBookImpl func",
                lms::kBookErrorLoggerPath);
        error_logger->error(err.what());
        spdlog::drop(error_logger->name());
    } catch (...) {
        respond["is_inserted"] = false;
        lock.lock();
        ::stdout_respond_json = respond;
        lock.unlock();
        ::stdout_cv.notify_one();
        auto error_logger = spdlog::daily_logger_mt("In InsertBookImpl func",
                lms::kBookErrorLoggerPath);
        error_logger->error("未捕获的未知的异常");
        spdlog::drop(error_logger->name());
    }
}

void DeleteBookImpl(const Json::Value &value) {
    mysqlx::Session mysql_ses{lms::kMysqlxURL};
    mysqlx::string sql_str{"delete from lms.T_BOOK where book_class_id = ?"};
    mysqlx::SqlStatement sql_state = mysql_ses.sql(sql_str);
    int book_id = value["book_id"].asInt();
    sql_state.bind(book_id);
    Json::Value respond;
    std::unique_lock lock{::stdout_respond_json_mutex, std::defer_lock};
    try {
        sql_state.execute();
        respond["is_deleted"] = true;
        lock.lock();
        ::stdout_respond_json = respond;
        lock.unlock();
        ::stdout_cv.notify_one();
        auto logger = spdlog::daily_logger_mt("In DeleteBookImpl func",
                lms::kBookInfoLoggerPath);
        logger->info("书籍id为{}被删除",book_id);
        spdlog::drop(logger->name());
    } catch (std::exception &err) {
        respond["is_deleted"] = false;
        lock.lock();
        ::stdout_respond_json = respond;
        lock.unlock();
        ::stdout_cv.notify_one();
        auto error_logger = spdlog::daily_logger_mt("In DeleteBookImpl func",
                lms::kBookErrorLoggerPath);
        error_logger->error(err.what());
        spdlog::drop(error_logger->name());
    } catch (...) {
        respond["is_deleted"] = false;
        lock.lock();
        ::stdout_respond_json = respond;
        lock.unlock();
        ::stdout_cv.notify_one();
        auto error_logger = spdlog::daily_logger_mt("In DeleteBookImpl func",
                lms::kBookErrorLoggerPath);
        error_logger->error("未捕获的未知的异常");
        spdlog::drop(error_logger->name());
    }
}

}

int main(int argc, char *argv[]) {
    cjs::ThreadPool thread_pool{};
    spdlog::flush_every(std::chrono::seconds(3));
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
        try {
            if (action == "search") {
                thread_pool.SubmitTask(::SearchBookImpl, std::move(json_request_data));
            } else if (action == "update") {
                thread_pool.SubmitTask(::UpdateBookImpl, std::move(json_request_data));
            } else if (action == "insert") {
                thread_pool.SubmitTask(::InsertBookImpl, std::move(json_request_data));
            } else if (action == "delete") {
                thread_pool.SubmitTask(::DeleteBookImpl, std::move(json_request_data));
            }
            std::unique_lock main_unique_lock{::stdout_respond_json_mutex};
            ::stdout_cv.wait(main_unique_lock);
            FCGI_printf("Content-type: application/json\r\n"); // 传出数据格式 必须
            FCGI_printf("\r\n"); // 标志响应头结束 必须
            FCGI_printf(::stdout_respond_json.toStyledString().c_str());
        } catch (std::exception &err) {
            auto logger = spdlog::daily_logger_mt("In main func",
                    lms::kBookErrorLoggerPath);
            logger->error(err.what());
            spdlog::drop(logger->name());
        } catch (...) {
            auto logger = spdlog::daily_logger_mt("In main func",
                    lms::kBookErrorLoggerPath);
            logger->error("未捕获的未知的异常");
            spdlog::drop(logger->name());
        }
	}
	return 0;
}
