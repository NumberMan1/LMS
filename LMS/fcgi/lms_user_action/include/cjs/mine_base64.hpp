#ifndef MINE_BASE64_HPP
#define MINE_BASE64_HPP

#include <openssl/bio.h>
#include <openssl/evp.h>
#include <string>
#include <memory>

namespace cjs {

namespace openssl {

// 将数据转换为base64格式
// 需要有类似stl的接口: size(), data()
template<typename T>
std::string ToBase64(const T &datas) {
    using BIOPtr = std::unique_ptr<BIO, decltype(BIO_free)*>;
    BIOPtr b64(nullptr, BIO_free), mem(nullptr, BIO_free);
    b64.reset(BIO_new(BIO_f_base64()));
    mem.reset(BIO_new(BIO_s_mem()));
    BIO_push(b64.get(), mem.get());
    BIO_write(b64.get(), datas.data(), datas.size());
    BIO_flush(b64.get());
    BUF_MEM *buf_ptr = nullptr;
    BIO_get_mem_ptr(b64.get(), &buf_ptr);
    std::string result(buf_ptr->data, buf_ptr->length - 1);
    return result;
}

// 以base64格式转换为相应数据
// 需要有类似stl的接口: size(), data()
template<typename T>
std::string FromBase64(const T &datas) {
    using BIOPtr = std::unique_ptr<BIO, decltype(BIO_free)*>;
    BIOPtr b64(nullptr, BIO_free), mem(nullptr, BIO_free);
    b64.reset(BIO_new(BIO_f_base64()));
    mem.reset(BIO_new(BIO_s_mem()));
    BIO_write(mem.get(), datas.data(), datas.size());
    BIO_push(b64.get(), mem.get());
    std::string result;
    result.resize(datas.size());
    int true_len = BIO_read(b64.get(), result.data(), result.size());
    result.resize(true_len);
    return result;
}

}

} // namespace cjs

#endif