#include <chrono>
#include <gtest/gtest.h>
#include <iostream>
#include <spdlog/spdlog.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include <vector>

#include "ntt.h"
#include "params.h"
#include "pir.h"
#include "utils.h"

// #ifdef INTEL_HEXL
#include <hexl/hexl.hpp>
// #endif

namespace kspir {
namespace test {

class NTTTest : public ::testing::Test {
protected:
  void SetUp() override { spdlog::set_level(spdlog::level::debug); }
};

class HEXLTest : public ::testing::Test {
protected:
  void SetUp() override {
    // Any setup code would go here
  }

  void TearDown() override {
    // Any cleanup code would go here
  }

  // Helper functions
  int64_t barret_reduce_hexl(int128_t a) {
    int64_t t;
    const int64_t v = 4611686044196143104;
    t = (__int128_t)v * a >> 104;
    t *= bigMod;
    return a - t;
  }

  uint64_t bit_inverse(uint64_t a, int loglen) {
    uint64_t result = 0;
    for (size_t i = 0; i < loglen; i++) {
      uint64_t temp = (a >> i) & 0x01;
      result |= temp << (loglen - i - 1);
    }
    return result;
  }

  uint64_t powd(uint64_t x, uint64_t y, uint64_t modulus) {
    uint64_t s = 1;
    for (size_t i = 0; i < y; i++) {
      s = (__int128_t)s * x % modulus;
    }
    return s;
  }

  inline int16_t barret_reduce_7681(int16_t a) {
    int16_t t;
    const int16_t v = 17474;
    t = (int32_t)v * a >> 27;
    t *= 7681;
    return a - t;
  }

  inline int16_t barret_reduce_7681_mult(int32_t a) {
    int16_t t;
    const int32_t v = 1145175501;
    t = (int64_t)v * a >> 43;
    t *= 7681;
    return a - t;
  }

  inline uint16_t special_reduce_7681(int16_t a) {
    uint16_t t = a >> 13;
    uint16_t u = a & 0x1fff;
    u -= t;
    t <<= 9;
    return u + t;
  }
};

TEST_F(HEXLTest, SmallNTT) {
  std::cout << "Testing small NTT with q = 8380417, N = 2048..." << std::endl;
  int32_t a[dim] = {1, 6};
  int32_t s[dim] = {1, 1, 1};
  int32_t result[dim];

  ntt(a);
  ntt(s);
  hadamard_mult(result, a, s);
  invntt_tomont(result);
  SPDLOG_INFO("result: {}", fmt::join(&result[0], &result[0]+20, ", "));

  std::cout << "Small NTT test completed successfully" << std::endl;
}

TEST_F(HEXLTest, BigNTT) {
  std::cout << "Testing big NTT with q = 4398046486529, N = 2048..."
            << std::endl;
  int64_t a[dim] = {1, 6};
  int64_t s[dim] = {1, 1, 1};
  int64_t result[dim];

  ntt(a);
  ntt(s);
  hadamard_mult(result, a, s);
  invntt_tomont(result);
  SPDLOG_INFO("result: {}", fmt::join(&result[0], &result[0]+20, ", "));

  std::cout << "Big NTT test completed successfully" << std::endl;
}

TEST_F(HEXLTest, BarretReduce) {
  std::cout << "Testing Barrett reduction..." << std::endl;
  int64_t t = 4398046486531 + 4398046486531;
  uint64_t result = barret_reduce(t);
  std::cout << "Barrett reduce result: " << result << std::endl;
}

TEST_F(HEXLTest, OmegaCalculation) {
  std::cout << "Testing omega calculations..." << std::endl;
  uint64_t omega = 384399401;
  uint64_t modulus = 4398046486529;

  std::cout << "Powers of omega mod " << modulus << ":" << std::endl;
  for (size_t i = 1; i < 10; i += 2) {
    std::cout << powd(omega, i, modulus) << ", ";
  }
  std::cout << "..." << std::endl;

  std::cout << "dim = " << dim << std::endl;
  std::cout << "omega^(2*dim) = " << powd(omega, 2 * dim, modulus) << std::endl;
}

TEST_F(HEXLTest, Reduce7681Performance) {
  std::cout << "Testing reduction methods performance with modulus 7681..."
            << std::endl;
  uint64_t length = 0x01 << 20; // Reduced size for quicker test
  uint64_t modulus = 7681;

  std::vector<uint16_t> input1(length);
  std::vector<uint16_t> input2(length);
  std::vector<uint16_t> output(length);

  for (size_t i = 0; i < length; i++) {
    input1[i] = rand() % modulus;
    input2[i] = rand() % modulus;
  }

  // Special reduce
  auto start = std::chrono::high_resolution_clock::now();
  for (size_t i = 0; i < length; i++) {
    output[i] = special_reduce_7681(input1[i] * input2[i]);
  }
  auto stop = std::chrono::high_resolution_clock::now();
  auto elapsed =
      std::chrono::duration_cast<std::chrono::microseconds>(stop - start);
  std::cout << "Special reduce time: " << elapsed.count() << " us" << std::endl;

  // Barrett reduce
  start = std::chrono::high_resolution_clock::now();
  for (size_t i = 0; i < length; i++) {
    output[i] = barret_reduce_7681_mult((int32_t)input1[i] * input2[i]);
  }
  stop = std::chrono::high_resolution_clock::now();
  elapsed = std::chrono::duration_cast<std::chrono::microseconds>(stop - start);
  std::cout << "Barrett reduce time: " << elapsed.count() << " us" << std::endl;

  // Native reduce
  start = std::chrono::high_resolution_clock::now();
  for (size_t i = 0; i < length; i++) {
    output[i] = ((int32_t)input1[i] * input2[i]) % 7681;
  }
  stop = std::chrono::high_resolution_clock::now();
  elapsed = std::chrono::duration_cast<std::chrono::microseconds>(stop - start);
  std::cout << "Native reduce time: " << elapsed.count() << " us" << std::endl;
}

TEST_F(HEXLTest, BitInverse) {
  std::cout << "Testing bit inverse calculations..." << std::endl;
  int loglen = 4; // Small size for demonstration
  int64_t len = 0x01 << loglen;

  std::cout << "Bit reverse values for loglen=" << loglen << ": ";
  for (size_t i = 0; i < len; i++) {
    std::cout << bit_inverse(i, loglen) << ", ";
  }
  std::cout << std::endl;
}

void test_intel_hexl() {
  int32_t length = N;
  uint64_t modulus = bigMod;

  /***********************     test permutation      *************************/

  int32_t rotate_num = 2;

  std::vector<uint64_t> input(length, 0);
  std::vector<uint64_t> input_ntt(length, 0);
  std::vector<uint64_t> automorphiced(length, 0);
  std::vector<uint64_t> automorphiced_ntt(length, 0);

#ifdef INTEL_HEXL
  // intel::hexl::NTT ntts(length, modulus);
  intel::hexl::NTT ntts(length, modulus);
  sample_random8_vector(input.data(), length);
  showLargeVector(input, "input = ");

  // input_ntt
  ntts.ComputeForward(input_ntt.data(), input.data(), 1, 1);
  automorphic(automorphiced, input, pow_mod(5, rotate_num, 2 * length),
              modulus); // rotate 2
  ntts.ComputeForward(automorphiced_ntt.data(), automorphiced.data(), 1, 1);

  SPDLOG_INFO("input ntt = {}",
              fmt::join(&input_ntt[0], &input_ntt[0]+20, ", "));
  SPDLOG_INFO("automorphic ntt = {}",
              fmt::join(&automorphiced_ntt[0], &automorphiced_ntt[0]+20, ", "));

  {
    int test_n = 8;
    std::vector<int32_t> hexl_ntt_index(test_n, 0);
    std::vector<int32_t> rotate_index(test_n, 0);
    compute_hexl_rotate_indexes(hexl_ntt_index, rotate_index, test_n);
    SPDLOG_INFO("hexl_ntt_index = {}", fmt::join(hexl_ntt_index, ", "));
    SPDLOG_INFO("rotate_index = {}", fmt::join(rotate_index, ", "));
    std::vector<int32_t> find_index(test_n, 0);
    for (size_t i = 0; i < test_n; i++) {
      find_index[hexl_ntt_index[i] >> 0x01] = i;
    }
    SPDLOG_INFO("find_index: {}", fmt::join(find_index, ","));
    std::vector<int32_t> permutation(test_n, 0);
    compute_permutation(permutation, 1, test_n);
    SPDLOG_INFO("permutation = {}", fmt::join(permutation, ", "));
    vector<uint32_t> index(test_n);
    for (size_t i = 0; i < test_n; i++) {
      index[permutation[i]] = i;
    }
    SPDLOG_INFO("index = {}", fmt::join(index, ", "));
  }

  // automorphic elments has same ntt numbers

  /***********
      std::vector<uint64_t> input2(length, 0);
      std::vector<uint64_t> input2_ntt(length, 0);
      input2[1] = 1;
      ntts.ComputeForward(input2_ntt.data(),
  input2.data(), 1, 1);

      std::cout << "RootOfUnity: " <<
  ntts.GetRootOfUnityPower(0) << std::endl; std::cout <<
  "MiniRootOfUnity: " << ntts.GetMinimalRootOfUnity() <<
  std::endl;
      // for (size_t i = 0; i < length; i++)
      for (size_t i = length - 20; i < length; i ++)
      {
          for (size_t j = 1; j < 2 * N; j += 2)
          {
              if (pow_mod(ntts.GetMinimalRootOfUnity(), j,
  modulus) == input2_ntt[i])
              {
                  std::cout << j << ", " << std::endl;
                  break;
              }
          }
      }
  *************/
  std::vector<int32_t> permutation(length, 0);
  compute_permutation(permutation, rotate_num, length);

  std::vector<uint64_t> result1(length, 0);
  for (size_t i = 0; i < length; i++) {
    result1[i] = input_ntt[permutation[i]];
  }

  showLargeVector(automorphiced_ntt, "result  = ");
  showLargeVector(result1, "result1 = ");
  SPDLOG_INFO(" permutation finished.");

  /***********************  test permutation matrix
   * *************************/
  int32_t max_index = 64;

  auto start = std::chrono::high_resolution_clock::now();
  std::vector<std::vector<int32_t>> permutations(
      max_index, std::vector<int32_t>(length, 0));
  compute_permutation_matrix(permutations, max_index, length);
  auto stop = std::chrono::high_resolution_clock::now();

  auto glapsed =
      std::chrono::duration_cast<std::chrono::microseconds>(stop - start);
  SPDLOG_INFO("generating permutation matrix costs {} us.", glapsed.count());

  std::vector<uint64_t> result2(length, 0);
  for (size_t i = 0; i < length; i++) {
    result2[i] = input_ntt[permutations[rotate_num][i]];
  }

  showLargeVector(result2, "result2 = ");
  SPDLOG_INFO("permutation matrix finished.");

  /***********************  test encode
   * *************************/
  /**
  std::vector<uint64_t> input_single(length, 0);
  std::vector<uint64_t> automorphiced_single(length, 0);
  std::vector<uint64_t> result3(length, 0);

  int32_t encode_index = 3; // rand() & (N / 2 - 1);
  input_single[encode_index] = 111111;
  input_single[N - 1 - encode_index] = 111111;

  ntts.ComputeInverse(input_single.data(),
  input_single.data(), 1, 1); for (size_t i = 1; i < N /
  2; i++)
  {
      automorphic(automorphiced_single, input_single,
  pow_mod(5, i, 2 * length), modulus); // rotate 2
  ntts.ComputeForward(automorphiced_single.data(),
  automorphiced_single.data(), 1, 1);
  intel::hexl::EltwiseAddMod(result3.data(),
  result3.data(), automorphiced_single.data(), length,
  modulus);
  }

  std::cout << " test encode finished." << std::endl;
  **/
#endif
}

TEST_F(HEXLTest, TestHexlNTT) { test_intel_hexl(); }

TEST_F(HEXLTest, TestNTTMultiply) {
  // 设置参数
  const uint32_t length = 8;
  const uint64_t modulus = bigMod;

  // 创建NTT实例
  intel::hexl::NTT ntt(length, modulus);

  // 准备输入数据
  std::vector<uint64_t> input1(length, 1);  // 全3向量
  std::vector<uint64_t> input2(length, 2);  // 全2向量
  std::vector<uint64_t> ntt1(length);
  std::vector<uint64_t> ntt2(length);
  std::vector<uint64_t> product(length);
  std::vector<uint64_t> result(length);

  // 执行NTT
  ntt.ComputeForward(ntt1.data(), input1.data(), 1, 1);
  ntt.ComputeForward(ntt2.data(), input2.data(), 1, 1);

  // 在NTT域做点乘
  intel::hexl::EltwiseMultMod(product.data(), ntt1.data(), ntt2.data(), length, modulus, 1);

  // 执行INTT
  ntt.ComputeInverse(result.data(), product.data(), 1, 1);

  SPDLOG_INFO("result = {}", fmt::join(result, ", "));

  SPDLOG_INFO("NTT multiply test passed");
}


} // namespace test
} // namespace kspir

int main(int argc, char **argv) {
  // spdlog::set_level(spdlog::level::debug);
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
