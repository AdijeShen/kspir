#include <gtest/gtest.h>
#include "utils.h"
#include <spdlog/spdlog.h>

namespace kspir {
namespace test {

class NTTIndexTest : public ::testing::Test {
protected:
    void SetUp() override {
        spdlog::set_level(spdlog::level::debug);
    }
};

// 测试不同长度下的hexl_ntt_index
TEST_F(NTTIndexTest, TestHexlNTTIndexDifferentLengths) {
    // 测试N=8的情况
    std::vector<int32_t> hexl_ntt_index_8(8, 0);
    std::vector<int32_t> rotate_index_8(8, 0);
    compute_hexl_rotate_indexes(hexl_ntt_index_8, rotate_index_8, 8);
    
    SPDLOG_INFO("N=8 hexl_ntt_index: {}", fmt::join(hexl_ntt_index_8, ","));
    SPDLOG_INFO("N=8 rotate_index: {}", fmt::join(rotate_index_8, ","));

    // 测试N=16的情况
    std::vector<int32_t> hexl_ntt_index_16(16, 0);
    std::vector<int32_t> rotate_index_16(16, 0);
    compute_hexl_rotate_indexes(hexl_ntt_index_16, rotate_index_16, 16);
    
    SPDLOG_INFO("N=16 hexl_ntt_index: {}", fmt::join(hexl_ntt_index_16, ","));
    SPDLOG_INFO("N=16 rotate_index: {}", fmt::join(rotate_index_16, ","));
}

// 测试hexl_ntt_index的规律
TEST_F(NTTIndexTest, TestHexlNTTIndexPattern) {
    const int32_t N = 32;
    std::vector<int32_t> hexl_ntt_index(N, 0);
    std::vector<int32_t> rotate_index(N, 0);
    compute_hexl_rotate_indexes(hexl_ntt_index, rotate_index, N);

    // 分析奇偶性
    std::vector<int32_t> even_indices;
    std::vector<int32_t> odd_indices;
    for (size_t i = 0; i < N; i++) {
        if (hexl_ntt_index[i] % 2 == 0) {
            even_indices.push_back(hexl_ntt_index[i]);
        } else {
            odd_indices.push_back(hexl_ntt_index[i]);
        }
    }

    SPDLOG_INFO("Even indices: {}", fmt::join(even_indices, ","));
    SPDLOG_INFO("Odd indices: {}", fmt::join(odd_indices, ","));

    // 分析索引的分布
    std::vector<int32_t> index_diffs;
    for (size_t i = 1; i < N; i++) {
        index_diffs.push_back(hexl_ntt_index[i] - hexl_ntt_index[i-1]);
    }
    SPDLOG_INFO("Index differences: {}", fmt::join(index_diffs, ","));
}

// 测试hexl_ntt_index与NTT计算的关系
TEST_F(NTTIndexTest, TestHexlNTTIndexWithPermutation) {
    const int32_t N = 16;
    const int32_t N1 = 2;
    
    // 计算置换矩阵
    std::vector<std::vector<int32_t>> permutations(N1, std::vector<int32_t>(N, 0));
    compute_permutation_matrix(permutations, N1, N);

    // 分析每个置换矩阵的特征
    for (int i = 0; i < N1; i++) {
        // 计算每个置换矩阵的逆
        std::vector<int32_t> inverse_perm(N);
        for (int j = 0; j < N; j++) {
            inverse_perm[permutations[i][j]] = j;
        }
        
        SPDLOG_INFO("Permutation[{}] inverse: {}", i, fmt::join(inverse_perm, ","));
        
        // 计算置换矩阵的循环
        std::vector<bool> visited(N, false);
        std::vector<std::vector<int32_t>> cycles;
        for (int j = 0; j < N; j++) {
            if (!visited[j]) {
                std::vector<int32_t> cycle;
                int32_t current = j;
                do {
                    cycle.push_back(current);
                    visited[current] = true;
                    current = permutations[i][current];
                } while (current != j);
                cycles.push_back(cycle);
            }
        }
        
        SPDLOG_INFO("Permutation[{}] cycles:", i);
        for (const auto& cycle : cycles) {
            SPDLOG_INFO("  Cycle: {}", fmt::join(cycle, " -> "));
        }
    }
}




} // namespace test
} // namespace kspir 