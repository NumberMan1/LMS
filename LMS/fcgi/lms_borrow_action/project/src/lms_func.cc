#include "lms_func.h"

namespace lms {

void SearchBindArgs(const Json::Value &value, mysqlx::SqlStatement *state,
        bool type_is_all, int arg_num) noexcept {
    std::string text{"%"};
    text += value["text"].asString();
    text += '%';
    if (type_is_all) {
        switch (arg_num) {
        case 1:
            state->bind((value["page"].asInt() - 1) * 10);
            break;
        case 2:
            state->bind(text, (value["page"].asInt() - 1) * 10);
            break;
        case 3:
            state->bind(text, text, (value["page"].asInt() - 1) * 10);
            break;
        case 4:
            state->bind(text, text, text, (value["page"].asInt() - 1) * 10);
            break;
        }
    } else {
        switch (arg_num) {
        case 1:
            state->bind(value["type"].asInt(), (value["page"].asInt() - 1) * 10);
            break;
        case 2:
            state->bind(value["type"].asInt(), text, (value["page"].asInt() - 1) * 10);
            break;
        case 3:
            state->bind(value["type"].asInt(), text, text, (value["page"].asInt() - 1) * 10);
            break;
        case 4:
            state->bind(value["type"].asInt(), text, text, text, (value["page"].asInt() - 1) * 10);
            break;
        }
    }
}

void SearchBindArgs(const Json::Value &value, mysqlx::SqlStatement *state,
        int arg_num) noexcept {
    std::string text{"%"};
    text += value["text"].asString();
    text += '%';
    switch (arg_num) {
    case 1:
        state->bind((value["page"].asInt() - 1) * 10);
        break;
    case 2:
        state->bind(text, (value["page"].asInt() - 1) * 10);
        break;
    case 3:
        state->bind(text, text, (value["page"].asInt() - 1) * 10);
        break;
    case 4:
        state->bind(text, text, text, (value["page"].asInt() - 1) * 10);
        break;
    }
}

bool DoubleIsZero(double num) noexcept {
    if (std::abs(num) < 1e-8) {
        return true;
    } else {
        return false;
    }
}

std::string GetNow() noexcept {
    using namespace std::chrono;
    auto &&time = system_clock::now();
    time_t &&tt = system_clock::to_time_t(time);
    auto &&time_tm = localtime(&tt);
    std::string str;
    str.resize(10);
    std::sprintf(str.data(), "%d-%02d-%02d",
        time_tm->tm_year + 1900, time_tm->tm_mon + 1, time_tm->tm_mday);
    return str;
}
    
} // namespace lms