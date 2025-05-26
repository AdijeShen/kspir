#pragma once

#include <stdint.h>
#include <vector>

#include "lwe.h"
#include "secret.h"

namespace kspir {
/**
 * 生成PIR查询密文
 *
 * @param cipher 生成的查询密文
 * @param queryKey 查询密钥
 * @param row 要查询的行索引
 */
void query(RlweCiphertext &cipher, Secret &queryKey, uint64_t row);

} // namespace kspir
