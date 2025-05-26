#pragma once

#include <stddef.h>
#include <stdint.h>
#include <vector>

namespace kspir {
/**
 * 生成随机数组
 * 在[0, modulus)范围内均匀采样随机值
 */
void sample_random(uint64_t *a, uint64_t modulus, int32_t size);

/**
 * 生成随机向量
 * 在[0, modulus)范围内均匀采样随机值填充向量
 */
void sample_random(std::vector<uint64_t> &result, uint64_t modulus);

/**
 * 采样单个高斯分布噪声
 * 返回基于高斯分布的随机误差项
 */
uint64_t sample_guass(uint64_t modulus);

/**
 * @brief
 *
 * 采样高斯分布噪声数组
 * 用高斯分布随机值填充结果数组
 */
void sample_guass(uint64_t *result, uint64_t modulus);

/**
 * @brief evaluate automorphic transform
 *
 * @param result
 * @param modulus1 input modulus
 * @param modulus2 output modulus
 *
 * 将高斯噪声从一个模数转换到另一个模数
 * @param result 噪声数组
 * @param modulus1 输入模数
 * @param modulus2 输出模数
 */
void guass_to_modulus(uint64_t *result, uint64_t modulus1, uint64_t modulus2);

/**
 * 生成随机数据库（C风格数组版本）
 * 用随机值填充二维数据库
 */
void sample_database(uint64_t **data);

/**
 * 生成随机数据库（STL向量版本）
 * 用随机值填充二维数据库
 */
void sample_database(std::vector<std::vector<uint64_t>> &data);

/**
 * 生成8位随机向量
 * 每个元素是8位随机值
 */
void sample_random8_vector(uint64_t *a, size_t length);

} // namespace kspir
