#pragma once

#include "params.h"
#include <stddef.h>
#include <stdint.h>
#include <string>
#include <vector>

#ifdef INTEL_HEXL
#include <hexl/hexl.hpp>
#endif

namespace kspir {
/**
 * 复制向量（uint64_t到int64_t）
 * 将无符号整数向量复制到有符号整数向量
 */
void copy_vector(int64_t *result, uint64_t *a);

/**
 * 复制向量（int64_t到uint64_t）
 * 将有符号整数向量复制到无符号整数向量，应用模数
 */
void copy_vector(uint64_t *result, int64_t *a, uint64_t modulus);

/**
 * 通用复制向量模板函数
 * 支持不同类型间的向量复制
 */
template <typename T1, typename T2> void copy_vector(T1 *result, T2 *a);

/**
 * 显示大型向量内容（模板版本）
 * 用于调试输出
 */
template <typename T> void showLargeVector(std::vector<T> &vals);

/**
 * 显示大型无符号向量内容
 * 带有可选的标题字符串
 */
void showLargeVector(std::vector<uint64_t> &vals, std::string ss = "");

/**
 * 按间隔显示大型向量内容
 * 每隔指定间隔输出一个元素
 */
void showLargeIntervalVector(std::vector<uint64_t> &vals, int32_t interval,
                             std::string ss = "");

/**
 * 计算Q的逆
 * 基于LWE数量计算特定常数
 */
uint64_t QInv(int lwenum, uint64_t modulus = bigMod);

/**
 * 将数据库转换为有符号表示
 * 用于同态加密计算前的预处理
 */
void database_to_signed(std::vector<std::vector<uint64_t>> &data,
                        uint64_t pbits, uint64_t modulus);

/**
 * 将数据库转换为RNS有符号表示
 * 在两个不同模数下表示数据
 */
void database_to_rnssigned(std::vector<std::vector<uint64_t>> &data,
                           uint64_t pbits, uint64_t modulus1,
                           uint64_t modulus2);

/**
 * 将数据库转换为NTT形式
 * 便于执行高效的多项式乘法
 */
void database_tontt(std::vector<std::vector<uint64_t>> &data);

/**
 * 将数据库转换为RNS-NTT形式
 * 同时应用RNS和NTT变换
 */
void database_to_rnsntt(std::vector<std::vector<uint64_t>> &data);

/**
 * 将数据转换为准备好的数据
 * 适用于特定存储格式的预处理
 */
void data_to_setupdata(std::vector<std::vector<uint64_t>> &setup_data,
                       std::vector<std::vector<uint64_t>> &data, uint64_t pbits,
                       uint64_t modulus1, uint64_t modulus2);

/**
 * 对向量取负
 * 将每个元素替换为其在模意义下的负数
 */
void negate(uint64_t *result, uint64_t length, uint64_t modulus);

/**
 * 向量乘以常数
 * 将向量的每个元素乘以同一个常数
 */
void multConst(std::vector<uint64_t> &result, uint64_t cosntNum,
               uint64_t modulus = bigMod);

/**
 * LWE到RLWE转换
 * 将LWE形式的向量转换为RLWE形式
 */
void lweToRlwe(std::vector<uint64_t> &result, uint64_t modulus = bigMod);

/**
 * 矩阵转置
 * 将二维向量进行转置操作
 */
void transpose(std::vector<std::vector<uint64_t>> &a);

/**
 * 计算Bg的幂
 * 用于分解步骤中的基数计算
 */
std::vector<uint64_t> powerOfBg(uint64_t base, uint64_t BBg, int32_t ellnum);

/**
 * 计算Bg的128位幂
 * 用于处理大整数的分解
 */
std::vector<uint128_t> powerOfBg128(uint64_t base, uint64_t BBg,
                                    int32_t ellnum);

// inline uint64_t powerOfBg(uint64_t base, uint64_t BBg, int32_t num);

/**
 * CRT逆变换
 * 结合两个不同模数下的表示恢复原始值
 */
void crt_inv(std::vector<uint128_t> &result,
             const std::vector<uint64_t> &input1,
             const std::vector<uint64_t> &input2, uint64_t modulus1,
             uint64_t modulus2);

/**
 * 元素分解（C风格数组版本）
 * 将输入向量分解为基数表示
 */
void decompose(uint64_t **result, const uint64_t *input, int32_t ellnum,
               uint64_t base, uint64_t BBg);

/**
 * 元素分解（STL向量版本）
 * 将输入向量分解为基数表示
 */
void decompose(std::vector<std::vector<uint64_t>> &result,
               const std::vector<uint64_t> &input, int32_t ellnum,
               uint64_t base, uint64_t BBg, uint64_t modulus = bigMod);

/**
 * 从分解表示重构原始值
 * 用于验证分解操作的正确性
 */
std::vector<uint64_t> recontruct(std::vector<std::vector<uint64_t>> &dec_a,
                                 int32_t ellnum, uint64_t base, uint64_t BBg,
                                 uint64_t modulus = bigMod);

/**
 * 检查分解与重构操作是否一致
 * 用于调试和验证
 */
void check_recontruct(std::vector<std::vector<uint64_t>> &dec_a,
                      const std::vector<uint64_t> &a, int32_t ellnum,
                      uint64_t base, uint64_t BBg, uint64_t modulus = bigMod);

/**
 * 检查BSGS分解与重构操作是否一致
 * 针对BSGS算法的特定验证
 */
void check_recontruct_bsgs(std::vector<std::vector<uint64_t>> &dec_a1,
                           std::vector<std::vector<uint64_t>> &dec_a2,
                           const std::vector<uint64_t> &a1,
                           const std::vector<uint64_t> &a2, int32_t ellnum,
                           uint64_t base, uint64_t BBg, uint128_t modulus);

/**
 * 变体分解算法
 * 针对特定应用场景的优化分解
 */
void decompose_variant(std::vector<std::vector<uint64_t>> &result,
                       const std::vector<uint64_t> &input, int32_t ellnum,
                       uint64_t base, uint64_t BBg);

/**
 * RLWE密文分解
 * 处理完整的RLWE密文进行分解
 */
void decompose_rlwe(std::vector<std::vector<uint64_t>> &result,
                    const std::vector<uint64_t> &input1,
                    const std::vector<uint64_t> &input2, int32_t ellnum,
                    uint64_t base, uint64_t BBg, uint64_t modulus = bigMod);

/**
 * CRT形式的分解
 * 对两个不同模数下的密文进行分解
 */
void decompose_crt(std::vector<std::vector<uint64_t>> &result1,
                   std::vector<std::vector<uint64_t>> &result2,
                   const std::vector<uint64_t> &input1,
                   const std::vector<uint64_t> &input2, int32_t ellnum,
                   uint64_t base, uint64_t BBg);

/**
 * BSGS算法的分解
 * 针对Baby-Step Giant-Step算法优化的分解
 */
void decompose_bsgs(std::vector<std::vector<uint64_t>> &result1,
                    std::vector<std::vector<uint64_t>> &result2,
                    const std::vector<uint64_t> &input1,
                    const std::vector<uint64_t> &input2, int32_t ellnum,
                    uint64_t base, uint64_t BBg);

/**
 * BSGS-BA算法的分解
 * 针对Baby-Step算法的特殊优化
 */
void decompose_bsgs_ba(std::vector<std::vector<uint64_t>> &result1,
                       std::vector<std::vector<uint64_t>> &result2,
                       const std::vector<uint64_t> &input1,
                       const std::vector<uint64_t> &input2, int32_t ellnum,
                       uint64_t base, uint64_t BBg);

/**
 * BSGS-AUX算法的分解
 * 使用辅助模数的BSGS分解
 */
void decompose_bsgs_aux(std::vector<std::vector<uint64_t>> &result1,
                        std::vector<std::vector<uint64_t>> &result2,
                        const std::vector<uint64_t> &input1,
                        const std::vector<uint64_t> &input2, int32_t ellnum,
                        uint64_t base, uint64_t BBg);

/**
 * 用单一元素填充向量
 * 将向量的所有元素设置为相同的值
 */
inline void element_to_vector(std::vector<uint64_t> &result, uint64_t input) {
  for (auto iter = result.begin(); iter != result.end(); iter++)
    *iter = input;
}

/**
 * 自同态变换
 * 应用多项式的自同态映射
 */
void automorphic(std::vector<uint64_t> &result,
                 const std::vector<uint64_t> &input, const int32_t index,
                 const uint64_t modulus);

/**
 * CRT形式的编码
 * 对结果应用特定的CRT编码
 */
void encode_crt(std::vector<uint64_t> &result);

/**
 * 计算索引指示器
 * 确定数据索引和方向
 */
void compute_indicator(int32_t &data_index, bool &reverse, size_t j,
                       int32_t s_index);

/**
 * 模幂运算（32位版本）
 * 计算a^b mod mod_number
 */
int32_t pow_mod(int32_t a, int32_t b, int32_t mod_number);

/**
 * 模幂运算（64位版本）
 * 计算a^b mod mod_number
 */
uint64_t pow_mod(uint64_t a, int32_t b, uint64_t mod_number);

/**
 * hexl_ntt_index 是 (bit_reversal_index << 1 | 1)。
 * rotate_index是i^5 mod 2N。
 */
void compute_hexl_rotate_indexes(std::vector<int32_t> &hexl_ntt_index,
                                 std::vector<int32_t> &rotate_index,
                                 const int32_t length = N);

/**
 * 计算置换向量
 * 基于特定索引计算置换映射
 */
void compute_permutation(std::vector<int32_t> &permutation, const int32_t index,
                         const int32_t length);

/**
 * @brief 计算置换矩阵
 * 该函数计算用于BSGS算法的置换矩阵。置换矩阵用于重排列向量元素,以实现高效的同态乘法。
 * @param permutations 输出参数,存储计算得到的置换矩阵
 * @param max_indexs Baby-step大小,决定置换矩阵的行数
 * @param length 向量长度,决定置换矩阵的列数
 * @note 置换矩阵的每一行对应一个不同的循环移位量,用于BSGS算法中的Baby-step计算
 */
void compute_permutation_matrix(std::vector<std::vector<int32_t>> &permutations,
                                const int32_t max_indexs, const int32_t length);

/**
 * 计算查询编码
 * 用于PIR查询的特殊编码
 * @param query_encode 输出参数,存储计算得到的查询编码
 * @param length 向量长度,决定置换矩阵的列数
 * @note 得到shift_index[i] = i^5 mod 2N
 * @note 然后计算query_encode[i]是shift_index[i]的bit_reversal_index
 */
void compute_query_encode(std::vector<int32_t> &query_encode,
                          const int32_t length = N);

/**
 * 计算查询解码
 * 用于反转查询编码操作
 * compute_query_encode的逆操作
 */
void compute_query_decode(std::vector<int32_t> &query_decode,
                          const int32_t length = N);

} // namespace kspir
