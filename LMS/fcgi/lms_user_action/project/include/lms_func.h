#ifndef LMS_LMS_FUNC_H
#define LMS_LMS_FUNC_H

#include <mysqlx/xdevapi.h>
#include <json/json.h>

namespace lms {

void SearchBindArgs(const Json::Value &value,
        mysqlx::SqlStatement *state, bool type_is_all, int arg_num);

void SearchBindArgs(const Json::Value &value,
        mysqlx::SqlStatement *state, int arg);

}

#endif