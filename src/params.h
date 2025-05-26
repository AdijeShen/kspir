#pragma once

#include <stdint.h>

#define INTEL_HEXL
#define KSPIR_USE_CRTMOD
#define KSPIR_THREADS

namespace kspir {
enum class ParamSet {
    SET_256,    // N = 256
    SET_2048,   // N = 2048
    SET_4096    // N = 4096
};

// 选择参数集
constexpr ParamSet CURRENT_PARAM_SET = ParamSet::SET_4096;

// 基础参数
constexpr uint32_t N = (CURRENT_PARAM_SET == ParamSet::SET_256) ? 256 :
                      (CURRENT_PARAM_SET == ParamSet::SET_2048) ? 2048 : 4096;

#ifdef KSPIR_THREADS
constexpr uint32_t THREADS_NUM = 16;
#endif

// 模数相关参数
constexpr uint64_t mod = 8380417;  // (UINT64_C(0x01) << 32), omega = 1753
constexpr uint64_t bigMod = 1125899906826241;  // pow(2, 50)
constexpr uint64_t bigMod2 = 4398046504961;

// 模数逆元
constexpr uint64_t mod2inv = 569391470430536;  // q_2^-1 (mod q_1)
constexpr uint64_t mod1inv = 2173861076666;    // q_1^-1 (mod q_2)

// Delta和Bg参数
constexpr uint64_t Delta = 17179869183;  // floor(q/p)
constexpr uint64_t Bg = 0x01 << 17;

// 2^128模数相关参数
constexpr uint64_t two128_mod_hignbits = 528449607;  // 2^128 (mod q1 * q2) >> 63
constexpr uint64_t two128_mod_lowbits = 9116411467323735968;

// bNinv参数
constexpr uint64_t bNinv = (CURRENT_PARAM_SET == ParamSet::SET_256) ? 1121501860315201 :
                          (CURRENT_PARAM_SET == ParamSet::SET_2048) ? 1125350151012361 :
                          1125625028919301;

// bNinv2参数
constexpr uint64_t bNinv2 = (CURRENT_PARAM_SET == ParamSet::SET_256) ? 4380866635801 :
                           (CURRENT_PARAM_SET == ParamSet::SET_2048) ? 4395899021316 :
                           2197949510658;

// 其他参数
constexpr uint32_t Pbits = 16;
constexpr uint32_t ell = 3;
constexpr uint32_t Base = 0;

// bsgs参数
constexpr uint64_t bsgsp = (CURRENT_PARAM_SET == ParamSet::SET_256) ? 7681 :
                          (CURRENT_PARAM_SET == ParamSet::SET_2048) ? 40961 :
                          65537;

#ifdef KSPIR_USE_CRTMOD
constexpr uint64_t bsgsDelta = (CURRENT_PARAM_SET == ParamSet::SET_256) ? 
                              8719527371384 :
                              (CURRENT_PARAM_SET == ParamSet::SET_2048) ?
                              1635084342169 :
                              1021937069741;
#else
constexpr uint64_t bsgsDelta = (CURRENT_PARAM_SET == ParamSet::SET_256) ? 
                              146582464109 :
                              (CURRENT_PARAM_SET == ParamSet::SET_2048) ?
                              27487119621 :
                              27487119621;
#endif


} // namespace kspir
