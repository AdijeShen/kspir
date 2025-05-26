#pragma once

#include <stddef.h>
#include <stdint.h>
#include <vector>

#include "lwe.h"
#include "secret.h"

namespace kspir {
/**
 * 从密文中恢复原始消息
 * 
 * @param message 解密后的消息结果
 * @param cipher 要解密的RLWE密文
 * @param new_secret 用于解密的密钥
 */
void recover(std::vector<uint64_t> &message, RlweCiphertext &cipher,
             Secret &new_secret);

} // namespace kspir
