#ifndef CJS_MINE_RSA_HPP
#define CJS_MINE_RSA_HPP

#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/err.h>
#include <iostream>

#include "mine_base64.hpp"

namespace cjs {

namespace openssl {

class MyRSA {
public:
    enum class SignType {
        kSHA256Type,
        kSHA384Type,
        kSHA512Type
    };
    static constexpr int kKeyBitsLen = 1024;
    static constexpr std::size_t kBigNumW = 12345;
    // 用于生成n * 1024的bits的密钥
    explicit MyRSA(const int &n = 1)
        : _impl_(RSAImpl()) {
        RSAPtr rsa_ptr(RSA_new(), RSA_free);
        BNPtr big_num_ptr(BN_new(), BN_free);
        BN_set_word(big_num_ptr.get(), kBigNumW);
        RSA_generate_key_ex(rsa_ptr.get(), kKeyBitsLen * n, 
                            big_num_ptr.get(), nullptr);
        _impl_.pub_key_ptr_ = RSAPublicKey_dup(rsa_ptr.get());
        _impl_.pri_key_ptr_ = RSAPrivateKey_dup(rsa_ptr.get());
    }
    // 用于初始化以拥有的密钥
    explicit MyRSA(std::string_view pub_key_path,
                   std::string_view pri_key_path)
        : _impl_(RSAImpl()) {
        try {
            Init(pub_key_path, pri_key_path);
        } catch (std::runtime_error &err) {
            std::cerr << err.what() << std::endl;
        }
    }
    explicit MyRSA(std::string_view pub_key_path)
        : _impl_(RSAImpl()),
          only_pubkey_(true) {
        try {
            InitPubImpl(pub_key_path);
        } catch (std::runtime_error &err) {
            std::cerr << err.what() << std::endl;
        }
    }
    ~MyRSA() = default;
    // 生成密钥对
    inline void Init(std::string_view pub_key_path,
                     std::string_view pri_key_path) {
        if (!_impl_.pub_key_ptr_) { // 用于清除之前初始化的数据
            RSA_free(_impl_.pub_key_ptr_);
            _impl_.pub_key_ptr_ = nullptr;
        }
        if (!_impl_.pri_key_ptr_) {
            RSA_free(_impl_.pri_key_ptr_);
            _impl_.pri_key_ptr_ = nullptr;
        }
        // 开始工作
        InitPubImpl(pub_key_path);
        InitPriImpl(pri_key_path);
    }
    // 储存密钥对
    // 失败抛出std::runtime_error
    inline void SaveRSAKey(std::string_view pub_key_path,
                           std::string_view pri_key_path) const {
        if (only_pubkey_) {
            throw std::runtime_error("只有公钥");
        }
        BIOPtr bio_pub_key_file_ptr(BIO_new_file(pub_key_path.data(), "w"), BIO_free);
        BIOPtr bio_pri_key_file_ptr(BIO_new_file(pri_key_path.data(), "w"), BIO_free);
        PEM_write_bio_RSAPublicKey(bio_pub_key_file_ptr.get(), _impl_.pub_key_ptr_);
        PEM_write_bio_RSAPrivateKey(bio_pri_key_file_ptr.get(), _impl_.pri_key_ptr_,
            nullptr, nullptr, 0, nullptr, nullptr);
    }
    // 公钥加密, 每次最多密钥长度-11
    // 需要stl类似的接口: data(), size()
    // 失败抛出std::runtime_error
    template<typename T>
    std::string EncryptPubKey(const T &datas, bool to_base64 = true) const {
        const int len = RSA_size(_impl_.pub_key_ptr_);
        int flag;
        // char *temp = new char[len] {0};
        std::string result;
        result.resize(len);
        flag = RSA_public_encrypt(datas.size(),
            reinterpret_cast<const unsigned char*>(datas.data()),
            // reinterpret_cast<unsigned char*>(temp),
            reinterpret_cast<unsigned char*>(result.data()),
            _impl_.pub_key_ptr_,
            RSA_PKCS1_PADDING);
        // std::string result(temp, len);
        // delete[] temp;
        if (flag == -1) {
            ERR_print_errors_fp(stdout);
            throw std::runtime_error("公钥加密时出错");
        }
        if (to_base64) {
            return ToBase64(result);
        } else {
            return result;
        }
    }
    // 私钥解密, 每次最多密钥长度-11
    // 需要stl类似的接口: data(), size()
    // 失败抛出std::runtime_error
    template<typename T>
    std::string DecryptPriKey(const T &datas, bool from_base64 = true) const {
        if (only_pubkey_) {
            throw std::runtime_error("只有公钥");
        }
        const int len = RSA_size(_impl_.pri_key_ptr_);
        int flag;
        // char *temp = new char[len] {0};
        std::string result;
        result.resize(len);
        if (from_base64) {
            std::string temp = FromBase64(datas);
            flag = RSA_private_decrypt(temp.size(),
                reinterpret_cast<const unsigned char*>(temp.data()),
                // reinterpret_cast<unsigned char*>(temp),
                reinterpret_cast<unsigned char*>(result.data()),
                _impl_.pri_key_ptr_,
                RSA_PKCS1_PADDING);
        } else {
            flag = RSA_private_decrypt(datas.size(),
                reinterpret_cast<const unsigned char*>(datas.data()),
                // reinterpret_cast<unsigned char*>(temp),
                reinterpret_cast<unsigned char*>(result.data()),
                _impl_.pri_key_ptr_,
                RSA_PKCS1_PADDING);
            // std::string result(temp, len);
        }
        if (flag == -1) {
            ERR_print_errors_fp(stdout);
            throw std::runtime_error("私钥解密时出错");
        }
        // delete[] temp;
        return result;
    }
    // 数据签名
    // 需要stl类似的接口: data(), size()
    // 如果只有公钥抛出std::runtime_error
    template<typename T>
    std::string Sign(const T &datas, SignType type, bool to_base64 = true) const {
        if (only_pubkey_) {
            throw std::runtime_error("只有公钥");
        }
        const int len = RSA_size(_impl_.pri_key_ptr_);
        // unsigned char *temp = new unsigned char[len] {0};
        std::string result;
        result.resize(len);
        unsigned out_len = 0;
        int flag;
        switch (type) {
        case SignType::kSHA256Type:
            flag = RSA_sign(NID_sha256,
                reinterpret_cast<const unsigned char*>(datas.data()),
                datas.size(),
                // temp,
                reinterpret_cast<unsigned char*>(result.data()),
                &out_len,
                _impl_.pri_key_ptr_);
            break;
        case SignType::kSHA384Type:
            flag = RSA_sign(NID_sha384,
                reinterpret_cast<const unsigned char*>(datas.data()),
                datas.size(),
                // temp,
                reinterpret_cast<unsigned char*>(result.data()),
                &out_len,
                _impl_.pri_key_ptr_);
            break;
        case SignType::kSHA512Type:
            flag = RSA_sign(NID_sha512,
                reinterpret_cast<const unsigned char*>(datas.data()),
                datas.size(),
                // temp,
                reinterpret_cast<unsigned char*>(result.data()),
                &out_len,
                _impl_.pri_key_ptr_);
            break;
        default:
            break;
        }
        if (flag == -1) {
            ERR_print_errors_fp(stdout);
            throw std::runtime_error("签名时出错");
        }
        result.resize(out_len);
        // std::string result(reinterpret_cast<char*>(temp), out_len);
        // delete[] temp;
        if (to_base64) {
            return ToBase64(result);
        } else {
            return result;
        }
    }
    // 验证签名
    // 需要stl类似的接口: data(), size()
    template<typename T1, typename T2>
    bool Verify(T1 &&datas,
                T2 &&sign_datas,
                SignType type,
                bool from_base64 = true) const {
        if (from_base64) {
            return VerifyImpl(std::forward<T1>(datas),
                FromBase64(sign_datas), type);
        } else {
            return VerifyImpl(std::forward<T1>(datas),
                std::forward<T2>(sign_datas), type);
        }
        
    }
private:
    using BIOPtr = std::unique_ptr<BIO, decltype(BIO_free)*>;
    using RSAPtr = std::unique_ptr<RSA, decltype(RSA_free)*>;
    using BNPtr = std::unique_ptr<BIGNUM, decltype(BN_free)*>;
    struct RSAImpl {
        RSA* pub_key_ptr_ = nullptr;
        RSA* pri_key_ptr_ = nullptr;
        ~RSAImpl() {
            if (!pub_key_ptr_) {
                RSA_free(pub_key_ptr_);
            }
            if (!pri_key_ptr_) {
                RSA_free(pri_key_ptr_);
            }
        }
    };
    inline void InitPubImpl(std::string_view file_name) {
        _impl_.pub_key_ptr_ = RSA_new();
        BIOPtr bio_pub_key_file_ptr(BIO_new_file(file_name.data(), "r"), BIO_free);
        if (PEM_read_bio_RSAPublicKey(bio_pub_key_file_ptr.get(),
                &_impl_.pub_key_ptr_, nullptr, nullptr)
            == nullptr) {
            ERR_print_errors_fp(stdout);
            throw std::runtime_error("bio读取公钥时出错");
        }
    }
    inline void InitPriImpl(std::string_view file_name) {
        _impl_.pri_key_ptr_ = RSA_new();
        BIOPtr bio_pri_key_file_ptr(BIO_new_file(file_name.data(), "r"), BIO_free);
        if (PEM_read_bio_RSAPrivateKey(bio_pri_key_file_ptr.get(),
                &_impl_.pri_key_ptr_, nullptr, nullptr)
            == nullptr) {
            ERR_print_errors_fp(stdout);
            throw std::runtime_error("bio读取私钥时出错");
        }
    }
    template<typename T1, typename T2>
    inline bool VerifyImpl(T1 datas, T2 sign_datas, SignType type) const {
        int flag;
        switch (type) {
        case SignType::kSHA256Type:
            flag = RSA_verify(NID_sha256,
                reinterpret_cast<const unsigned char*>(datas.data()),
                datas.size(),
                reinterpret_cast<const unsigned char*>(sign_datas.data()),
                sign_datas.size(),
                _impl_.pub_key_ptr_);
            break;
        case SignType::kSHA384Type:
            flag = RSA_verify(NID_sha384,
                reinterpret_cast<const unsigned char*>(datas.data()),
                datas.size(),
                reinterpret_cast<const unsigned char*>(sign_datas.data()),
                sign_datas.size(),
                _impl_.pub_key_ptr_);
            break;
        case SignType::kSHA512Type:
            flag = RSA_verify(NID_sha512,
                reinterpret_cast<const unsigned char*>(datas.data()),
                datas.size(),
                reinterpret_cast<const unsigned char*>(sign_datas.data()),
                sign_datas.size(),
                _impl_.pub_key_ptr_);
            break;
        }
        if (flag == -1) {
            ERR_print_errors_fp(stdout);
            throw std::runtime_error("效验签名时出错");
        } else {
            return flag;
        }
    }
    RSAImpl _impl_;
    bool only_pubkey_ = false;
};

}

} // namespace cjs

#endif // ! CJS_MINE_RSA_HPP
