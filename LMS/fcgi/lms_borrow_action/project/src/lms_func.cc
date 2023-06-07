#include "lms_func.h"

namespace lms {

void SearchBindArgs(const Json::Value &value,
        mysqlx::SqlStatement *state, bool type_is_all, int arg_num) {
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

void SearchBindArgs(const Json::Value &value,
        mysqlx::SqlStatement *state, int arg_num) {
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

bool DoubleIsZero(double num) {
    if (std::abs(num) < 1e-8) {
        return true;
    } else {
        return false;
    }
}
    
} // namespace lms