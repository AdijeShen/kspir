/**
 * @file test-pir-string.cpp
 * @brief PIR字符串查询演示程序
 *
 * 本文件实现了一个基于现有PIR算法的字符串查询系统，演示了完整的两方交互过程：
 *
 * ## 系统概述
 * - Server: 存储字符串数组，支持私有查询
 * - Client: 通过索引私密地查询特定字符串
 * - 基于RLWE/RGSW加密，确保查询隐私性
 *
 * ## 核心特性
 * - 字符串自动编码为PIR兼容的uint64_t格式
 * - 支持任意长度字符串（最大64字节）
 * - 完整的加密查询和解密响应流程
 * - 详细的性能统计和结果验证
 *
 * ## 技术实现
 * - 字符串编码：UTF-8字节流打包为uint64_t数组
 * - 数据约束：确保所有数值在模数范围内
 * - 内存管理：高效的数据库布局和查询处理
 *
 * @note 本实现在单进程内模拟两方交互，展示真实PIR协议的完整流程
 */

#include <algorithm>
#include <chrono>
#include <cstring>
#include <iostream>
#include <memory>
#include <stdint.h>
#include <stdlib.h>
#include <string>
#include <time.h>
#include <vector>

#include "crt.h"
#include "pir.h"
#include "spdlog/fmt/bundled/format.h"
#include "timer.h"
#include <spdlog/spdlog.h>

using namespace kspir;

// =================== 字符串编码/解码工具 ===================
/**
 * @brief 简化的字符串编码，每个字符使用UTF-16编码
 */
std::vector<uint64_t> encode_string_simple(const std::string &str) {
  std::vector<uint64_t> result(N, 0); // 初始化为N个0
  size_t pos = 0;

  // 将UTF-8字符串转换为UTF-16编码
  for (size_t i = 0; i < str.length() && pos < N; i++) {
    unsigned char c = str[i];
    uint32_t code_point;

    if (c < 0x80) {
      // ASCII字符直接转换
      code_point = c;
    } else if (c < 0xE0) {
      // 2字节UTF-8
      code_point = ((c & 0x1F) << 6) | (str[i + 1] & 0x3F);
      i++;
    } else if (c < 0xF0) {
      // 3字节UTF-8
      code_point =
          ((c & 0x0F) << 12) | ((str[i + 1] & 0x3F) << 6) | (str[i + 2] & 0x3F);
      i += 2;
    } else {
      // 4字节UTF-8
      code_point = ((c & 0x07) << 18) | ((str[i + 1] & 0x3F) << 12) |
                   ((str[i + 2] & 0x3F) << 6) | (str[i + 3] & 0x3F);
      i += 3;
    }

    // 将Unicode码点转换为UTF-16
    if (code_point < 0x10000) {
      result[pos++] = code_point;
    } else {
      // 处理代理对
      uint32_t surrogate = code_point - 0x10000;
      uint16_t high = (surrogate >> 10) + 0xD800;
      uint16_t low = (surrogate & 0x3FF) + 0xDC00;
      result[pos++] = high;
      result[pos++] = low;
    }
  }

  return result;
}

/**
 * @brief 简化的字符串解码，从UTF-16值恢复字符串
 */
std::string decode_string_simple(const std::vector<uint64_t> &values) {
  std::string result;
  std::string utf8;

  for (size_t i = 0; i < values.size(); i++) {
    if (values[i] == 0)
      break;

    uint32_t code_point;
    if (values[i] >= 0xD800 && values[i] <= 0xDBFF && i + 1 < values.size()) {
      // 处理代理对
      uint16_t high = values[i];
      uint16_t low = values[i + 1];
      if (low >= 0xDC00 && low <= 0xDFFF) {
        code_point = ((high - 0xD800) << 10) + (low - 0xDC00) + 0x10000;
        i++; // 跳过下一个值
      } else {
        code_point = values[i];
      }
    } else {
      code_point = values[i];
    }

    // 将Unicode码点转换为UTF-8
    if (code_point < 0x80) {
      utf8 += static_cast<char>(code_point);
    } else if (code_point < 0x800) {
      utf8 += static_cast<char>(0xC0 | (code_point >> 6));
      utf8 += static_cast<char>(0x80 | (code_point & 0x3F));
    } else if (code_point < 0x10000) {
      utf8 += static_cast<char>(0xE0 | (code_point >> 12));
      utf8 += static_cast<char>(0x80 | ((code_point >> 6) & 0x3F));
      utf8 += static_cast<char>(0x80 | (code_point & 0x3F));
    } else {
      utf8 += static_cast<char>(0xF0 | (code_point >> 18));
      utf8 += static_cast<char>(0x80 | ((code_point >> 12) & 0x3F));
      utf8 += static_cast<char>(0x80 | ((code_point >> 6) & 0x3F));
      utf8 += static_cast<char>(0x80 | (code_point & 0x3F));
    }
  }

  return utf8;
}

// =================== PIR字符串数据库 ===================

/**
 * @brief PIR字符串数据库类
 *
 * 管理字符串数据的存储和PIR格式转换，提供高效的查询支持
 */
class PIRStringDatabase {
private:
  std::vector<std::string> strings;
  std::vector<std::vector<uint64_t>> data;
  size_t max_string_length;
  int32_t r; // 打包数量

public:
  PIRStringDatabase(int32_t pack_size = 1)
      : r(pack_size), max_string_length(0) {
    data.resize(N, std::vector<uint64_t>(N / 2, 0));
  }

  void add_string(const std::string &str) {
    if (strings.size() >= N / 2) {
      throw std::runtime_error("数据库已满，最多支持 " + std::to_string(N / 2) +
                               " 个字符串");
    }

    strings.push_back(str);
    max_string_length = std::max(max_string_length, str.length());

    // 重新编码所有字符串
    encode_all_strings();
  }

  void encode_all_strings() {
    // 重置数据库
    for (auto &row : data) {
      std::fill(row.begin(), row.end(), 0);
    }

    // 将每个字符串编码到对应的列
    for (size_t col_idx = 0; col_idx < strings.size(); col_idx++) {
      const std::string &str = strings[col_idx];

      // 将字符串编码为uint64数组
      std::vector<uint64_t> encoded_string = encode_string_simple(str);

      // 确保编码后的数据不超过N行
      // if (encoded_string.size() > N) {
      //   throw std::runtime_error("字符串太长，编码后超过了数据库行数限制");
      // }

      // 将编码后的字符串数据放入第col_idx列
      for (size_t i = 0; i < N; i++) { // encoded_string.size() is always N
        data[i][col_idx] = encoded_string[i];
      }

      SPDLOG_INFO("字符串 \"{}\" 已编码到列 {}，编码后长度: {}", str, col_idx,
                  encoded_string.size());
    }

    // 对每一列进行inverse_encode处理
    std::vector<uint64_t> input_record(N);
    for (size_t col = 0; col < strings.size(); col++) {
      // 提取列数据
      for (size_t i = 0; i < N; i++) {
        input_record[i] = data[i][col];
      }

      // 对列进行inverse_encode
      inverse_encode(input_record);

      // 将处理后的数据写回
      for (size_t i = 0; i < N; i++) {
        data[i][col] = input_record[i];
      }

      SPDLOG_INFO("列 {} inverse_encode完成", col);
    }
  }

  const std::vector<std::vector<uint64_t>> &get_data() const { return data; }

  size_t get_string_count() const { return strings.size(); }

  const std::string &get_string(size_t index) const {
    if (index >= strings.size()) {
      throw std::out_of_range("字符串索引超出范围");
    }
    return strings[index];
  }

  size_t get_max_string_length() const { return max_string_length; }

  void print_info() const {
    SPDLOG_INFO("=== PIR 字符串数据库信息 ===");
    SPDLOG_INFO("总字符串数量: {}", strings.size());
    SPDLOG_INFO("最大字符串长度: {}", max_string_length);
  }
};

// =================== PIR服务器端 ===================

/**
 * @brief PIR字符串服务器类
 *
 * 处理客户端的加密查询，返回加密响应，保护客户端查询隐私
 */
class PIRStringServer {
private:
  std::vector<std::vector<uint64_t>> pir_database;
  int32_t r; // 打包数量
  uint64_t *datacrt; // CRT转换后的数据库数据
  size_t num_words;  // datacrt的总字数
  int32_t N1;        // Baby-step大小

public:
  PIRStringServer(int32_t pack_size = 1)
      : r(pack_size), datacrt(nullptr), num_words(0), N1(128) {}

  ~PIRStringServer() {
    if (datacrt) {
      free(datacrt);
    }
  }

  /**
   * @brief 设置PIR服务器的数据库
   * @param data 已经编码并inverse_encode处理过的数据库矩阵
   */
  void setup_database(const std::vector<std::vector<uint64_t>> &data, Timer &timer) {
    pir_database = data;

    SPDLOG_INFO("PIR服务器数据库设置完成");
    SPDLOG_INFO("数据库大小: {} x {}", pir_database.size(),
                pir_database[0].size());

    // 显示数据库的一部分用于调试
    SPDLOG_INFO("数据库预览（前5行前5列）:");
    for (size_t i = 0; i < std::min((size_t)5, pir_database.size()); i++) {
      SPDLOG_INFO("{}",
                  fmt::join(pir_database[i].begin(),
                            pir_database[i].begin() +
                                std::min((size_t)5, pir_database[i].size()),
                            ", "));
    }

    // 数据库预处理：转换为BSGS-NTT格式和CRT表示
    std::vector<std::vector<uint64_t>> data_ntt(N / 2,
                                                std::vector<uint64_t>(N, 0));

    num_words = N * N / 2;
    datacrt = (uint64_t *)aligned_alloc(64, sizeof(uint64_t) * num_words * r);

    // 1. 将数据库转换为BSGS-NTT格式
    database_tobsgsntt(data_ntt, pir_database, crtMod, N1);
    SPDLOG_INFO("数据库BSGS-NTT转换完成");

    // 2. 转换为CRT表示
    database_tocrt(datacrt, data_ntt, N1);
    SPDLOG_INFO("数据库CRT转换完成");

    timer.setTimePoint("Server_DatabaseSetup");
  }

  /**
   * @brief 处理PIR查询
   * @param rlwe_query RLWE查询密文（用于列选择）
   * @param rgsw_query RGSW查询密文（用于行选择）
   * @param autoKey 客户端提供的BSGS自同态密钥
   * @param packingKey 客户端提供的打包密钥
   * @param permutations 客户端提供的置换矩阵
   * @param timer 性能计时器
   * @return 加密的响应
   */
  RlweCiphertext
  process_query(std::vector<RlweCiphertext> &rlwe_query,
                const RGSWCiphertext &rgsw_query, const AutoKeyBSGSRNS &autoKey,
                const AutoKey &packingKey,
                std::vector<std::vector<int32_t>> &permutations, Timer &timer) {

    SPDLOG_INFO("开始处理PIR查询...");

    // 1. 第一维折叠：矩阵-向量乘法
    std::vector<RlweCiphertext> result(r, RlweCiphertext(N, crtMod));
    matrix_vector_mul_bsgs_rns_crt_large(result, rlwe_query, datacrt, autoKey,
                                         N1, permutations, r);
    SPDLOG_INFO("矩阵-向量乘法完成");
    timer.setTimePoint("Server_MatrixVectorMultiplication");

    // 2. 第二维折叠：外部乘积
    std::vector<RlweCiphertext> ext_rlwes;
    for (size_t i = 0; i < r; i++) {
      ext_rlwes.push_back(RlweCiphertext(N, crtMod));
    }

    for (size_t j = 0; j < r; j++) {
      externalProduct(ext_rlwes[j], result[j], rgsw_query);
    }
    SPDLOG_INFO("外部乘积完成");
    timer.setTimePoint("Server_ExternalProduct");

    // 3. 结果打包
    RlweCiphertext result_output(N, crtMod);
    packingRLWEs(result_output, ext_rlwes, packingKey);
    SPDLOG_INFO("响应打包完成");
    timer.setTimePoint("Server_ResponsePacking");

    // datacrt不再在此处释放，因为它现在是成员变量
    // free(datacrt);
    return result_output;
  }
};

// =================== PIR客户端 ===================

/**
 * @brief PIR字符串客户端类
 *
 * 生成加密查询，处理服务器响应，确保查询隐私性
 */
class PIRStringClient {
private:
  Secret queryKey;
  int32_t r;

public:
  PIRStringClient(int32_t pack_size = 1)
      : queryKey(crtMod, false), r(pack_size) {}

  /**
   * @brief 生成PIR查询和所有必要的密钥
   * @param target_string_index 要查询的字符串索引（即列索引）
   * @return 查询数据包，包含查询密文和所有必要的密钥
   */
  struct QueryPackage {
    std::vector<RlweCiphertext> rlwe_query;
    RGSWCiphertext rgsw_query;
    AutoKeyBSGSRNS autoKey;
    AutoKey packingKey;
    std::vector<std::vector<int32_t>> permutations;

    QueryPackage()
        : rlwe_query(1, RlweCiphertext(N, crtMod)),
          rgsw_query(N, crtMod, 2, 0x01 << 20, 0x01 << 18),
          autoKey(N, crtMod, bsMod), packingKey(N, crtMod, 4, 0, 0x01 << 14) {
      rlwe_query.push_back(RlweCiphertext(N, bsMod));
    }
  };

  QueryPackage generate_query(uint32_t target_string_index) {

    SPDLOG_INFO("正在为字符串索引 {} 生成查询...", target_string_index);

    QueryPackage package;

    // 1. 生成RLWE查询密文（用于列选择）
    query_bsgs_rns(package.rlwe_query, queryKey, target_string_index);
    SPDLOG_INFO("RLWE查询生成完成（列={}）", target_string_index);

    // 2. 生成RGSW查询密文（用于行选择，这里我们选择第0行作为示例）
    uint32_t target_packing = 0; // 简化：总是选择第0行
    package.rgsw_query.keyGen(queryKey, target_packing, true);
    SPDLOG_INFO("RGSW查询生成完成（行={}）", target_packing);

    // 3. 生成BSGS自同态密钥
    int32_t N1 = 128;
    package.autoKey.bsgsKeyGen(queryKey, N1);
    SPDLOG_INFO("BSGS自同态密钥生成完成");

    // 4. 生成打包密钥
    package.packingKey.keyGen(queryKey, r, true);
    SPDLOG_INFO("打包密钥生成完成");

    // 5. 计算置换矩阵
    package.permutations.resize(N1,
                                std::vector<int32_t>(queryKey.getLength(), 0));
    compute_permutation_matrix(package.permutations, N1, queryKey.getLength());
    SPDLOG_INFO("置换矩阵计算完成");

    return package;
  }

  /**
   * @brief 解密PIR响应
   * @param response 服务器返回的加密响应
   * @return 解密后的数据
   */
  std::vector<uint64_t> decrypt_response(RlweCiphertext &response) {
    SPDLOG_INFO("开始解密PIR响应...");

    std::vector<uint64_t> decrypted_message(queryKey.getLength());
    decrypt_bsgs_total(decrypted_message, response, queryKey, r);

    std::transform(decrypted_message.begin(), decrypted_message.end(),
                   decrypted_message.begin(),
                   [](uint64_t val) { return val % bsgsp; });

    SPDLOG_INFO("PIR响应解密完成");

    return decrypted_message;
  }
};

// =================== 主演示函数 ===================

// 前向声明
void pir_string_demo();

void pir_string_demo() {
  SPDLOG_INFO("=================== PIR字符串检索Demo ===================");

  // 创建性能计时器
  Timer timer;
  timer.setTimePoint("Begin");

  // 设置打包数量，影响数据库大小和吞吐量
  const int32_t r = 64;

  // 1. 创建PIR字符串数据库
  PIRStringDatabase db(r);

  // 2. 添加测试字符串
  std::vector<std::string> test_strings = {
      "Hello",  "World",   "PIR",         "Test",      "Demo",
      "String", "Privacy", "Information", "Retrieval", "BSGS"};

  SPDLOG_INFO("正在构建字符串数据库...");
  for (const auto &str : test_strings) {
    db.add_string(str);
  }
  timer.setTimePoint("DatabaseConstruction");

  db.print_info();

  // 3. 创建PIR服务器和客户端
  PIRStringServer server(r);
  PIRStringClient client(r);
  timer.setTimePoint("KeyGeneration");

  // 4. 设置服务器数据库
  SPDLOG_INFO("正在设置PIR服务器...");
  server.setup_database(db.get_data(), timer);
  timer.setTimePoint("ServerSetup"); // 这个计时点现在会包含内部的BSGSNTT和CRT转换时间

  // 5. 随机选择要查询的字符串
  uint32_t target_string_index = rand() % db.get_string_count();
  std::string expected_string = db.get_string(target_string_index);

  SPDLOG_INFO("准备查询字符串[{}] \"{}\"", target_string_index,
              expected_string);

  // 6. 客户端生成查询和所有必要的密钥
  SPDLOG_INFO("正在生成PIR查询和密钥...");
  auto query_package = client.generate_query(target_string_index);
  timer.setTimePoint("QueryGeneration");

  // 7. 服务器处理查询
  SPDLOG_INFO("正在处理PIR查询...");
  auto response = server.process_query(
      query_package.rlwe_query, query_package.rgsw_query, query_package.autoKey,
      query_package.packingKey, query_package.permutations, timer);
  timer.setTimePoint("QueryProcessing");

  // 8. 客户端解密响应
  SPDLOG_INFO("正在解密PIR响应...");
  auto decrypted_data = client.decrypt_response(response);
  timer.setTimePoint("Decryption");

  // 9. 解码为字符串
  SPDLOG_INFO("正在解码为字符串...");
  std::string retrieved_string = decode_string_simple(decrypted_data);
  timer.setTimePoint("Decoding");

  // 10. 验证结果
  SPDLOG_INFO("=================== 结果验证 ===================");
  SPDLOG_INFO("预期字符串: \"{}\"", expected_string);
  SPDLOG_INFO("检索字符串: \"{}\"", retrieved_string);

  // 显示解密的原始数据
  SPDLOG_INFO("解密的原始数据: ({}) {}", retrieved_string.length(),
              fmt::join(decrypted_data.begin(),
                        decrypted_data.begin() +
                            std::min(decrypted_data.size(), (size_t)20),
                        ", "));

  bool success = (retrieved_string == expected_string);
  SPDLOG_INFO("验证结果: {}", success ? "✓ 成功" : "✗ 失败");

  // ========== 性能统计报告 ==========
  SPDLOG_INFO("");
  SPDLOG_INFO("=================== 性能统计 ===================");
  SPDLOG_INFO("\n{}", timer);

  double totalTime = timer.getTotalTime();

  // 获取关键时间点
  auto databaseConstruction = timer.getTimePoint("DatabaseConstruction");
  auto keyGeneration = timer.getTimePoint("KeyGeneration");
  auto serverSetup = timer.getTimePoint("ServerSetup");
  auto queryGeneration = timer.getTimePoint("QueryGeneration");
  auto decryption = timer.getTimePoint("Decryption");
  auto decoding = timer.getTimePoint("Decoding");

  // 服务器内部时间点 (现在在ServerSetup内部)
  auto serverMatrixVectorMultiplication = timer.getTimePoint("Server_MatrixVectorMultiplication");
  auto serverExternalProduct = timer.getTimePoint("Server_ExternalProduct");
  auto serverResponsePacking = timer.getTimePoint("Server_ResponsePacking");
  auto serverOverallSetup = timer.getTimePoint("Server_DatabaseSetup");

  SPDLOG_INFO("关键阶段时间分布:");
  SPDLOG_INFO("  - 数据库构建:           {:.2f}%",
              (databaseConstruction.second / totalTime) * 100.0);
  SPDLOG_INFO("  - 密钥生成 (客户端初始化): {:.2f}%",
              (keyGeneration.second / totalTime) * 100.0);
  SPDLOG_INFO("  - 服务器设置 (含预处理): {:.2f}%",
              (serverOverallSetup.second / totalTime) * 100.0);
  SPDLOG_INFO("  - 查询生成:             {:.2f}%",
              (queryGeneration.second / totalTime) * 100.0);
  SPDLOG_INFO("  - 服务器-矩阵向量乘法:  {:.2f}%",
              (serverMatrixVectorMultiplication.second / totalTime) * 100.0);
  SPDLOG_INFO("  - 服务器-外部乘积:      {:.2f}%",
              (serverExternalProduct.second / totalTime) * 100.0);
  SPDLOG_INFO("  - 服务器-响应打包:      {:.2f}%",
              (serverResponsePacking.second / totalTime) * 100.0);
  SPDLOG_INFO("  - 解密:                 {:.2f}%",
              (decryption.second / totalTime) * 100.0);
  SPDLOG_INFO("  - 解码:                 {:.2f}%",
              (decoding.second / totalTime) * 100.0);

  // 服务器/客户端处理时间
  double serverTime = serverOverallSetup.second + \
                      serverMatrixVectorMultiplication.second +
                      serverExternalProduct.second +
                      serverResponsePacking.second;
  double clientTime = keyGeneration.second + queryGeneration.second + \
                      decryption.second + decoding.second;

  SPDLOG_INFO("服务器总处理时间: {:.2f} ms", serverTime);
  SPDLOG_INFO("客户端总处理时间: {:.2f} ms", clientTime);

  // 通信开销
  size_t rlwe_query_size = query_package.rlwe_query[0].b.size() +\
                           query_package.rlwe_query[1].b.size();
  size_t rgsw_query_size = query_package.rgsw_query.getEllnum() * 2 * 2 * N / 2;

  double querySize = (rlwe_query_size + rgsw_query_size) *\
                     sizeof(uint64_t) / (1024.0);
  double responseSize = (response.a.size() + response.b.size()) *\
                        sizeof(uint64_t) / (1024.0);

  SPDLOG_INFO("通信开销:");
  SPDLOG_INFO("  - 查询大小:    {:.2f} KB", querySize);
  SPDLOG_INFO("  - 响应大小:    {:.2f} KB", responseSize);
  SPDLOG_INFO("  - 总计:        {:.2f} KB", querySize + responseSize);

  // 数据库信息
  size_t db_rows = db.get_data().size();
  size_t db_cols = 0;
  if (db_rows > 0) {
      db_cols = db.get_data()[0].size();
  }
  double dataSizeMB = (double)(db_rows * db_cols * sizeof(uint64_t)) / (1024.0 * 1024.0);

  SPDLOG_INFO("数据库配置:");
  SPDLOG_INFO("  - 打包数量 (r): {}", r);
  SPDLOG_INFO("  - 数据库矩阵大小: {} 行 x {} 列 (uint64_t)", db_rows, db_cols);
  SPDLOG_INFO("  - 数据库内存占用: {:.2f} MB", dataSizeMB);
  SPDLOG_INFO("  - 字符串数量: {}", db.get_string_count());
  SPDLOG_INFO("  - 最大字符串长度: {}", db.get_max_string_length());

  SPDLOG_INFO("=================== Demo完成 ===================");
}

/**
 * @brief 主函数
 */
int main() {
  SPDLOG_INFO("PIR字符串检索Demo启动...");

  // 设置随机种子
  srand(time(NULL));

  pir_string_demo();

  return 0;
}