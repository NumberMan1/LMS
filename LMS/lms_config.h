#ifndef LMS_LMS_CONFIG_H
#define LMS_LMS_CONFIG_H

namespace lms {

enum class UserType {
    kManager,
    kBookReader,
};

constexpr auto kHostName{"10.62.0.135"};
constexpr auto kDataBaseName{"lms"};

}

#endif // !LMS_LMS_CONFIG_H