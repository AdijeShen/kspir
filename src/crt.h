#pragma once

#include <stddef.h>
#include <stdint.h>
#include <vector>

#include "params.h"

/**
 * we use another ntt and multiplication implementation that is same as Spiral.
 *
 */

namespace kspir {
// CRT模数
constexpr uint64_t crtq1 = 268369921; // 2^28 - 2^16 + 1
constexpr uint64_t crtq2 = 249561089; // 2^28 - 2^21 - 2^12 + 1
constexpr uint64_t crtMod = crtq1 * crtq2; //可以将crtMod输入NTT，关闭HEXL_DEBUG就行

// CRT系数
constexpr uint64_t cr1_p = 68736257792;
constexpr uint64_t cr1_b = 73916747789;

// 打包参数
constexpr size_t max_summed_pa_or_b_in_u64 = 1 << 6;
constexpr uint64_t packed_offset_1 = 32;
constexpr uint64_t packed_offset_diff = 0; // log (a_i)
constexpr uint64_t packed_offset_2 = packed_offset_1 + packed_offset_diff;

// mini root of unity for intel::hexl::NTT
constexpr uint64_t mini_root_of_unity = 10297991595;

// bs模数相关参数
constexpr uint64_t bsMod = 16760833;             // 2^24 - 2^14 + 1
constexpr uint64_t bsModInv = 51309461009346113; // bsMod^-1 (mod crtMod)
constexpr uint64_t crtModInv = 3920321;          // crtMod^-1 (mod bsMod)

// 辅助模数
constexpr uint64_t auxMod = 268361729; // 28 bits

// 基于auxMod的参数
constexpr uint64_t crtBaMod = bsMod * auxMod;       // bsMod * auxMod
constexpr uint64_t crtBaModInv = 26456327859403970; // crtBaMod^-1 (mod crtMod)
constexpr uint64_t crtModInvBa = 2721180491014976;  // crtMod^-1 (mod crtBaMod)
constexpr uint64_t auxModInv = 48735363086513670;   // auxMod^-1 (mod crtMod)
constexpr uint64_t auxModInvBs = 4280662;           // auxMod^-1 (mod bsMod)

// 2^128模数相关参数
constexpr uint64_t two128_mod_hignbits_bsaux = 14764224294473;
constexpr uint64_t two128_mod_lowbits_bsaux = 1483995998376346528;
constexpr uint64_t crtModInvAux = 73083388;

// 2^128模数相关参数（aux）
constexpr uint64_t two128_mod_hignbits_aux = 1379708;
constexpr uint64_t two128_mod_lowbits_aux = 2086958912630458341;

// 2^128模数相关参数（bs）
constexpr uint64_t two128_mod_hignbits_bs = 54824; // 2^128 (mod q1 * q2) >> 63
constexpr uint64_t two128_mod_lowbits_bs =
    1764040975123512121; // 2^128 (mod q1 * q2) & (2^63 - 1)

// 多项式长度和单位根参数
constexpr size_t coeff_count_pow_of_2 =
    (CURRENT_PARAM_SET == ParamSet::SET_256)    ? 8
    : (CURRENT_PARAM_SET == ParamSet::SET_2048) ? 11
                                                : 12;
constexpr size_t poly_len = (CURRENT_PARAM_SET == ParamSet::SET_256)
                                ? 256
                                : (0x01 << coeff_count_pow_of_2);

constexpr uint64_t root_of_unity_crt =
    (CURRENT_PARAM_SET == ParamSet::SET_256)    ? 46801507955698810
    : (CURRENT_PARAM_SET == ParamSet::SET_2048) ? 38878761190133527
                                                : 3375402822066082;

constexpr uint64_t root_of_unity_crt_bamod =
    (CURRENT_PARAM_SET == ParamSet::SET_4096) ? 1486445687605966 : 0;

/**
 * @brief computeForward computes ntt form
 * @param result the length is 2 * N
 * @param input the length is N
 *
 * 计算前向NTT（数字理论变换）
 * @param result 结果数组，长度为2 * N
 * @param input 输入数组，长度为N
 */
void computeForward(uint64_t *result, uint64_t *input);

/**
 * @brief computeInverse computes ntt form
 * @param result the length is N
 * @param input the length is 2 * N
 *
 * 计算逆向NTT变换
 * @param result 结果数组，长度为N
 * @param input 输入数组，长度为2 * N
 */
void computeInverse(uint64_t *result, uint64_t *input);

/**
 * 巴雷特约简系数计算
 * 用于模运算优化
 */
inline uint64_t barrett_coeff(uint64_t val, size_t n);

/**
 * 中国剩余定理合成函数
 * 合并模crtq1和模crtq2下的两个余数x和y
 */
uint64_t crt_compose(uint64_t x, uint64_t y);

/**
 * 快速乘法：查询向量乘以数据库（第一维）
 * 实现数据库查询的第一维度高效乘法
 */
void fastMultiplyQueryByDatabaseDim1(std::vector<std::vector<uint64_t>> &out,
                                     const uint64_t *db,
                                     const uint64_t *v_firstdim, size_t dim0,
                                     size_t num_per);

/**
 * 快速乘法：查询向量乘以数据库（第一维），带逆CRT变换
 * 实现数据库查询的第一维度高效乘法并应用逆CRT变换
 */
void fastMultiplyQueryByDatabaseDim1InvCRT(
    std::vector<std::vector<uint64_t>> &out, const uint64_t *db,
    const uint64_t *v_firstdim, size_t dim0, size_t num_per);

/**
 * 将数据库转换为CRT（中国剩余定理）形式
 * 预处理数据库以便于同态加密计算
 */
void database_tocrt(uint64_t *datacrt,
                    std::vector<std::vector<uint64_t>> &data_ntt, int32_t N1);

} // namespace kspir
