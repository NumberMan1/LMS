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

void LoginActionImpl(const Json::Value json_value) noexcept {
    std::ofstream out{"/home/num/logs/a.txt"};
    mysqlx::Session mysql_session{lms::kMysqlxURL};
    mysqlx::string sql_str;
    bool is_reader = json_value["is_reader"].asBool();
    if (is_reader) {
        sql_str = "select reader_id from lms.T_READER where reader_login_name = ? and reader_psw = ?";
    } else {
        sql_str = "select administer_id from lms.T_ADMINISTER where administer_login_name = ? and administer_pwd = ?";
    }
    mysqlx::SqlStatement sql_state = mysql_session.sql(sql_str);
    std::string login_name = json_value["user_login_name"].asString();
    sql_state.bind(login_name, json_value["user_pwd"].asString());
    Json::Value json_respond;
    std::unique_lock lock{::stdout_respond_json_mutex, std::defer_lock};
    try {
        mysqlx::SqlResult sql_result = sql_state.execute();
        auto row = sql_result.fetchOne();
        if (row[0].isNull()) {
            json_respond["is_login"] = false;
        } else {
            json_respond["is_login"] = true;
            json_respond["user_id"] = static_cast<int>(row[0]);
        }
        lock.lock();
        ::stdout_respond_json = json_respond;
        lock.unlock();
        ::stdout_cv.notify_one();
        if (!row[0].isNull()) {
            std::string user_type;
            if (is_reader) {
                user_type = "管理员";
            } else {
                user_type = "读者";
            }
            auto logger = spdlog::daily_logger_mt("In LoginActionImpl func",
                    lms::kUserInfoLoggerPath);
            logger->info("用户{}id为{}登录系统(其为{})",
                login_name, json_respond["user_id"].asInt(), user_type);
            spdlog::drop(logger->name());
        }
    } catch (std::exception &err) {
        json_respond["is_login"] = false;
        lock.lock();
        ::stdout_respond_json = json_respond;
        lock.unlock();
        ::stdout_cv.notify_one();
        auto error_logger = spdlog::daily_logger_mt("In LoginActionImpl func",
                lms::kUserErrorLoggerPath);
        error_logger->error(err.what());
        spdlog::drop(error_logger->name());
    } catch (...) {
        json_respond["is_login"] = false;
        lock.lock();
        ::stdout_respond_json = json_respond;
        lock.unlock();
        ::stdout_cv.notify_one();
        auto error_logger = spdlog::daily_logger_mt("In LoginActionImpl func",
                lms::kUserErrorLoggerPath);
        error_logger->error("未捕获的未知的异常");
        spdlog::drop(error_logger->name());
    }
}

void SignUpImpl(const Json::Value json_value) noexcept {
    mysqlx::Session mysql_session{lms::kMysqlxURL};
    mysqlx::string sql_str{"insert into lms.T_READER(reader_login_name, reader_name, reader_psw, reader_tel) values (?, ?, ?, ?)"};
    mysqlx::SqlStatement sql_state = mysql_session.sql(sql_str);
    std::string login_name = json_value["user_login_name"].asString(),
        user_name = json_value["user_name"].asString(),
        tel = json_value["user_tel"].asString();
    sql_state.bind(login_name, user_name,
        json_value["user_pwd"].asString(), tel);
    Json::Value json_respond;
    std::unique_lock lock{::stdout_respond_json_mutex, std::defer_lock};
    try {
        sql_state.execute();
        json_respond["is_inserted"] = true;
        lock.lock();
        ::stdout_respond_json = json_respond;
        lock.unlock();
        ::stdout_cv.notify_one();
        auto logger = spdlog::daily_logger_mt("In SignUpImpl func",
                lms::kUserInfoLoggerPath);
        logger->info("{}注册了读者账号. 账号名为{}, 联系电话未{}",
            user_name, login_name, tel);
        spdlog::drop(logger->name());
    } catch (std::exception &err) {
        json_respond["is_inserted"] = false;
        lock.lock();
        ::stdout_respond_json = json_respond;
        lock.unlock();
        ::stdout_cv.notify_one();
        auto error_logger = spdlog::daily_logger_mt("In SignUpImpl func",
                lms::kUserErrorLoggerPath);
        error_logger->error(err.what());
        spdlog::drop(error_logger->name());
    } catch (...) {
        json_respond["is_inserted"] = false;
        lock.lock();
        ::stdout_respond_json = json_respond;
        lock.unlock();
        ::stdout_cv.notify_one();
        auto error_logger = spdlog::daily_logger_mt("In SignUpImpl func",
                lms::kUserErrorLoggerPath);
        error_logger->error("未捕获的未知的异常");
        spdlog::drop(error_logger->name());
    }
}

void SearchReaderImpl(const Json::Value json_value) noexcept {
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
    std::scoped_lock lock{::stdout_respond_json_mutex};
    ::stdout_respond_json = json_respond;
    ::stdout_cv.notify_one();
}

void UpdateReaderImpl(const Json::Value value) noexcept {
    mysqlx::Session mysql_session{lms::kMysqlxURL};
    mysqlx::string sql_str{"update lms.T_READER set reader_login_name = ?, reader_name = ?, reader_tel = ?, reader_psw = ? where reader_id = ?"};
    mysqlx::SqlStatement sql_state = mysql_session.sql(sql_str);
    int user_id = value["user_id"].asInt();
    std::string login_name = value["user_login_name"].asString(),
        user_name = value["user_name"].asString(),
        tel = value["user_tel"].asString();
    sql_state.bind(login_name, user_name, tel,
        value["user_pwd"].asString(), user_id);
    Json::Value respond;
    std::unique_lock lock{::stdout_respond_json_mutex, std::defer_lock};
    try {
        sql_state.execute();
        respond["is_updated"] = true;
        lock.lock();
        ::stdout_respond_json = respond;
        lock.unlock();
        ::stdout_cv.notify_one();
        auto logger = spdlog::daily_logger_mt("In UpdateReaderImpl func",
                lms::kUserInfoLoggerPath);
        logger->info(R"cpp(用户id为{}其登录名 用户姓名 用户电话 密码被更新分别为
{}, {}, {})cpp", login_name, user_name, tel);
        spdlog::drop(logger->name());
    } catch (std::exception &err) {
        respond["is_updated"] = false;
        lock.lock();
        ::stdout_respond_json = respond;
        lock.unlock();
        ::stdout_cv.notify_one();
        auto error_logger = spdlog::daily_logger_mt("In UpdateReaderImpl func",
                lms::kUserErrorLoggerPath);
        error_logger->error(err.what());
        spdlog::drop(error_logger->name());
    } catch (...) {
        respond["is_updated"] = false;
        lock.lock();
        ::stdout_respond_json = respond;
        lock.unlock();
        ::stdout_cv.notify_one();
        auto error_logger = spdlog::daily_logger_mt("In UpdateReaderImpl func",
                lms::kUserErrorLoggerPath);
        error_logger->error("未捕获的未知的异常");
        spdlog::drop(error_logger->name());
    }
}

void DeleteReaderImpl(const Json::Value &value) {
    mysqlx::Session mysql_session{lms::kMysqlxURL};
    mysqlx::string sql_str{"delete from lms.T_READER where reader_id = ?"};
    mysqlx::SqlStatement sql_state = mysql_session.sql(sql_str);
    int user_id = value["user_id"].asInt();
    sql_state.bind(user_id);
    Json::Value respond;
    std::unique_lock lock{::stdout_respond_json_mutex, std::defer_lock};
    try {
        sql_state.execute();
        respond["is_deleted"] = true;
        lock.lock();
        ::stdout_respond_json = respond;
        lock.unlock();
        ::stdout_cv.notify_one();
        auto logger = spdlog::daily_logger_mt("In UpdateReaderImpl func",
                lms::kUserInfoLoggerPath);
        logger->info("用户id为{}被删除", user_id);
        spdlog::drop(logger->name());
    } catch (std::exception &err) {
        respond["is_deleted"] = false;
        lock.lock();
        ::stdout_respond_json = respond;
        lock.unlock();
        ::stdout_cv.notify_one();
        auto error_logger = spdlog::daily_logger_mt("In UpdateReaderImpl func",
                lms::kUserErrorLoggerPath);
        error_logger->error(err.what());
        spdlog::drop(error_logger->name());
    } catch (...) {
        respond["is_deleted"] = false;
        lock.lock();
        ::stdout_respond_json = respond;
        lock.unlock();
        ::stdout_cv.notify_one();
        auto error_logger = spdlog::daily_logger_mt("In UpdateReaderImpl func",
                lms::kUserErrorLoggerPath);
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
                thread_pool.SubmitTask(::SearchReaderImpl, std::move(json_request_data));
            } else if (action == "login") {
                thread_pool.SubmitTask(::LoginActionImpl, std::move(json_request_data));
            } else if (action == "insert") {
                thread_pool.SubmitTask(::SignUpImpl, std::move(json_request_data));
            } else if (action == "update") {
                thread_pool.SubmitTask(::UpdateReaderImpl, std::move(json_request_data));
            } else if (action == "delete") {
                thread_pool.SubmitTask(::DeleteReaderImpl, std::move(json_request_data));
            }
            std::unique_lock main_unique_lock{::stdout_respond_json_mutex};
            ::stdout_cv.wait(main_unique_lock);
            FCGI_printf("Content-type: application/json\r\n"); // 传出数据格式 必须
            FCGI_printf("\r\n"); // 标志响应头结束 必须
            FCGI_printf(::stdout_respond_json.toStyledString().c_str());
        } catch (std::exception &err) {
            auto logger = spdlog::daily_logger_mt("In main func",
                    lms::kUserErrorLoggerPath);
            logger->error(err.what());
            spdlog::drop(logger->name());
        } catch (...) {
            auto logger = spdlog::daily_logger_mt("In main func",
                    lms::kUserErrorLoggerPath);
            logger->error("未捕获的未知的异常");
            spdlog::drop(logger->name());
        }
    }
	return 0;
}
