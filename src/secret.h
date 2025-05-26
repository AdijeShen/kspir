#pragma once

#include <stdint.h>
#include <vector>

#include "params.h"

#ifdef INTEL_HEXL
#include <hexl/hexl.hpp>
#endif

namespace kspir {
/**
 * LWE密文类型枚举
 * LWE: 学习误差问题密文
 * RLWE: 环上学习误差问题密文
 */
enum LWE_TYPE { LWE, RLWE };

/**
 * 密钥类
 * 用于加密和解密操作的密钥
 */
class Secret {
private:
  /* data */
  LWE_TYPE lwe_type = RLWE;

  uint64_t length = N;
  uint64_t modulus;

  std::vector<uint64_t> data;

  bool nttform = true;

#ifdef INTEL_HEXL
  intel::hexl::NTT ntts;
#endif

public:
  /**
   * 构造RLWE类型的密钥
   * @param module 模数
   * @param ntt 是否以NTT形式存储
   */
  Secret(uint64_t module = bigMod, bool ntt = true);

  /**
   * 构造指定类型的密钥
   * @param type 密钥类型(LWE或RLWE)
   * @param module 模数
   */
  Secret(LWE_TYPE type, uint64_t module = mod); // LWE construct

  /**
   * 获取密钥数据
   * @return 密钥向量
   */
  std::vector<uint64_t> getData();

  /**
   * 获取密钥数据中特定索引的值
   * @param index 索引
   * @return 对应位置的值
   */
  uint64_t getData(int32_t index);

  /**
   * 获取密钥使用的模数
   * @return 模数
   */
  uint64_t getModulus();

  /**
   * 获取密钥长度
   * @return 长度
   */
  int32_t getLength();

  /**
   * 获取密钥类型
   * @return LWE或RLWE类型
   */
  LWE_TYPE getLweType();

  /**
   * 检查密钥是否以NTT形式存储
   * @return 是否为NTT形式
   */
  bool isNttForm();

#ifdef INTEL_HEXL
  /**
   * 获取NTT实例
   * @return NTT实例
   */
  intel::hexl::NTT getNTT();
#endif

  /**
   * 将密钥转换为系数形式
   */
  void toCoeffForm();

  /**
   * 将密钥转换为NTT形式
   */
  void toNttForm();

  ~Secret();
};

} // namespace kspir
