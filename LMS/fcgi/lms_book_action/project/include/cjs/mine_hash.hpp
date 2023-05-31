#ifndef CJS_MINE_HASH_HPP
#define CJS_MINE_HASH_HPP

#include <openssl/evp.h>
#include <openssl/sha.h>
#include <string_view>
#include <memory>


namespace cjs {

namespace openssl {

enum HashLength : std::size_t {
    kSHA256Len = SHA256_DIGEST_LENGTH,
    kSHA384Len = SHA384_DIGEST_LENGTH,
    kSHA512Len = SHA512_DIGEST_LENGTH
};
enum class HashType {
    kSHA256Type,
    kSHA384Type,
    kSHA512Type
};

class Hash {
public:
    Hash() = delete;
    explicit Hash(const HashType &type)
        : _impl_(HashImpl()) {
        _impl_.hash_type_ = type;
        _impl_.Init();
    }
    virtual ~Hash() = default;
    inline const HashType& type() const noexcept {
        return _impl_.hash_type_;
    }
    /* 添加hash的参数，可以重复添加例如：
     *  hello 
     *  , world 
     * 需要类似stl的接口: data(), size()*/
    template<typename T>
    inline void Update(const T &datas) noexcept {
        EVP_DigestUpdate(_impl_.md_ctx_.get(), datas.data(), datas.size());
    }
    // 返回计算结果, 使用后该结构体将重置为初始状态
    inline std::string Final() noexcept {
        using type = HashType;
        using len = HashLength;
        // unsigned length = 0;
        std::string result_str;
        switch (_impl_.hash_type_) {
        case type::kSHA256Type: {
            // std::array<unsigned char, len::kSHA256Len> datas{0};
            // EVP_DigestFinal_ex(_impl_.md_ctx_.get(), datas.data(), &length);
            // EVP_DigestInit_ex(_impl_.md_ctx_.get(), EVP_sha256(), nullptr);
            // std::array<char, len::kSHA256Len * 2 + 1> result_arr{0};
            // for (unsigned i = 0; i != length; ++i) {
            //     sprintf(&result_arr[i * 2], "%02x", datas[i]);
            // }
            // std::string result_str(result_arr.data(), length * 2 + 1);
            FinalImpl<len::kSHA256Len>(result_str);
            break;
        }
        case type::kSHA384Type: {
            // std::array<unsigned char, len::kSHA384Len> datas{0};
            // EVP_DigestFinal_ex(_impl_.md_ctx_.get(), datas.data(), &length);
            // EVP_DigestInit_ex(_impl_.md_ctx_.get(), EVP_sha384(), nullptr);
            // std::array<char, len::kSHA384Len * 2 + 1> result_arr{0};
            // for (unsigned i = 0; i != length; ++i) {
            //     sprintf(&result_arr[i * 2], "%02x", datas[i]);
            // }
            // std::string result_str(result_arr.data(), length * 2 + 1);
            FinalImpl<len::kSHA384Len>(result_str);
            break;
        }
        case type::kSHA512Type: {
            // std::array<unsigned char, len::kSHA512Len> datas{0};
            // EVP_DigestFinal_ex(_impl_.md_ctx_.get(), datas.data(), &length);
            // EVP_DigestInit_ex(_impl_.md_ctx_.get(), EVP_sha512(), nullptr);
            // std::array<char, len::kSHA512Len * 2 + 1> result_arr{0};
            // for (unsigned i = 0; i != length; ++i) {
            //     sprintf(&result_arr[i * 2], "%02x", datas[i]);
            // }
            // std::string result_str(result_arr.data(), length * 2 + 1);
            FinalImpl<len::kSHA512Len>(result_str);
            break;
        }
        default:
            break;
        }
        return result_str;
    }

private:
    struct HashImpl {
        HashType hash_type_;
        using EVPMDCTXPtr = std::unique_ptr<EVP_MD_CTX, decltype(EVP_MD_CTX_free)*>;
        EVPMDCTXPtr md_ctx_ {EVP_MD_CTX_new(), EVP_MD_CTX_free};
        // std::variant<SHA256_CTX, SHA512_CTX> sha_ctx_;
        inline void Init() noexcept {
            using type = HashType;
            switch (hash_type_) {
            case type::kSHA256Type:
                // sha_ctx_ = SHA256_CTX();
                // SHA256_Init(&std::get<SHA256_CTX>(sha_ctx_));
                EVP_DigestInit_ex(md_ctx_.get(), EVP_sha256(), nullptr);
                break;
            case type::kSHA384Type:
                // sha_ctx_ = SHA256_CTX();
                // SHA256_Init(&std::get<SHA256_CTX>(sha_ctx_));
                EVP_DigestInit_ex(md_ctx_.get(), EVP_sha384(), nullptr);
                break;
            case type::kSHA512Type:
                // sha_ctx_ = SHA512_CTX();
                // SHA512_Init(&std::get<SHA512_CTX>(sha_ctx_));
                EVP_DigestInit_ex(md_ctx_.get(), EVP_sha512(), nullptr);
                break;
            default:
                break;
            }
        }
    };
    template<HashLength len>
    void FinalImpl(std::string &s) {
        unsigned l = 0;
        std::array<unsigned char, len> datas{0};
        EVP_DigestFinal_ex(_impl_.md_ctx_.get(), datas.data(), &l);
        switch (len) {
        case HashLength::kSHA256Len:
            EVP_DigestInit_ex(_impl_.md_ctx_.get(), EVP_sha256(), nullptr);
            break;
        case HashLength::kSHA384Len:
            EVP_DigestInit_ex(_impl_.md_ctx_.get(), EVP_sha384(), nullptr);
            break;
        case HashLength::kSHA512Len:
            EVP_DigestInit_ex(_impl_.md_ctx_.get(), EVP_sha512(), nullptr);
            break;
        default:
            return;
        }
        std::array<char, len * 2 + 1> result_arr{0};
        for (unsigned i = 0; i != l; ++i) {
            std::sprintf(&result_arr[i * 2], "%02x", datas[i]);
        }
        s.append(result_arr.data(), l * 2 + 1);
    }
    HashImpl _impl_;
};

}

} // ! namespace cjs

#endif // ! CJS_MINE_HASH_HPP