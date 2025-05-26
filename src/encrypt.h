#pragma once

#include <stdint.h>

#include "lwe.h"
#include "secret.h"

namespace kspir {
/**
 * @brief encrypt an LWE cipertext
 *
 * @param b
 * @param a
 * @param secret
 * @param message
 *
 * 加密一个LWE密文
 * @param b 密文的b部分
 * @param a 密文的a部分
 * @param secret 密钥
 * @param message 要加密的消息
 */
void encrypt(uint64_t *b, uint64_t *a, Secret &secret, uint64_t message);

/**
 * @brief encrypt an RLWE ciphertex
 *
 * @param b return in ntt form
 * @param a return in ntt form
 * @param secret coefficient or ntt form
 * @param message note: shoule be ntt form
 *
 * 加密一个RLWE密文
 * @param b 返回的b部分，NTT形式
 * @param a 返回的a部分，NTT形式
 * @param secret 密钥，系数或NTT形式
 * @param message 消息，应该是NTT形式
 */
void encrypt(uint64_t *b, uint64_t *a, Secret &secret, uint64_t *message);

/**
 * @brief encrypt an RLWE ciphertex
 *
 * @param b1 return in ntt form
 * @param a1 return in ntt form
 * @param b1 another part of CRT, return in ntt form
 * @param a1 another part of CRT, return in ntt form
 * @param secret coefficient or ntt form
 * @param secret1 store in ntt form
 * @param message note: shoule be ntt form
 *
 * 使用CRT（中国剩余定理）加密RLWE密文
 * @param b1 第一个模数下的b部分，返回NTT形式
 * @param a1 第一个模数下的a部分，返回NTT形式
 * @param b2 第二个模数下的b部分，返回NTT形式
 * @param a2 第二个模数下的a部分，返回NTT形式
 * @param secret 密钥，系数或NTT形式
 * @param secret1 密钥的第一部分，NTT形式
 * @param message 消息，应该是NTT形式
 */
void encrypt(uint64_t *b1, uint64_t *a1, uint64_t *b2, uint64_t *a2,
             Secret &secret, uint64_t *message, uint64_t *secret1,
             uint64_t *secret2, intel::hexl::NTT &ntts1,
             intel::hexl::NTT &ntts2, uint64_t modulus1, uint64_t modulus2);

/**
 * @brief note that the message will be crypted as: message * bigMod
 *
 * @param cipher1
 * @param cipher2
 * @param secret
 * @param message
 *
 * RNS（剩余数系统）加密
 * 注意：消息将被加密为: message * bigMod
 * @param cipher1 第一个模数下的密文
 * @param cipher2 第二个模数下的密文
 * @param secret 密钥
 * @param message 消息
 */
void encrypt_rns(RlweCiphertext &cipher1, RlweCiphertext &cipher2,
                 Secret &secret, std::vector<uint64_t> &message);

/**
 * 双模数下的RNS加密
 * 用于同时加密两个不同的消息
 */
void encrypt_rns(std::vector<uint64_t> &b1, std::vector<uint64_t> &a1,
                 std::vector<uint64_t> &b2, std::vector<uint64_t> &a2,
                 Secret &secret, std::vector<uint64_t> &message1,
                 std::vector<uint64_t> &message2, int32_t num = 0,
                 uint64_t modulus2 = bigMod2);

/**
 * 为BSGS自同态密钥生成RNS加密
 * 使用BSGS（Baby-Step Giant-Step）算法优化
 */
void encrypt_rns_bsgs_autokey(std::vector<uint64_t> &b1,
                              std::vector<uint64_t> &a1,
                              std::vector<uint64_t> &b2,
                              std::vector<uint64_t> &a2, Secret &secret,
                              std::vector<uint64_t> &message1,
                              std::vector<uint64_t> &message2,
                              uint64_t modulus2);

/**
 * @brief used to remove term 2^l. (lazy variant)
 * Refer to `4.1 Removing the Leading Term N' in [].
 *
 *
 * @param b
 * @param a
 * @param secret
 * @param message
 * @param lwesnum
 *
 * 用于移除项2^l的特殊RLWE加密（惰性变体）
 * 参考论文 `4.1 Removing the Leading Term N'`
 * @param b 密文的b部分
 * @param a 密文的a部分
 * @param secret 密钥
 * @param message 消息
 * @param lwesnum LWE数量
 */
void encrypt_special_rlwe(uint64_t *b, uint64_t *a, Secret &secret,
                          uint64_t *message, int32_t lwesnum);

/**
 * 使用RLWE密文类进行加密
 * 简化的加密接口
 */
void encrypt(RlweCiphertext &cipher, Secret &secret,
             std::vector<uint64_t> &message);

/**
 * @brief encrypt for rns bsgs algorithm
 * @param cipher encrypted cipher
 * @param secret secret
 * @param message stored in coefficients form
 *
 * 针对BSGS算法的RNS加密
 * @param cipher 加密后的密文
 * @param secret 密钥
 * @param message 消息，以系数形式存储
 */
void encrypt_rns_bsgs(std::vector<RlweCiphertext> &cipher, Secret &secret,
                      std::vector<uint64_t> &message);

/**
 * @brief
 *
 * @param b
 * @param a
 * @param secret
 * @return uint64_t
 *
 * 解密LWE密文
 * @param b 密文的b部分
 * @param a 密文的a部分
 * @param secret 密钥
 * @return 解密后的消息
 */
uint64_t decrypt(uint64_t *b, uint64_t *a, Secret &secret);

/**
 * @brief decrypt an RLWE ciphertext
 *
 * @param message
 * @param b
 * @param a
 * @param secret
 *
 * 解密RLWE密文
 * @param message 解密后的消息
 * @param b 密文的b部分
 * @param a 密文的a部分
 * @param secret 密钥
 */
void decrypt(uint64_t *message, uint64_t *b, uint64_t *a, Secret &secret);

/**
 * 使用RLWE密文类进行解密
 * 简化的解密接口
 */
void decrypt(std::vector<uint64_t> &message, RlweCiphertext &cipher,
             Secret &secret);

/**
 * @brief decrypt an RLWE ciphertext in another modulus
 *
 * @param message
 * @param b
 * @param a
 * @param secret
 * @param modulus2 the second modulus
 *
 * 在不同模数下解密RLWE密文
 * @param message 解密后的消息
 * @param b 密文的b部分
 * @param a 密文的a部分
 * @param secret 密钥
 * @param modulus2 第二个模数
 */
void decrypt(uint64_t *message, uint64_t *b, uint64_t *a, Secret &secret,
             uint64_t modulus2);
} // namespace kspir