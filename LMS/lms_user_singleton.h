#ifndef LMS_LMS_USER_SINGLETON_H
#define LMS_LMS_USER_SINGLETON_H

#include <string>

#include "lms_config.h"

namespace lms {

// 用户单例类
class UserSingleton {
public:
    static inline UserSingleton* Instance() noexcept {
        return &instance_;
    }
    explicit UserSingleton(const UserSingleton&) = delete;
    explicit UserSingleton(UserSingleton&&) = delete;
    UserSingleton& operator=(const UserSingleton&) = delete;
    UserSingleton& operator=(UserSingleton&&) = delete;

    inline void SetType(const UserType& type) {
        type_ = type;
    }
    inline UserType GetType() {
        return type_;
    }
    inline void SetLoginName(const std::string& login_name) {
        login_name_ = login_name;
    }
    inline std::string GetLoginName() {
        return login_name_;
    }
private:
    explicit UserSingleton() = default;
    UserType type_;
    std::string login_name_;
    static UserSingleton instance_;
};

}

#endif // !LMS_LMS_USER_SINGLETON_H
