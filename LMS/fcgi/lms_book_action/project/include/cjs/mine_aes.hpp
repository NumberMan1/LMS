#ifndef CJS_MINE_AES_HPP
#define CJS_MINE_AES_HPP

#include <openssl/aes.h>
#include <string>
#include <stdexcept>

namespace cjs {

namespace openssl {

class AES {
private:
    std::string key_;
    AES_KEY enckey_,
            deckey_;
public:
    enum class Pattern {
        kCBC
    };
    // 如果给定的key的长度不为16,24,32则抛出std::runtime_error
    AES(std::string_view key) : key_(key) {
        if (key_.size() == 16 ||
            key_.size() == 24 ||
            key_.size() == 32) {
            AES_set_encrypt_key(
                reinterpret_cast<unsigned char*>(key_.data()),
                key_.size() * 8, &enckey_
            );
            AES_set_decrypt_key(
                reinterpret_cast<unsigned char*>(key_.data()),
                key_.size() * 8, &deckey_
            );
        } else {
            throw std::runtime_error{"传递给AES的key长度不正确"};
        }
    }
    // 需要类似于stl的接口: data(), size()
    template<typename T>
    inline std::string Encrypt(T &&datas, const Pattern &pattern) const {
        return Crypto<Pattern::kCBC>(std::forward<T>(datas), AES_ENCRYPT);
    }
    // 需要类似于stl的接口: data(), size()
    template<typename T>
    inline std::string Decrypt(T &&datas, const Pattern &pattern) const {
        return Crypto<Pattern::kCBC>(std::forward<T>(datas), AES_DECRYPT);
    }
private:
    template<Pattern pattern, typename T>
    std::string Crypto(T datas, bool flag) const {
        AES_KEY *key = const_cast<AES_KEY*>(flag == AES_ENCRYPT ? &enckey_ : &deckey_);
    	unsigned char ivec[AES_BLOCK_SIZE];
    	int length = datas.size() + 1;	// +1 是添加\0的长度
    	if (length % 16) {
    		length = (length / 16 + 1) * 16;
    	}
        try {
	        char* out = new char[length];
        	GenerateIvec(ivec);
            if constexpr(pattern == Pattern::kCBC) {
                AES_cbc_encrypt(
                    reinterpret_cast<unsigned char*>(datas.data()),
                    reinterpret_cast<unsigned char*>(out),
                    length, key, ivec, flag
                );
            }
            std::string result = std::string{out};
            delete[] out;
            return result;
        } catch (std::bad_alloc &err) {
            throw err;
        }
    }
    void GenerateIvec(unsigned char *ivec) const noexcept {
    	for (std::size_t i = 0; i < AES_BLOCK_SIZE; ++i) {
    		ivec[i] = key_.at(AES_BLOCK_SIZE - i - 1);
    	}
    }
};
    
}

} // namespace cjs
    
#endif