#pragma once

#include <stddef.h>
#include <stdint.h>
#include <vector>

#include "lwe.h"
#include "secret.h"

namespace kspir {
/**
 * 生成BSGS算法使用的随机数据库
 * 每个元素满足特定范围条件
 */
void sample_database_bsgs(std::vector<std::vector<uint64_t>> &data);

/**
 * @brief database to bsgs ntt form
 * @param data input database
 * @param modulus
 *
 * 将数据库转换为BSGS算法所需的NTT形式
 * @param result 结果数组
 * @param data 输入数据库
 * @param modulus 使用的模数
 * @param N1 参数N1，默认为N/2
 */
void database_tobsgsntt(std::vector<std::vector<uint64_t>> &result,
                        std::vector<std::vector<uint64_t>> &data,
                        uint64_t modulus, int32_t N1 = N / 2);

/**
 * 逆向编码消息
 * 将NTT域的数据转换回时域并应用特定排列
 */
void inverse_encode(std::vector<uint64_t> &message);

/**
 * 生成BSGS算法使用的查询密文
 * @param cipher 输出的密文
 * @param queryKey 查询密钥
 * @param row 要查询的行
 */
void query_bsgs(RlweCiphertext &cipher, Secret &queryKey, uint64_t row);

/**
 * 生成RNS版本的BSGS查询密文
 * 使用剩余数系统优化大整数运算
 */
void query_bsgs_rns(std::vector<RlweCiphertext> &cipher, Secret &queryKey,
                    uint64_t row);

/**
 * 解密BSGS算法的结果密文
 * @param message 解密后的消息
 * @param cipher 输入密文
 * @param secret 解密密钥
 */
void decrypt_bsgs(std::vector<uint64_t> &message, RlweCiphertext &cipher,
                  Secret &secret);

/**
 * 解密BSGS总体协议中的结果密文
 * 处理多个RLWE密文的情况
 */
void decrypt_bsgs_total(std::vector<uint64_t> &message, RlweCiphertext &cipher,
                        Secret &secret, const int32_t rlwesNum);

/**
 * 在RNS-CRT表示下评估自同态变换
 * 结合RNS和CRT进行高效运算
 */
void evalAutoRNSCRT(RlweCiphertext &result,
                    std::vector<std::vector<uint64_t>> &input, const int32_t N2,
                    const AutoKeyBSGSRNS &autokey, const int32_t ellnum,
                    const uint64_t PP, const uint64_t BBg);

/**
 * 重排密文向量
 * 用于准备进行矩阵-向量乘法
 */
void reorientCipher(uint64_t *cipherbuf, RlweCiphertext &input,
                    std::vector<RlweCiphertext> &rotated_cipher, int32_t N1);

/**
 * 执行矩阵-向量乘法
 * 计算加密数据库与查询向量的乘积
 */
void matrix_vector_mul(RlweCiphertext &result, RlweCiphertext &input,
                       std::vector<std::vector<uint64_t>> &data,
                       const AutoKeyBSGS &autoKey);

/**
 * 使用BSGS算法执行矩阵-向量乘法
 * 相比普通乘法具有更优的时间复杂度
 */
void matrix_vector_mul_bsgs(RlweCiphertext &result, RlweCiphertext &input,
                            std::vector<std::vector<uint64_t>> &data,
                            const AutoKeyBSGS &autoKey, const int32_t N1);

/**
 * 使用BSGS算法结合CRT执行矩阵-向量乘法
 * 进一步优化大规模数据库操作
 */
void matrix_vector_mul_bsgs_crt(RlweCiphertext &result, RlweCiphertext &input,
                                uint64_t *data, const AutoKeyBSGS &autoKey,
                                const int32_t N1);

/**
 * 使用RNS-BSGS算法结合CRT执行矩阵-向量乘法
 * 综合应用多种优化技术
 */
void matrix_vector_mul_bsgs_rns_crt(
    RlweCiphertext &result, std::vector<RlweCiphertext> &input, uint64_t *data,
    const AutoKeyBSGSRNS &autoKey, const int32_t N1,
    std::vector<std::vector<int32_t>> &permutations);

/**
 * 用于大规模问题的RNS-BSGS-CRT矩阵-向量乘法
 * 支持多重递归处理
 */
void matrix_vector_mul_bsgs_rns_crt_large(
    std::vector<RlweCiphertext> &result, std::vector<RlweCiphertext> &input,
    uint64_t *data, const AutoKeyBSGSRNS &autoKey, const int32_t N1,
    std::vector<std::vector<int32_t>> &permutations, int32_t r = 1);

/**
 * 从离线密钥生成在线自同态密钥
 * 用于提高密钥生成效率
 * @param result 结果密钥
 * @param result1 第一个辅助结果密钥
 * @param result2 第二个辅助结果密钥
 * @param offlineKey1 第一个离线密钥
 * @param offlineKey2 第二个离线密钥
 * @param N1 参数N1
 */
void genAutoKeyFromOffline(AutoKeyBSGSRNS &result, AutoKeyBSGSRNS &result1,
                           AutoKeyBSGSRNS &result2,
                           const AutoKeyBSGSRNS &offlineKey1,
                           const AutoKeyBSGSRNS &offlineKey2, const int32_t N1);
} // namespace kspir
