#pragma once

#include <stddef.h>
#include <stdint.h>
#include <vector>

#include "lwe.h"
#include "secret.h"

namespace kspir {

/**
 * 将LWE密文的a向量分解为小分量
 * 用于密钥切换操作
 */
void decompose_a(uint64_t **dec_a, uint64_t *a);

#ifndef INTEL_HEXL
/**
 * 构建针对每个查询的密钥切换提示
 * 使用非Intel HEXL实现
 */
void ksKey_hint(uint64_t *b_rlwe, uint64_t *a_rlwe, uint64_t *a, uint64_t **ks);
#else
/**
 * 构建针对每个查询的密钥切换提示
 * 使用Intel HEXL实现
 */
int ksKey_hint(std::vector<uint64_t> &b_rlwe, std::vector<uint64_t> &a_rlwe,
               std::vector<uint64_t> &a,
               std::vector<std::vector<uint64_t>> &ks);

/**
 * 使用第一种构造方法生成密钥切换提示
 * 基于公钥和数据
 */
int ksKey_hint_first_construction(
    std::vector<uint64_t> &b_rlwe, std::vector<uint64_t> &a_rlwe,
    std::vector<uint64_t> &a, std::vector<std::vector<uint64_t>> &pk,
    std::vector<std::vector<uint64_t>> &data, int32_t ellnum = 3,
    uint64_t base = 0x01 << 17, uint64_t BBg = 0x01 << 11);
#endif

#ifndef INTEL_HEXL
/**
 * 处理数据库查询的回答
 * 使用非Intel HEXL实现
 */
int answer(uint64_t *b_rlwe, uint64_t *a_rlwe, uint64_t *b, uint64_t *a,
           std::vector<std::vector<uint64_t>> &ks,
           std::vector<std::vector<uint64_t>> &data, bool isntt_b = 1,
           bool isntt_data = 1);
#else
/**
 * 处理第一维度的数据库查询回答
 * 使用Intel HEXL实现
 */
int answer_first_dimension(
    std::vector<uint64_t> &b_rlwe, std::vector<uint64_t> &a_rlwe,
    std::vector<uint64_t> &query_b, // std::vector<uint64_t>& a,
    std::vector<std::vector<uint64_t>>
        &data, // std::vector<std::vector<uint64_t> >& ks,
    bool isntt_b = 1, bool isntt_data = 1);

/**
 * 处理二维数据库查询回答
 * @param result 结果密文
 * @param b_rlwe b部分，系数形式
 * @param a_rlwe a部分，系数形式
 * @param b 查询b，NTT形式
 * @param data 数据，NTT形式
 * @param query 查询密文
 * @param isntt_b b是否为NTT形式
 * @param isntt_data 数据是否为NTT形式
 * @return 执行状态码
 */
int answer_two_dimensions(RlweCiphertext &result, std::vector<uint64_t> &b_rlwe,
                          std::vector<uint64_t> &a_rlwe,
                          std::vector<uint64_t> &b,
                          std::vector<std::vector<uint64_t>> &data,
                          RGSWCiphertext &query, bool isntt_b = 1,
                          bool isntt_data = 1);

/**
 * 生成公钥
 * 用于同态加密系统中的密钥切换
 */
void pkKeyGen(std::vector<std::vector<uint64_t>> &pk, Secret &queryKey,
              Secret &answerKey, int32_t ellnum = 3, uint64_t base = 0x01 << 17,
              uint64_t BBg = 0x01 << 11);

/**
 * 生成虚拟密钥切换密钥
 * 用于测试和开发
 */
void dummy_ksKeyGen(std::vector<std::vector<uint64_t>> &ks, Secret &queryKey,
                    Secret &answerKey,
                    std::vector<std::vector<uint64_t>> &data);
#endif

/**
 * 评估自同态变换
 * @param result 结果密文
 * @param index 自同态指数
 * @param autokey 自同态密钥
 */
void evalAuto(RlweCiphertext &result, const int32_t index,
              const AutoKey &autokey);

/**
 * 打包多个LWE密文到单个RLWE密文
 * 用于批处理和优化
 */
void packingLWEs(RlweCiphertext &result, std::vector<RlweCiphertext> &lwes,
                 const AutoKey &autokey);

/**
 * 打包多个RLWE密文到单个RLWE密文
 * 用于批处理和优化
 */
void packingRLWEs(RlweCiphertext &result, std::vector<RlweCiphertext> &rlwes,
                  const AutoKey &autokey);

/**
 * 评估RNS模式下的自同态变换
 * 用于大整数计算的优化
 */
void evalAutoRNS(RlweCiphertext &result1, RlweCiphertext &result2,
                 const int32_t index, const AutoKeyRNS &autokey);

/**
 * 在RNS表示下扩展密文
 * 用于向量打包优化
 */
void evalExpandRNS(std::vector<RlweCiphertext> &result,
                   std::vector<RlweCiphertext> &input,
                   const AutoKeyRNS &autokey);

/**
 * 生成用于同态加密的公钥
 * 基于查询密钥和答案密钥
 */
void publicKeyGen(std::vector<std::vector<uint64_t>> &publicKey,
                  Secret &queryKey, Secret &answerKey);

/**
 * 生成密钥切换密钥
 * 使用公钥和数据
 */
void evalKsKeyGen(std::vector<std::vector<uint64_t>> &ks,
                  std::vector<std::vector<uint64_t>> &pk,
                  std::vector<std::vector<uint64_t>> &data);

/**
 * 模数切换操作
 * 用于在不同模数间转换密文
 * @param result 结果数组，维度1: 2 * ell * N, 维度2: N，以NTT形式存储
 * @param ks 密钥切换数组，维度1: 2 * ell * N * 2(CRT), 维度2: N，以NTT形式存储
 */
void modSwitch(std::vector<std::vector<uint64_t>> &result,
               std::vector<std::vector<uint64_t>> &ks);

/**
 * RNS模数切换
 * 在不同模数间转换RLWE密文
 * @param result 结果密文，以NTT形式存储
 * @param input1 模数1下的RLWE密文
 * @param input2 模数2下的RLWE密文
 */
void modSwitch(RlweCiphertext &result, RlweCiphertext &input1,
               RlweCiphertext &input2);

} // namespace kspir
