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

void SearchBorrowListImpl(const Json::Value data) noexcept {
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
    std::scoped_lock lock(::stdout_respond_json_mutex);
    ::stdout_respond_json = json_respond;
    ::stdout_cv.notify_one();
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

void BorrowBookImpl(const Json::Value data) noexcept {
    mysqlx::Session sess{lms::kMysqlxURL};
    sess.startTransaction();
    mysqlx::string save_point = sess.setSavepoint();
    mysqlx::string sql{"insert into lms.T_BORROW_RECORD(book_class_id, reader_id, borrow_date, borrow_deadline, borrow_price) VALUES (?, ?, now(), ?, ?)"};
    auto state = sess.sql(sql);
    int book_class_id = data["book_class_id"].asInt();
    int reader_id = data["reader_id"].asInt();
    std::string deadline = data["borrow_deadline"].asString();
    Json::Value respond;
    std::unique_lock lock{::stdout_respond_json_mutex, std::defer_lock};
    try {
        state.bind(book_class_id, reader_id,
            deadline, ::CalculateBorrowPrice(book_class_id, deadline));
        state.execute();
        sql = "update lms.T_BOOK set book_num = book_num - 1 where book_class_id = ?";
        state = sess.sql(sql);
        state.bind(book_class_id);
        state.execute();
        sess.commit();
        respond["is_borrowed"] = true;
        lock.lock();
        ::stdout_respond_json = respond;
        lock.unlock();
        ::stdout_cv.notify_one();
        auto info_logger = spdlog::daily_logger_mt("In BorrowBookImpl func",
                lms::kBorrowInfoLoggerPath);
        info_logger->info("读者id为{}借阅了书籍id为{}的书, 从{}到{}",
            reader_id, book_class_id,
            lms::GetNow(), deadline);
        spdlog::drop(info_logger->name());
    } catch (std::exception &err) {
        sess.rollbackTo(save_point);
        respond["is_borrowed"] = false;
        lock.lock();
        ::stdout_respond_json = respond;
        lock.unlock();
        ::stdout_cv.notify_one();
        auto error_logger = spdlog::daily_logger_mt("In BorrowBookImpl func",
                lms::kBorrowErrorLoggerPath);
        error_logger->error(err.what());
        spdlog::drop(error_logger->name());
    } catch (...) {
        sess.rollbackTo(save_point);
        respond["is_borrowed"] = false;
        lock.lock();
        ::stdout_respond_json = respond;
        lock.unlock();
        ::stdout_cv.notify_one();
        auto error_logger = spdlog::daily_logger_mt("In BorrowBookImpl func",
                lms::kBorrowErrorLoggerPath);
        error_logger->error("未捕获的未知异常");
        spdlog::drop(error_logger->name());
    }
}

void ReturnBookImpl(const Json::Value data) noexcept {
    mysqlx::Session sess{lms::kMysqlxURL};
    sess.startTransaction();
    auto save_point = sess.setSavepoint();
    // 确认是否付款
    mysqlx::string sql{"select borrow_price from lms.T_BORROW_RECORD where book_class_id = ? and reader_id = ? and borrow_deadline = ?"};
    auto state = sess.sql(sql);
    int book_class_id = data["book_class_id"].asInt(), reader_id = data["reader_id"].asInt();
    std::string borrow_deadline = data["borrow_deadline"].asString();
    state.bind(book_class_id, reader_id, borrow_deadline);
    Json::Value respond;
    std::unique_lock lock{::stdout_respond_json_mutex, std::defer_lock};
    try {
        double price = static_cast<double>(state.execute().fetchOne().get(0));
        auto info_logger = spdlog::daily_logger_mt("In ReturnBookImpl func",
                lms::kBorrowInfoLoggerPath);
        if (lms::DoubleIsZero(price)) {
            sql = "delete from lms.T_BORROW_RECORD where book_class_id = ? and reader_id = ? and borrow_deadline = ?";
            state = sess.sql(sql);
            state.bind(book_class_id, reader_id, borrow_deadline);
            state.execute();
            sql = "update lms.T_BOOK set book_num = book_num + 1 where book_class_id = ?";
            state = sess.sql(sql);
            state.bind(book_class_id);
            state.execute();
            sess.commit();
            respond["is_returned"] = true;
            lock.lock();
            ::stdout_respond_json = respond;
            lock.unlock();
            ::stdout_cv.notify_one();
            info_logger->info("读者id为{}归还书籍id为{}",
                reader_id, book_class_id);
        } else {
            respond["is_returned"] = false;
            lock.lock();
            ::stdout_respond_json = respond;
            lock.unlock();
            ::stdout_cv.notify_one();
            info_logger->info("读者id为{}尝试归还书籍id为{}, 却无付款价格{}",
                reader_id, book_class_id, price);
        }
        spdlog::drop(info_logger->name());
    } catch (std::exception &err) {
        sess.setSavepoint(save_point);
        respond["is_returned"] = false;
        lock.lock();
        ::stdout_respond_json = respond;
        lock.unlock();
        ::stdout_cv.notify_one();
        auto error_logger = spdlog::daily_logger_mt("In ReturnBookImpl func",
                lms::kBorrowErrorLoggerPath);
        error_logger->error(err.what());
        spdlog::drop(error_logger->name());
    } catch (...) {
        sess.setSavepoint(save_point);
        respond["is_returned"] = false;
        lock.lock();
        ::stdout_respond_json = respond;
        lock.unlock();
        ::stdout_cv.notify_one();
        auto error_logger = spdlog::daily_logger_mt("In ReturnBookImpl func",
                lms::kBorrowErrorLoggerPath);
        error_logger->error("未捕获的未知异常");
        spdlog::drop(error_logger->name());
    }
}

void PayBookImpl(const Json::Value data) noexcept {
    mysqlx::Session sess{lms::kMysqlxURL};
    mysqlx::string sql{"update lms.T_BORROW_RECORD set borrow_price = 0 where book_class_id = ? and reader_id = ? and borrow_deadline = ?"};
    auto state = sess.sql(sql);
    int book_class_id = data["book_class_id"].asInt(),
        reader_id = data["reader_id"].asInt();
    std::string deadline = data["borrow_deadline"].asString();
    state.bind(book_class_id, reader_id, deadline);
    Json::Value respond;
    std::unique_lock lock{::stdout_respond_json_mutex, std::defer_lock};
    try {
        state.execute();
        respond["is_payed"] = true;
        lock.lock();
        ::stdout_respond_json = respond;
        lock.unlock();
        ::stdout_cv.notify_one();
        auto info_logger = spdlog::daily_logger_mt("In PayBookImpl func",
                lms::kBorrowInfoLoggerPath);
        info_logger->info("用户id为{}付款了书籍id为{}其期限为{}",
            reader_id, book_class_id, deadline);
        spdlog::drop(info_logger->name());
    } catch (std::exception &err) {
        respond["is_payed"] = false;
        lock.lock();
        ::stdout_respond_json = respond;
        lock.unlock();
        ::stdout_cv.notify_one();
        auto error_logger = spdlog::daily_logger_mt("In PayBookImpl func",
                lms::kBorrowErrorLoggerPath);
        error_logger->error(err.what());
        spdlog::drop(error_logger->name());
    } catch (...) {
        respond["is_payed"] = false;
        lock.lock();
        ::stdout_respond_json = respond;
        lock.unlock();
        ::stdout_cv.notify_one();
        auto error_logger = spdlog::daily_logger_mt("In PayBookImpl func",
                lms::kBorrowErrorLoggerPath);
        error_logger->error("未捕获的未知异常");
        spdlog::drop(error_logger->name());
    }
}

void ErrorBorrowImpl(const Json::Value data) noexcept {
    mysqlx::Session sess{lms::kMysqlxURL};
    mysqlx::string sql{"update lms.T_BORROW_RECORD set borrow_price = ?, borrow_deadline = ?, borrow_text = ? where book_class_id = ? and reader_id = ? and borrow_date = ?"};
    auto state = sess.sql(sql);
    double price = data["borrow_price"].asDouble();
    std::string deadline = data["borrow_deadline"].asString(),
        borrow_date = data["borrow_date"].asString(),
        error_text = data["error_text"].asString();
    int book_class_id = data["book_class_id"].asInt(),
        reader_id = data["reader_id"].asInt();
    state.bind(price, deadline, error_text,
        book_class_id, reader_id, borrow_date);
    Json::Value respond;
    std::unique_lock lock{::stdout_respond_json_mutex, std::defer_lock};
    try {
        state.execute();
        respond["is_solved"] = true;
        lock.lock();
        ::stdout_respond_json = respond;
        lock.unlock();
        ::stdout_cv.notify_one();
        auto info_logger = spdlog::daily_logger_mt("In ErrorBorrowImpl func",
                lms::kBorrowInfoLoggerPath);
        info_logger->info(R"cpp(该借阅记录(读者id为{}借阅书籍id为{}从{}到{})其借阅价格被修改为{}
其修改备注为{})cpp", reader_id, book_class_id, borrow_date, deadline, price, error_text);
        spdlog::drop(info_logger->name());
    } catch (std::exception &err) {
        respond["is_solved"] = false;
        lock.lock();
        ::stdout_respond_json = respond;
        lock.unlock();
        ::stdout_cv.notify_one();
        auto error_logger = spdlog::daily_logger_mt("In ErrorBorrowImpl func",
                lms::kBorrowErrorLoggerPath);
        error_logger->error(err.what());
        spdlog::drop(error_logger->name());
    } catch (...) {
        respond["is_solved"] = false;
        lock.lock();
        ::stdout_respond_json = respond;
        lock.unlock();
        ::stdout_cv.notify_one();
        auto error_logger = spdlog::daily_logger_mt("In ErrorBorrowImpl func",
                lms::kBorrowErrorLoggerPath);
        error_logger->error("未捕获的未知异常");
        spdlog::drop(error_logger->name());
    }
}

}

int main(int argc, char *argv[]) {
    spdlog::flush_every(std::chrono::seconds(3));
    cjs::ThreadPool thread_pool{};
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
                thread_pool.SubmitTask(::SearchBorrowListImpl, std::move(json_request_data));
            } else if (action == "borrow") {
                thread_pool.SubmitTask(::BorrowBookImpl, std::move(json_request_data));
            } else if (action == "pay") {
                thread_pool.SubmitTask(::PayBookImpl, std::move(json_request_data));
            } else if (action == "return") {
                thread_pool.SubmitTask(::ReturnBookImpl, std::move(json_request_data));
            } else if (action == "error") {
                thread_pool.SubmitTask(::ErrorBorrowImpl, std::move(json_request_data));
            }
            // 主线程输出结果
            std::unique_lock main_unique_lock(::stdout_respond_json_mutex);
            stdout_cv.wait(main_unique_lock);
            FCGI_printf("Content-type: application/json\r\n"); // 传出数据格式 必须
            FCGI_printf("\r\n"); // 标志响应头结束 必须
            FCGI_printf(stdout_respond_json.toStyledString().c_str());
        } catch (std::exception &err) {
            auto logger = spdlog::daily_logger_mt("In main func",
                    lms::kBorrowErrorLoggerPath);
            logger->error(err.what());
            spdlog::drop(logger->name());
        } catch (...) {
            auto logger = spdlog::daily_logger_mt("In main func",
                    lms::kBorrowErrorLoggerPath);
            logger->error("未知的未捕获的错误");
            spdlog::drop(logger->name());
        }
	}
	return 0;
}
