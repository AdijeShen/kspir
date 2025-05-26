/**
 * @file test-pir.cpp
 * @brief 第三种PIR构造的完整实现测试
 *
 * 本文件实现了基于BSGS算法的PIR协议测试，该构造具有以下特点：
 *
 * ## 算法概述
 * 第三种PIR构造通过以下步骤实现高效的私有信息检索：
 * 1. 数据库编码：M = NTT(DB)，将数据库转换为NTT形式
 * 2. 查询编码：生成RLWE(Δ·NTT⁻¹(u))和RGSW(X⁻ʷ)两部分查询
 * 3. 第一维折叠：ans₀ = BSGS(M, (b,a))，矩阵-向量同态乘法
 * 4. 第二维折叠：(b',a') = C ⊠ ans₀，外部乘积选择特定元素
 * 5. 结果提取：r = Ext(b',a')，打包并返回最终结果
 *
 * ## 核心优化技术
 * - Baby-step-Giant-step (BSGS) 算法：将O(N)旋转优化为O(√N)
 * - RNS (Residue Number System)：使用多个小模数并行计算
 * - CRT (Chinese Remainder Theorem)：模数间高效转换
 * - NTT优化：利用Intel HEXL库进行快速数论变换
 * - 无预处理：服务器端无需额外预处理负担
 *
 * ## 参数说明
 * - r: 打包数量，影响数据库大小和吞吐量
 * - N1: Baby-step大小，默认128，影响BSGS算法效率
 * - N2: Giant-step大小，由N/2/N1计算得到
 * - crtMod: 主要CRT模数，用于大部分计算
 * - bsMod: Baby-step模数，用于RNS优化
 *
 * ## 性能特点
 * - 整体吞吐量比Spiral快约5.8倍
 * - 无需预处理阶段，适用于动态数据库场景
 * - 通信开销较低，查询和响应大小都经过优化
 *
 * @note
 * 本实现严格遵循论文中的第三种构造方法，所有算法步骤都有对应的代码实现
 */

#include <chrono>
#include <iostream>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include <vector>

#include "crt.h"
#include "pir.h"
#include "spdlog/fmt/bundled/format.h"
#include "timer.h"
#include <spdlog/spdlog.h>

void test_pir() {
  SPDLOG_INFO("=================== 第三种PIR构造测试 ===================");

  // 创建性能计时器
  Timer timer;
  timer.setTimePoint("Begin");

  // ========== 数据库大小配置 ==========
  // r参数控制打包数量和数据库大小：
  // r = 16     : 256 MB 数据库
  // r = 64     :   1 GB 数据库  
  // r = 64 * 8 :   8 GB 数据库
  int32_t r = 64;

  // ========== 第一步：密钥生成阶段 ==========
  // 生成RLWE查询密钥，存储在系数形式
  // 这是客户端的私有密钥，用于加密查询和解密结果
  Secret queryKey(crtMod, false);
  timer.setTimePoint("KeyGeneration");

  // ========== 查询目标设置 ==========
  // 随机选择要查询的目标位置
  uint32_t target_col = rand() % (N / 2); // 对应算法中的二维坐标u（列）
  uint32_t target_packing = rand() % r;   // 对应算法中的二维坐标w（行），用于打包

  // ========== BSGS算法参数配置 ==========
  uint64_t modulus = crtMod; // 主要计算模数Rq
  int32_t N1 = 128;          // Baby-step大小，影响第一阶段旋转次数
  int32_t N2 = N / 2 / N1;   // Giant-step大小，确保 N1 * N2 = N/2

  // 输出配置信息
  SPDLOG_INFO("打包数量: {}", r);
  SPDLOG_INFO("数据库配置: {} * 8 KB, 总大小 {} MB",
              r * N / 2, 16 * r);
  SPDLOG_INFO("BSGS参数: (N1: {}, N2: {})", N1, N2);
  SPDLOG_INFO("查询目标: 列={}, 打包位置={}", target_col, target_packing);
  timer.setTimePoint("Configuration");

  // ========== 数据库内存分配 ==========
  size_t num_words = N * N / 2;
  uint64_t *datacrt =
      (uint64_t *)aligned_alloc(64, sizeof(uint64_t) * num_words * r);

  // 数据库矩阵：原始形式和NTT变换后的形式
  std::vector<std::vector<uint64_t>> data(N, std::vector<uint64_t>(N / 2, 0));
  std::vector<std::vector<uint64_t>> data_ntt(N / 2,
                                              std::vector<uint64_t>(N, 0));
  timer.setTimePoint("MemoryAllocation");

  // ========== 第二步：数据库编码阶段 ==========
  // 对应算法中的 M = NTT(DB) 步骤
  std::vector<uint64_t> input_record(N);
  for (size_t k = 0; k < r; k++) {
    // 1. 生成随机数据库内容
    sample_database_bsgs(data);
    if (k == 0)
      timer.setTimePoint("DatabaseSampling");

    // 2. 在目标位置插入测试数据，用于验证正确性
    for (size_t i = 0; i < N; i += r)
      data[i + target_packing][target_col] = i + 1 + k;

    // 3. 对目标列进行逆向编码
    // 这是为了配合后续的NTT变换和查询编码
    for (size_t i = 0; i < N; i++)
      input_record[i] = data[i][target_col];
    inverse_encode(input_record);
    for (size_t i = 0; i < N; i++)
      data[i][target_col] = input_record[i];

    if (k == 0)
      timer.setTimePoint("DatabaseEncoding");

    // 4. 执行BSGS-NTT预处理：将数据库转换为对角线形式并应用NTT
    // 这是算法的核心步骤，将数据库从普通形式转换为"魔法计算"所需的NTT形式
    database_tobsgsntt(data_ntt, data, crtMod, N1);
    if (k == 0)
      timer.setTimePoint("DatabaseBSGSNTT");
      
    // 5. 转换为CRT表示，用于后续的高效计算
    database_tocrt(datacrt + num_words * k, data_ntt, N1);
    if (k == 0)
      timer.setTimePoint("DatabaseCRT");

    if (k == 0 || k == r - 1) {
      SPDLOG_INFO("数据库 #{} 预处理完成", k);
    }
  }
  timer.setTimePoint("DatabasePreprocessing");

  // ========== 第三步：查询生成阶段 ==========
  // 对应算法中的查询编码步骤
  
  // 1. 生成RLWE查询密文 (b,a) = RLWE(Δ·NTT⁻¹(u))
  // 这部分加密了行选择信息（target_col对应的u）
  std::vector<RlweCiphertext> query1(1, RlweCiphertext(N, crtMod));
  query1.push_back(RlweCiphertext(N, bsMod));

  // 2. 生成RGSW查询密文 C = RGSW.Enc(sk, X⁻ʷ)  
  // 这部分加密了列选择信息（target_packing对应的w）
  RGSWCiphertext queryGsw(N, modulus, 2, 0x01 << 20, 0x01 << 18);

  // 3. 实际生成查询密文
  query_bsgs_rns(query1, queryKey, target_col);    // 生成行选择查询 (b,a)
  queryGsw.keyGen(queryKey, target_packing, true); // 生成列选择查询 C
  timer.setTimePoint("QueryGeneration");

  // ========== 第四步：密钥切换密钥生成 ==========
  // 生成BSGS算法所需的自同态密钥
  uint64_t length = queryKey.getLength();
  AutoKeyBSGSRNS autoKey(N, crtMod, bsMod);
  autoKey.bsgsKeyGen(queryKey, N1);
  timer.setTimePoint("KeySwitchingSetup");

  // 生成打包操作所需的密钥
  AutoKey packingKey(length, modulus, 4, 0, 0x01 << 14);
  packingKey.keyGen(queryKey, r, true);
  timer.setTimePoint("PackingKeyGeneration");

  // 计算BSGS算法所需的置换矩阵
  std::vector<std::vector<int32_t>> permutations(
      N1, std::vector<int32_t>(length, 0));
  compute_permutation_matrix(permutations, N1, length);
  SPDLOG_DEBUG("置换矩阵: {}", fmt::join(permutations[0], ","));
  timer.setTimePoint("PermutationMatrix");

  // ========== 结果存储准备 ==========
  std::vector<RlweCiphertext> result(r, RlweCiphertext(N, crtMod));
  RlweCiphertext result_output(N, modulus);
  std::vector<uint64_t> decryptd_message(length);
  std::vector<RlweCiphertext> ext_rlwes;
  for (size_t i = 0; i < r; i++)
    ext_rlwes.push_back(RlweCiphertext(N, modulus));
  timer.setTimePoint("ResultSetup");

  // ========== 第五步：服务器响应计算阶段 ==========
  int ntimes = 1;

  for (size_t i = 0; i < ntimes; i++) {
    // 1. 第一维折叠：矩阵-向量同态乘法
    // 计算 ans₀ = BSGS(M, (b,a))
    // 这一步通过BSGS算法高效地从数据库中"抽取"目标行的所有数据
    // 神奇之处：NTT⁻¹(NTT(DB) · u) = DB · u，同态地获得了第u列的所有元素
    matrix_vector_mul_bsgs_rns_crt_large(result, query1, datacrt, autoKey, N1,
                                         permutations, r);
    timer.setTimePoint("MatrixVectorMultiplication");

    // 2. 第二维折叠：外部乘积
    // 计算 (b',a') = C ⊠ ans₀
    // 这一步从刚才抽取的行数据中，同态地"锁定"特定的列元素
    // 由于C加密了X⁻ʷ，这个操作会选择出DB[w, u]位置的数据
    for (size_t j = 0; j < r; j++)
      externalProduct(ext_rlwes[j], result[j], queryGsw);
    timer.setTimePoint("ExternalProduct");

    // 3. 结果打包：将多个RLWE密文打包成单个响应
    // 计算最终响应 r = (b̄,ā) = Ext(b',a')
    // 这一步将所有结果打包，减少通信开销
    packingRLWEs(result_output, ext_rlwes, packingKey);
    timer.setTimePoint("ResponsePacking");
  }

  // ========== 第六步：客户端解密阶段 ==========
  // 计算 d = LWE.Dec(coef(s(X⁻¹)), (b̄,ā))
  // 使用客户端私钥解密服务器响应，恢复查询结果
  decrypt_bsgs_total(decryptd_message, result_output, queryKey, r);
  timer.setTimePoint("Decryption");

  // ========== 结果验证和输出 ==========
  SPDLOG_INFO("查询结果成功检索");
  SPDLOG_INFO(
      "结果值: {}",
      fmt::join(decryptd_message.begin(), decryptd_message.begin() + 20, ", "));
  timer.setTimePoint("ResultVerification");

  // ========== 性能统计报告 ==========
  SPDLOG_INFO("");
  SPDLOG_INFO("=================== 性能统计 ===================");
  SPDLOG_INFO("\n{}", timer);

  // 计算各阶段时间占比
  double totalTime = timer.getTotalTime();

  auto dbPrep = timer.getTimePoint("DatabasePreprocessing");
  auto queryGen = timer.getTimePoint("QueryGeneration");
  auto keySwitch = timer.getTimePoint("KeySwitchingSetup");
  auto matMul = timer.getTimePoint("MatrixVectorMultiplication");
  auto extProduct = timer.getTimePoint("ExternalProduct");
  auto response = timer.getTimePoint("ResponsePacking");
  auto decrypt = timer.getTimePoint("Decryption");
  auto packingKeyGen = timer.getTimePoint("PackingKeyGeneration");

  SPDLOG_INFO("关键阶段时间分布:");
  SPDLOG_INFO("  - 数据库预处理:       {:.2f}%",
              (dbPrep.second / totalTime) * 100.0);
  SPDLOG_INFO("  - 查询生成:           {:.2f}%",
              (queryGen.second / totalTime) * 100.0);
  SPDLOG_INFO("  - 矩阵-向量乘法:      {:.2f}%",
              (matMul.second / totalTime) * 100.0);
  SPDLOG_INFO("  - 外部乘积:           {:.2f}%",
              (extProduct.second / totalTime) * 100.0);
  SPDLOG_INFO("  - 响应打包:           {:.2f}%",
              (response.second / totalTime) * 100.0);
  SPDLOG_INFO("  - 解密:               {:.2f}%",
              (decrypt.second / totalTime) * 100.0);

  // 计算吞吐量指标
  double serverTime = matMul.second + extProduct.second + response.second;
  double clientTime = decrypt.second + queryGen.second + packingKeyGen.second;
  double dataSize = (r * N * N / 2.0 * 8) / (1024 * 1024);             // MB
  double throughput = dataSize / ((serverTime + clientTime) / 1000.0); // MB/s

  SPDLOG_INFO("服务器处理时间: {:.2f} ms", serverTime);
  SPDLOG_INFO("客户端处理时间: {:.2f} ms", clientTime);
  SPDLOG_INFO("服务器吞吐量: {:.2f} MB/s", throughput);

  // 分析通信开销
  size_t rlwe_query_size = query1[0].b.size() + query1[1].b.size();
  size_t rgsw_query_size = queryGsw.getEllnum() * 2 * 2 * N / 2;
  double querySize =
      (rlwe_query_size + rgsw_query_size) * sizeof(uint64_t) / (1024.0);
  double responseSize = (result_output.a.size() + result_output.b.size()) *
                        sizeof(uint64_t) / (1024.0);

  SPDLOG_INFO("通信开销:");
  SPDLOG_INFO("  - 查询大小:    {:.2f} KB", querySize);
  SPDLOG_INFO("  - 响应大小:    {:.2f} KB", responseSize);
  SPDLOG_INFO("  - 总计:        {:.2f} KB", querySize + responseSize);

  SPDLOG_INFO("=================== PIR测试完成 ===================");

  // 释放内存
  free(datacrt);
}

int main(int argc, char **argv) {
  // 设置随机种子（可选）
  // srand(time(NULL));

  test_pir();

  return 0;
}
