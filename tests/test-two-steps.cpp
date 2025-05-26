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

// 32 MB
void test_two_steps() {
  Secret queryKey(bigMod);

  uint64_t row = rand() & (N - 1);
  uint64_t col = 123;

  // sample database
  std::vector<std::vector<uint64_t>> data(N, std::vector<uint64_t>(N / 2, 0));
  sample_database_bsgs(data);

  // we let ntt(data) to be small
  /**
  std::vector<uint64_t> temp(N, 0);
  intel::hexl::NTT ntts(N, bigMod);
  for (size_t i = 0; i < N / 2; i++)
  {
      for (size_t j = 0; j < N; j++)
      {
          temp[j] = rand() & 0x01;
      }
      ntts.ComputeForward(temp.data(), temp.data(), 1, 1);
      for (size_t j = 0; j < N / 2; j++)
      {
          // diagonal line
          int row = (j - i + N/2) & (N/2 - 1); // the row is always smaller
          result[i][j] = data[row][j];
          result[i][N - 1 - j] = data[N - 1 - row][j];
      }
  }
  **/

  for (size_t i = 0; i < N; i++)
    data[i][col] = i + 1;
  std::cout << "the wanted message is " << data[row][col] << std::endl;

  std::vector<std::vector<uint64_t>> data_ntt(N / 2,
                                              std::vector<uint64_t>(N, 0));
  database_tobsgsntt(data_ntt, data, bigMod);

  // auto start_qu = std::chrono::high_resolution_clock::now();
  RlweCiphertext query1(N, bigMod);
  query_bsgs(query1, queryKey, col);

  uint64_t length = queryKey.getLength();
  // uint64_t moudlus = answerKey.getModulus();

  AutoKeyBSGS autoKey;
  std::vector<int32_t> indexLists;
  for (size_t i = 1; i < N / 2; i++)
  // for (size_t i = 1; i < 50; i++)
  {
    indexLists.push_back(pow_mod(5, i, 2 * N));
  }
  autoKey.keyGen(queryKey, indexLists);

  // the results
  RlweCiphertext result;
  std::vector<uint64_t> decryptd_message(length);

  int ntimes = 1;

  auto start = std::chrono::high_resolution_clock::now();

  for (size_t i = 0; i < ntimes; i++) {
    matrix_vector_mul(result, query1, data_ntt, autoKey);
  }
  auto stop = std::chrono::high_resolution_clock::now();

  auto glapsed =
      std::chrono::duration_cast<std::chrono::microseconds>(stop - start);
  std::cout << ntimes << " matrix vector multiplication costs "
            << glapsed.count() << " us." << std::endl;

  // decrypt message
  decrypt_bsgs(decryptd_message, result, queryKey);

  std::cout << std::endl;

  std::cout << "the recovered value is " << decryptd_message[row] << std::endl;
  showLargeVector(decryptd_message, "result = ");
}

void test_two_steps_bsgs_rns_large() {
  // r = 16     : 256 MB
  // r = 64     :   1 GB
  // r = 64 * 8 :   8 GB

  SPDLOG_INFO("=================== BSGS-RNS-LARGE 测试 ===================");

  // 创建定时器
  Timer timer;
  timer.setTimePoint("开始");

  int32_t r = 16; // 256 MB

  // 初始化密钥
  Secret queryKey(crtMod, false); // stored in coefficient form
  timer.setTimePoint("密钥初始化");

  uint64_t row = rand() & (N - 1);
  uint64_t col = 123;

  // 设置参数
  int32_t N1 = 128; // 128; // 64;
  int32_t N2 = N / 2 / N1;
  SPDLOG_INFO("参数设置: r={}, N1={}, N2={}", r, N1, N2);

  // 内存分配
  size_t num_words = N * N / 2;
  uint64_t *datacrt =
      (uint64_t *)aligned_alloc(64, sizeof(uint64_t) * num_words * r);

  std::vector<std::vector<uint64_t>> data(N, std::vector<uint64_t>(N / 2, 0));
  std::vector<std::vector<uint64_t>> data_ntt(N / 2,
                                              std::vector<uint64_t>(N, 0));
  timer.setTimePoint("内存分配");

  // 数据库预处理
  for (size_t k = 0; k < r; k++) {
    // 随机生成数据库
    sample_database_bsgs(data);
    for (size_t i = 0; i < N; i++)
      data[i][col] = i + 1;

    if (k == 0) {
      timer.setTimePoint("数据库采样");
    }

    // 转换为BSGS-NTT形式
    database_tobsgsntt(data_ntt, data, crtMod, N1);

    if (k == 0) {
      timer.setTimePoint("NTT转换");
    }

    // 转换为CRT形式
    database_tocrt(datacrt + num_words * k, data_ntt, N1);

    if (k == 0) {
      timer.setTimePoint("CRT转换");
    }

    if (k == 0 || k == r - 1) {
      SPDLOG_INFO("数据库 #{} 预处理完成", k);
    }
  }
  timer.setTimePoint("数据库预处理完成");

  // 生成查询密文
  std::vector<RlweCiphertext> query1(1, RlweCiphertext(N, crtMod));
  query1.push_back(RlweCiphertext(N, bsMod));
  query_bsgs_rns(query1, queryKey, col);
  timer.setTimePoint("查询生成");

  // 生成自同态密钥
  uint64_t length = queryKey.getLength();
  AutoKeyBSGSRNS autoKey(N, crtMod, bsMod);

  std::vector<int32_t> indexLists;
  for (size_t i = 1; i <= N1 / 2; i++) {
    indexLists.push_back(pow_mod(5, i, 2 * N));
  }
  autoKey.keyGen(queryKey, indexLists, BabyStep);

  indexLists.clear();
  for (size_t i = 1; i < N2; i++) {
    indexLists.push_back(pow_mod(5, N1 * i, 2 * N));
  }
  autoKey.keyGen(queryKey, indexLists, GaintStep);
  timer.setTimePoint("自同态密钥生成");

  // 计算置换矩阵
  std::vector<std::vector<int32_t>> permutations(
      N1, std::vector<int32_t>(length, 0));
  compute_permutation_matrix(permutations, N1, length);
  timer.setTimePoint("置换矩阵计算");

  // 矩阵-向量乘法
  std::vector<RlweCiphertext> result(r, RlweCiphertext(N, crtMod));
  std::vector<uint64_t> decryptd_message(length);
  int ntimes = 1;

  for (size_t i = 0; i < ntimes; i++) {
    matrix_vector_mul_bsgs_rns_crt_large(result, query1, datacrt, autoKey, N1,
                                         permutations, r);
  }
  timer.setTimePoint("矩阵-向量乘法");

  // 解密结果
  decrypt_bsgs(decryptd_message, result[0], queryKey);
  timer.setTimePoint("解密");

  // 验证结果
  SPDLOG_INFO("查询结果: 第 {} 行的值是 {}", row, decryptd_message[row]);
  timer.setTimePoint("结果验证");

  SPDLOG_INFO("解密结果是 {}", fmt::join(decryptd_message.begin(),
                                         decryptd_message.begin() + 20, ","));

  // 输出性能报告
  SPDLOG_INFO("");
  SPDLOG_INFO("=================== 性能统计 ===================");
  SPDLOG_INFO("\n{}", timer);

  // 对于特定阶段，计算其占总时间的百分比
  double totalTime = timer.getTotalTime();

  auto keyInit = timer.getTimePoint("密钥初始化");
  auto dbPrep = timer.getTimePoint("数据库预处理完成");
  auto queryGen = timer.getTimePoint("查询生成");
  auto keyGen = timer.getTimePoint("自同态密钥生成");
  auto matMul = timer.getTimePoint("矩阵-向量乘法");
  auto decrypt = timer.getTimePoint("解密");

  SPDLOG_INFO("关键阶段时间占比:");
  SPDLOG_INFO("  - 数据库预处理: {:.2f}%", (dbPrep.second / totalTime) * 100.0);
  SPDLOG_INFO("  - 查询生成:     {:.2f}%",
              (queryGen.second / totalTime) * 100.0);
  SPDLOG_INFO("  - 矩阵-向量乘:  {:.2f}%", (matMul.second / totalTime) * 100.0);
  SPDLOG_INFO("  - 解密:         {:.2f}%",
              (decrypt.second / totalTime) * 100.0);

  // 每秒可处理的数据量
  double throughput =
      (r * N * N / 2.0 * 8) / (totalTime / 1000.0) / (1024 * 1024); // MB/s
  SPDLOG_INFO("处理速度: {:.2f} MB/s", throughput);

  SPDLOG_INFO(
      "=================== BSGS-RNS-LARGE 测试完成 ===================");

  // 释放内存
  free(datacrt);
}

int main(int argc, char **argv) {
  // srand(time(NULL));

  // test_two_steps();
  test_two_steps_bsgs_rns_large(); // packing r basic database

  return 0;
}
