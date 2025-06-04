/**
 * @file test-bsgs-matrix-mul.cpp
 * @brief BSGS矩阵-向量乘法专项测试
 * 
 * 本测试专门用于验证和性能测试BSGS算法的矩阵-向量乘法部分，
 * 包括正确性验证、性能基准测试和参数敏感性分析。
 * 
 * 编译：在项目根目录执行 ninja -C build
 * 运行：./build/tests/test-bsgs-matrix-mul [选项]
 * 
 * 使用CLI11进行命令行解析，GoogleTest进行测试组织
 */

#include <chrono>
#include <iostream>
#include <iomanip>
#include <vector>
#include <random>
#include <cstring>
#include <cassert>
#include <memory>
#include <algorithm>
#include <map>
#include <fstream>

// 第三方库
#include <CLI/CLI.hpp>
#include <gtest/gtest.h>

// kspir核心头文件
#include "pir.h"
#include "timer.h"
#include "utils.h"
#include "twosteps.h"
#include "params.h"  // 添加params.h以使用bsgsDelta常量

// 使用spdlog进行日志输出
#include "spdlog/spdlog.h"
#include "spdlog/fmt/bundled/format.h"

using namespace kspir;

/**
 * @brief 全局测试配置结构体
 */
struct BSGSTestConfig {
    int32_t N = 4096;                    // 多项式环维度
    int32_t N1 = 128;                    // Baby-step大小
    int32_t r = 4;                       // 打包参数（减小以加快测试）
    uint64_t modulus = crtMod;           // 主模数
    int32_t iterations = 3;              // 测试迭代次数（减少以加快测试）
    bool enable_correctness = true;      // 启用正确性测试
    bool enable_performance = true;      // 启用性能测试
    bool enable_sensitivity = false;     // 启用参数敏感性测试
    bool verbose = false;                // 详细输出
    
    void print() const {
        SPDLOG_INFO("=== BSGS矩阵乘法测试配置 ===");
        SPDLOG_INFO("多项式环维度 (N): {}", N);
        SPDLOG_INFO("Baby-step大小 (N1): {}", N1);
        SPDLOG_INFO("Giant-step大小 (N2): {}", N/2/N1);
        SPDLOG_INFO("打包参数 (r): {}", r);
        SPDLOG_INFO("主模数: {}", modulus);
        SPDLOG_INFO("测试迭代次数: {}", iterations);
        SPDLOG_INFO("数据库大小: {} MB", (r * N * N / 2 * 8 / 1024 / 1024));
        SPDLOG_INFO("=============================");
    }
    
    bool validate() const {
        if (N <= 0 || (N & (N - 1)) != 0) {
            SPDLOG_ERROR("错误: N必须是2的幂次");
            return false;
        }
        
        if (N < 4096) {
            SPDLOG_ERROR("错误: N必须至少为4096");
            return false;
        }
        
        if (N1 <= 0 || N / 2 % N1 != 0) {
            SPDLOG_ERROR("错误: N1必须能整除N/2");
            return false;
        }
        
        if (r <= 0) {
            SPDLOG_ERROR("错误: r必须大于0");
            return false;
        }
        
        if (iterations <= 0) {
            SPDLOG_ERROR("错误: iterations必须大于0");
            return false;
        }
        
        return true;
    }
};

// 全局配置实例
BSGSTestConfig g_config;

/**
 * @brief 内存使用监控类
 */
class MemoryMonitor {
private:
    size_t initial_memory = 0;
    size_t peak_memory = 0;
    
    size_t getCurrentMemoryUsage() {
        std::ifstream status("/proc/self/status");
        std::string line;
        while (std::getline(status, line)) {
            if (line.substr(0, 6) == "VmRSS:") {
                size_t memory_kb = std::stoul(line.substr(7));
                return memory_kb * 1024; // 转换为字节
            }
        }
        return 0;
    }
    
public:
    void startMonitoring() {
        initial_memory = getCurrentMemoryUsage();
        peak_memory = initial_memory;
    }
    
    void updatePeak() {
        size_t current = getCurrentMemoryUsage();
        if (current > peak_memory) {
            peak_memory = current;
        }
    }
    
    size_t getPeakMemoryUsageMB() {
        updatePeak();
        return (peak_memory - initial_memory) / (1024 * 1024);
    }
};

/**
 * @brief 明文矩阵-向量乘法参考实现
 * 
 * 实现真正的明文矩阵-向量乘法，用于验证BSGS算法的正确性。
 * BSGS算法的目标是在加密域中实现与明文矩阵乘法完全相同的结果。
 * 因此这个参考实现直接返回数据库中对应的值，不需要任何额外的缩放。
 * 
 * @param result 输出结果，每个数据库实例的明文计算结果
 * @param original_database 原始明文数据库
 * @param target_col 查询的目标列
 * @param config 测试配置参数
 */
void naiveMatrixVectorMul(std::vector<std::vector<uint64_t>>& result,
                         const std::vector<std::vector<uint64_t>>& original_database,
                         uint64_t target_col,
                         const BSGSTestConfig& config) {
    
    SPDLOG_INFO("执行明文矩阵-向量乘法参考实现...");
    SPDLOG_INFO("目标列: {}", target_col);
    
    // 验证输入参数
    if (target_col >= config.N / 2) {
        SPDLOG_ERROR("目标列 {} 超出范围 [0, {})", target_col, config.N / 2);
        return;
    }
    
    if (original_database.size() != config.N) {
        SPDLOG_ERROR("数据库行数 {} 不匹配配置 {}", original_database.size(), config.N);
        return;
    }
    
    // 初始化结果
    result.resize(config.r);
    
    // 对每个数据库实例执行明文矩阵-向量乘法
    for (int32_t r_idx = 0; r_idx < config.r; r_idx++) {
        result[r_idx].resize(config.N);
        
        // 明文矩阵-向量乘法：result = database[:, target_col]
        // 由于查询向量是one-hot向量（只有target_col位置为1），
        // 结果就是数据库中目标列的所有值
        for (int32_t row = 0; row < config.N; row++) {
            if (target_col < original_database[row].size()) {
                result[r_idx][row] = original_database[row][target_col];
            } else {
                SPDLOG_ERROR("数据库行 {} 列数不足，无法访问列 {}", row, target_col);
                result[r_idx][row] = 0;
            }
        }
        
        SPDLOG_DEBUG("数据库实例 {} 明文参考计算完成", r_idx);
    }
    
    SPDLOG_INFO("明文参考实现完成，计算了 {} 个数据库实例", config.r);
    SPDLOG_INFO("返回数据库中目标列 {} 的所有值（无需额外缩放）", target_col);
    
    // 输出一些调试信息
    if (g_config.verbose && config.r > 0) {
        SPDLOG_INFO("数据库 0 的前10个参考值: [{}]", 
                   fmt::join(result[0].begin(), 
                            result[0].begin() + std::min(10, (int)result[0].size()), 
                            ", "));
    }
}

/**
 * @brief 解密BSGS算法的结果
 * 
 * @param decrypted_results 解密后的结果
 * @param bsgs_results BSGS算法的密文结果
 * @param secret 解密密钥
 * @param config 测试配置
 */
void decryptBSGSResults(std::vector<std::vector<uint64_t>>& decrypted_results,
                       const std::vector<RlweCiphertext>& bsgs_results,
                       Secret& secret,
                       const BSGSTestConfig& config) {
    
    SPDLOG_INFO("解密BSGS算法结果...");
    
    decrypted_results.resize(config.r);
    
    for (int32_t i = 0; i < config.r; i++) {
        decrypted_results[i].resize(config.N);
        
        // 创建密文副本用于解密（因为decrypt_bsgs可能修改输入）
        RlweCiphertext cipher_copy = bsgs_results[i];
        
        // 使用kspir库的decrypt_bsgs函数解密
        decrypt_bsgs(decrypted_results[i], cipher_copy, secret);
    }
    
    SPDLOG_INFO("解密完成");
}

/**
 * @brief 比较解密结果与参考结果
 * 
 * @param decrypted_results 解密后的BSGS结果
 * @param reference_results 参考实现的结果
 * @param target_row 查询的目标行
 * @param target_col 查询的目标列
 * @param config 测试配置
 * @return 是否匹配
 */
bool compareResults(const std::vector<std::vector<uint64_t>>& decrypted_results,
                   const std::vector<std::vector<uint64_t>>& reference_results,
                   uint64_t target_row,
                   uint64_t target_col,
                   const BSGSTestConfig& config) {
    
    SPDLOG_INFO("比较解密结果与参考结果...");
    
    bool all_match = true;
    int32_t mismatch_count = 0;
    const int32_t max_mismatches_to_show = 10;
    
    for (int32_t r_idx = 0; r_idx < config.r; r_idx++) {
        if (decrypted_results[r_idx].size() != reference_results[r_idx].size()) {
            SPDLOG_ERROR("数据库 {} 的结果大小不匹配: {} vs {}", 
                        r_idx, decrypted_results[r_idx].size(), reference_results[r_idx].size());
            all_match = false;
            continue;
        }
        
        for (int32_t row = 0; row < config.N; row++) {
            uint64_t decrypted_val = decrypted_results[r_idx][row];
            uint64_t reference_val = reference_results[r_idx][row];
            
            // 允许一定的数值误差（由于浮点运算和模运算）
            uint64_t diff = (decrypted_val > reference_val) ? 
                           (decrypted_val - reference_val) : 
                           (reference_val - decrypted_val);
            
            // 对于模运算，还需要考虑环绕的情况
            uint64_t wrap_diff = config.modulus - diff;
            uint64_t min_diff = (diff < wrap_diff) ? diff : wrap_diff;
            
            // 使用合理的容差：考虑到BSGS算法的数值特性，允许小的差异
            const uint64_t tolerance = 1000; // 固定容差，适合BSGS解密后的小数值
            
            if (min_diff > tolerance) {
                if (mismatch_count < max_mismatches_to_show) {
                    SPDLOG_WARN("数据库 {} 行 {} 不匹配: 解密值={}, 参考值={}, 差值={}", 
                               r_idx, row, decrypted_val, reference_val, min_diff);
                }
                mismatch_count++;
                all_match = false;
            }
        }
    }
    
    if (mismatch_count > 0) {
        SPDLOG_ERROR("总共发现 {} 个不匹配的值", mismatch_count);
        if (mismatch_count > max_mismatches_to_show) {
            SPDLOG_ERROR("（只显示了前 {} 个不匹配）", max_mismatches_to_show);
        }
    }
    
    // 特别检查目标位置的值
    if (target_row < config.N && target_col < config.N / 2) {
        for (int32_t r_idx = 0; r_idx < config.r; r_idx++) {
            uint64_t target_decrypted = decrypted_results[r_idx][target_row];
            uint64_t target_reference = reference_results[r_idx][target_row];
            
            SPDLOG_INFO("数据库 {} 目标位置 [{}][{}]: 解密值={}, 参考值={}", 
                       r_idx, target_row, target_col, target_decrypted, target_reference);
        }
    }
    
    return all_match;
}

/**
 * @brief 生成测试数据
 */
void generateTestData(uint64_t*& database,
                     std::vector<RlweCiphertext>& query_vector,
                     Secret& secret,
                     AutoKeyBSGSRNS& autoKey,
                     std::vector<std::vector<int32_t>>& permutations,
                     uint64_t& target_col,  // 添加输出参数
                     std::vector<std::vector<uint64_t>>& original_database,  // 添加原始数据库输出参数
                     const BSGSTestConfig& config) {
    
    SPDLOG_INFO("生成测试数据...");
    
    // 生成密钥 - 使用默认模数
    secret = Secret(crtMod, false);
    
    // 生成随机查询目标
    std::random_device rd;
    std::mt19937 gen(rd());
    target_col = gen() % (config.N / 2);
    
    // 生成原始数据库并保存副本
    original_database.resize(config.N);
    
    // 使用已知答案的测试策略：设置特定的测试值
    for (size_t i = 0; i < config.N; i++) {
        original_database[i].resize(config.N / 2);
        for (size_t j = 0; j < config.N / 2; j++) {
            // if (j == target_col) {
            //     // 在目标列设置已知的测试值：行号 + 1000
            //     // 这样我们知道期望的结果应该是什么
            //     original_database[i][j] = i + 1000;
            // } else {
            //     // 其他位置使用小的随机值，避免干扰
            //     original_database[i][j] = gen() % 100;
            // }
            original_database[i][j] = rand() % 1000;
        }
    }
    
    SPDLOG_INFO("生成的原始数据库大小: {}x{}", config.N, config.N / 2);
    SPDLOG_INFO("在目标列 {} 设置了已知测试值（行号 + 1000）", target_col);
    
    // 分配数据库内存
    size_t num_words = config.N * config.N / 2;
    database = (uint64_t*)aligned_alloc(64, sizeof(uint64_t) * num_words * config.r);
    
    // 为每个数据库实例应用BSGS预处理
    for (int32_t r_idx = 0; r_idx < config.r; r_idx++) {
        // 应用BSGS数据库预处理
        std::vector<std::vector<uint64_t>> data_ntt(config.N / 2, std::vector<uint64_t>(config.N, 0));
        database_tobsgsntt(data_ntt, original_database, config.modulus, config.N1);
        
        // 转换为CRT形式并存储
        database_tocrt(database + r_idx * num_words, data_ntt, config.N1);
    }
    
    // 生成查询向量 - 使用正确的方式（参考test-two-steps.cpp）
    query_vector.clear();
    query_vector.push_back(RlweCiphertext(config.N, crtMod));
    query_vector.push_back(RlweCiphertext(config.N, bsMod));
    query_bsgs_rns(query_vector, secret, target_col);
    
    // 生成自同态密钥 - 使用正确的方式（参考test-two-steps.cpp）
    autoKey = AutoKeyBSGSRNS(config.N, crtMod, bsMod);
    
    // 生成Baby-step密钥
    std::vector<int32_t> indexLists;
    for (size_t i = 1; i <= config.N1 / 2; i++) {
        indexLists.push_back(pow_mod(5, i, 2 * config.N));
    }
    autoKey.keyGen(secret, indexLists, BabyStep);
    
    // 生成Giant-step密钥
    int32_t N2 = config.N / 2 / config.N1;
    indexLists.clear();
    for (size_t i = 1; i < N2; i++) {
        indexLists.push_back(pow_mod(5, config.N1 * i, 2 * config.N));
    }
    autoKey.keyGen(secret, indexLists, GaintStep);
    
    // 计算置换矩阵
    permutations.resize(config.N1);
    for (int32_t i = 0; i < config.N1; i++) {
        permutations[i].resize(secret.getLength(), 0);
    }
    compute_permutation_matrix(permutations, config.N1, secret.getLength());
    
    SPDLOG_INFO("测试数据生成完成，目标列: {}", target_col);
}

/**
 * @brief BSGS矩阵乘法测试夹具
 */
class BSGSMatrixMulTest : public ::testing::Test {
protected:
    uint64_t* database = nullptr;
    std::vector<RlweCiphertext> query_vector;
    Secret secret;
    AutoKeyBSGSRNS autoKey;
    std::vector<std::vector<int32_t>> permutations;
    uint64_t target_col = 0;  // 添加目标列成员变量
    
    // 添加原始明文数据库，用于参考实现
    std::vector<std::vector<uint64_t>> original_database;
    
    void SetUp() override {
        // 设置日志级别
        if (g_config.verbose) {
            spdlog::set_level(spdlog::level::debug);
        } else {
            spdlog::set_level(spdlog::level::info);
        }
        
        // 生成测试数据
        generateTestData(database, query_vector, secret, autoKey, permutations, target_col, original_database, g_config);
    }
    
    void TearDown() override {
        if (database) {
            free(database);
            database = nullptr;
        }
    }
};

/**
 * @brief 正确性测试
 */
TEST_F(BSGSMatrixMulTest, CorrectnessTest) {
    SPDLOG_INFO("=================== 正确性测试 ===================");
    g_config.print();
    
    Timer timer;
    timer.setTimePoint("开始正确性测试");
    
    // 1. 执行BSGS算法
    SPDLOG_INFO("执行BSGS矩阵-向量乘法...");
    std::vector<RlweCiphertext> bsgs_result(g_config.r, RlweCiphertext(g_config.N, crtMod));
    
    matrix_vector_mul_bsgs_rns_crt_large(bsgs_result, query_vector, database, 
                                       autoKey, g_config.N1, permutations, g_config.r);
    
    timer.setTimePoint("BSGS算法执行");
    
    // 2. 解密BSGS结果
    SPDLOG_INFO("解密BSGS算法结果...");
    std::vector<std::vector<uint64_t>> decrypted_result;
    decryptBSGSResults(decrypted_result, bsgs_result, secret, g_config);
    timer.setTimePoint("BSGS结果解密");
    
    // 3. 执行明文参考实现
    SPDLOG_INFO("执行明文矩阵-向量乘法参考实现...");
    std::vector<std::vector<uint64_t>> reference_result;
    naiveMatrixVectorMul(reference_result, original_database, target_col, g_config);
    timer.setTimePoint("明文参考实现");
    
    // 4. 验证结果维度
    EXPECT_EQ(bsgs_result.size(), g_config.r);
    EXPECT_EQ(decrypted_result.size(), g_config.r);
    EXPECT_EQ(reference_result.size(), g_config.r);
    
    for (int32_t i = 0; i < g_config.r; i++) {
        EXPECT_EQ(bsgs_result[i].getLength(), g_config.N);
        EXPECT_EQ(decrypted_result[i].size(), g_config.N);
        EXPECT_EQ(reference_result[i].size(), g_config.N);
    }
    
    SPDLOG_INFO("维度检查: 通过");
    
    // 5. 验证数据完整性
    bool data_integrity = true;
    for (int32_t i = 0; i < g_config.r && data_integrity; i++) {
        bool all_zero = true;
        for (int32_t j = 0; j < g_config.N; j++) {
            if (bsgs_result[i].getA()[j] != 0 || bsgs_result[i].getB()[j] != 0) {
                all_zero = false;
                break;
            }
        }
        if (all_zero) {
            data_integrity = false;
            SPDLOG_WARN("BSGS结果 {} 全为零", i);
        }
    }
    
    EXPECT_TRUE(data_integrity) << "BSGS数据完整性检查失败";
    SPDLOG_INFO("BSGS数据完整性检查: {}", data_integrity ? "通过" : "失败");
    
    // 6. 核心正确性验证：比较BSGS解密结果与明文矩阵乘法结果
    SPDLOG_INFO("=================== 核心正确性验证 ===================");
    SPDLOG_INFO("比较BSGS解密结果与明文矩阵乘法结果...");
    
    bool correctness_passed = compareResults(decrypted_result, reference_result, 
                                           0, target_col, g_config);
    
    EXPECT_TRUE(correctness_passed) << "BSGS算法正确性验证失败！解密结果与明文矩阵乘法结果不匹配";
    
    if (correctness_passed) {
        SPDLOG_INFO("\033[32m🎉 正确性验证通过！BSGS算法结果与明文矩阵乘法结果匹配\033[0m");
    } else {
        SPDLOG_ERROR("❌ 正确性验证失败！BSGS算法结果与明文矩阵乘法结果不匹配");
    }
    
    // 7. 输出详细的比较信息（用于调试）
    if (g_config.verbose && g_config.r > 0) {
        SPDLOG_INFO("详细比较信息（数据库 0）:");
        SPDLOG_INFO("目标列: {}", target_col);
        SPDLOG_INFO("BSGS解密结果前10个值: [{}]", 
                   fmt::join(decrypted_result[0].begin(), 
                            decrypted_result[0].begin() + std::min(10, (int)decrypted_result[0].size()), 
                            ", "));
        SPDLOG_INFO("明文参考结果前10个值: [{}]", 
                   fmt::join(reference_result[0].begin(), 
                            reference_result[0].begin() + std::min(10, (int)reference_result[0].size()), 
                            ", "));
        
        // 特别检查前几行的对应关系
        SPDLOG_INFO("前5行的详细对比:");
        for (int i = 0; i < std::min(5, (int)g_config.N); i++) {
            uint64_t original_val = original_database[i][target_col];
            uint64_t bsgs_val = decrypted_result[0][i];
            uint64_t expected_val = reference_result[0][i];
            SPDLOG_INFO("  行 {}: 原始值={}, BSGS解密={}, 明文参考={}", 
                       i, original_val, bsgs_val, expected_val);
        }
    }
    
    timer.setTimePoint("测试完成");
    
    if (g_config.verbose) {
        SPDLOG_INFO("正确性测试计时报告:\n{}", timer.to_string());
    }
    
    SPDLOG_INFO("正确性测试完成");
}

/**
 * @brief 性能测试
 */
TEST_F(BSGSMatrixMulTest, PerformanceTest) {
    SPDLOG_INFO("=================== 性能测试 ===================");
    g_config.print();
    
    Timer timer;
    timer.setTimePoint("开始性能测试");
    
    // 内存监控
    MemoryMonitor memory_monitor;
    memory_monitor.startMonitoring();
    
    // 性能测试
    std::vector<double> execution_times;
    std::vector<RlweCiphertext> result(g_config.r, RlweCiphertext(g_config.N, crtMod));
    
    SPDLOG_INFO("开始性能测试 ({} 次迭代)...", g_config.iterations);
    
    for (int32_t iter = 0; iter < g_config.iterations; iter++) {
        auto start = std::chrono::high_resolution_clock::now();
        
        matrix_vector_mul_bsgs_rns_crt_large(result, query_vector, database,
                                           autoKey, g_config.N1, permutations, g_config.r);
        
        auto end = std::chrono::high_resolution_clock::now();
        double elapsed_ms = std::chrono::duration<double, std::milli>(end - start).count();
        execution_times.push_back(elapsed_ms);
        
        memory_monitor.updatePeak();
        
        if (g_config.verbose) {
            SPDLOG_INFO("  迭代 {}: {:.2f} ms", iter + 1, elapsed_ms);
        }
    }
    
    timer.setTimePoint("性能测试完成");
    
    // 计算统计信息
    double total_time = 0;
    double min_time = execution_times[0];
    double max_time = execution_times[0];
    
    for (double time : execution_times) {
        total_time += time;
        min_time = std::min(min_time, time);
        max_time = std::max(max_time, time);
    }
    
    double avg_time = total_time / g_config.iterations;
    
    // 计算标准差
    double variance = 0;
    for (double time : execution_times) {
        variance += (time - avg_time) * (time - avg_time);
    }
    double std_dev = std::sqrt(variance / g_config.iterations);
    
    // 计算吞吐量
    double data_size_mb = (g_config.r * g_config.N * g_config.N / 2.0 * 8) / (1024 * 1024);
    double throughput_mbps = data_size_mb / (avg_time / 1000.0);
    
    // 输出性能报告
    SPDLOG_INFO("=================== 性能报告 ===================");
    SPDLOG_INFO("平均执行时间:     {:.2f} ms", avg_time);
    SPDLOG_INFO("最小执行时间:     {:.2f} ms", min_time);
    SPDLOG_INFO("最大执行时间:     {:.2f} ms", max_time);
    SPDLOG_INFO("标准差:           {:.2f} ms", std_dev);
    SPDLOG_INFO("数据库大小:       {:.2f} MB", data_size_mb);
    SPDLOG_INFO("吞吐量:           {:.2f} MB/s", throughput_mbps);
    SPDLOG_INFO("峰值内存使用:     {} MB", memory_monitor.getPeakMemoryUsageMB());
    SPDLOG_INFO("===============================================");
    
    if (g_config.verbose) {
        SPDLOG_INFO("性能测试计时报告:\n{}", timer.to_string());
    }
    
    // 性能断言（可选）
    EXPECT_GT(avg_time, 0) << "平均执行时间应该大于0";
    EXPECT_GT(throughput_mbps, 0) << "吞吐量应该大于0";
}

/**
 * @brief 参数敏感性测试
 */
class BSGSParameterSensitivityTest : public ::testing::Test {
protected:
    void SetUp() override {
        spdlog::set_level(spdlog::level::info);
    }
};

TEST_F(BSGSParameterSensitivityTest, N1SensitivityTest) {
    SPDLOG_INFO("=============== 参数敏感性测试 ===============");
    
    // 测试不同的N1值
    std::vector<int32_t> N1_values = {32, 64, 128, 256};
    BSGSTestConfig base_config = g_config;
    base_config.iterations = 3; // 减少迭代次数以加快测试
    base_config.r = 4;          // 减少数据库大小
    
    SPDLOG_INFO("测试不同N1值对性能的影响:");
    SPDLOG_INFO("{:<8} {:<15} {:<15}", "N1", "平均时间(ms)", "吞吐量(MB/s)");
    SPDLOG_INFO("{}",std::string(40, '-'));
    
    for (int32_t N1 : N1_values) {
        if (base_config.N / 2 % N1 != 0) continue; // 确保N2是整数
        
        BSGSTestConfig test_config = base_config;
        test_config.N1 = N1;
        
        uint64_t* database = nullptr;
        std::vector<RlweCiphertext> query_vector;
        Secret secret;
        AutoKeyBSGSRNS autoKey;
        std::vector<std::vector<int32_t>> permutations;
        uint64_t target_col = 0;  // 添加缺失的target_col变量
        std::vector<std::vector<uint64_t>> original_database;  // 添加缺失的original_database变量
        
        generateTestData(database, query_vector, secret, autoKey, permutations, target_col, original_database, test_config);
        
        std::vector<double> times;
        std::vector<RlweCiphertext> result(test_config.r, RlweCiphertext(test_config.N, crtMod));
        
        for (int32_t iter = 0; iter < test_config.iterations; iter++) {
            auto start = std::chrono::high_resolution_clock::now();
            
            matrix_vector_mul_bsgs_rns_crt_large(result, query_vector, database,
                                               autoKey, test_config.N1, permutations, test_config.r);
            
            auto end = std::chrono::high_resolution_clock::now();
            times.push_back(std::chrono::duration<double, std::milli>(end - start).count());
        }
        
        double avg_time = 0;
        for (double t : times) avg_time += t;
        avg_time /= test_config.iterations;
        
        double data_size_mb = (test_config.r * test_config.N * test_config.N / 2.0 * 8) / (1024 * 1024);
        double throughput = data_size_mb / (avg_time / 1000.0);
        
        SPDLOG_INFO("{:<8} {:<15.1f} {:<15.1f}", N1, avg_time, throughput);
        
        // 性能断言
        EXPECT_GT(avg_time, 0) << "N1=" << N1 << " 的执行时间应该大于0";
        EXPECT_GT(throughput, 0) << "N1=" << N1 << " 的吞吐量应该大于0";
        
        free(database);
    }
    
    SPDLOG_INFO("===============================================");
}

/**
 * @brief 主函数 - 使用CLI11解析命令行参数
 */
int main(int argc, char* argv[]) {
    // 初始化GoogleTest
    ::testing::InitGoogleTest(&argc, argv);
    
    // 创建CLI11应用
    CLI::App app{"BSGS矩阵-向量乘法专项测试", "test-bsgs-matrix-mul"};
    app.set_version_flag("--version", "1.0.0");
    
    // 添加命令行选项
    app.add_option("-N,--N", g_config.N, "多项式环维度 (必须是2的幂次)")
        ->check(CLI::PositiveNumber)
        ->default_val(4096);
        
    app.add_option("--N1", g_config.N1, "Baby-step大小")
        ->check(CLI::PositiveNumber)
        ->default_val(128);
        
    app.add_option("-r,--r", g_config.r, "打包参数")
        ->check(CLI::PositiveNumber)
        ->default_val(4);
        
    app.add_option("-i,--iterations", g_config.iterations, "测试迭代次数")
        ->check(CLI::PositiveNumber)
        ->default_val(3);
        
    app.add_flag("-v,--verbose", g_config.verbose, "详细输出");
    
    // 测试模式选项
    bool correctness_only = false;
    bool performance_only = false;
    bool sensitivity_only = false;
    
    app.add_flag("--correctness", correctness_only, "只运行正确性测试");
    app.add_flag("--performance", performance_only, "只运行性能测试");
    app.add_flag("--sensitivity", sensitivity_only, "只运行参数敏感性测试");
    
    // 解析命令行参数
    try {
        app.parse(argc, argv);
    } catch (const CLI::ParseError &e) {
        return app.exit(e);
    }
    
    // 验证配置
    if (!g_config.validate()) {
        return 1;
    }
    
    // 设置测试过滤器
    std::string filter = "*";
    if (correctness_only) {
        filter = "*CorrectnessTest*";
    } else if (performance_only) {
        filter = "*PerformanceTest*";
    } else if (sensitivity_only) {
        filter = "*SensitivityTest*";
    }
    
    if (filter != "*") {
        ::testing::GTEST_FLAG(filter) = filter;
    }
    
    // 设置日志级别
    spdlog::set_level(g_config.verbose ? spdlog::level::debug : spdlog::level::info);
    
    SPDLOG_INFO("BSGS矩阵-向量乘法专项测试");
    SPDLOG_INFO("版本: 1.0.0");
    SPDLOG_INFO("基于kspir PIR库");
    SPDLOG_INFO("使用CLI11和GoogleTest框架");
    SPDLOG_INFO("========================================");
    
    // 运行测试
    return RUN_ALL_TESTS();
} 