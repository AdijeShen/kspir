#ifndef RECOVER_H
#define RECOVER_H

#include <stddef.h>
#include <stdint.h>
#include <vector>

#include "lwe.h"
#include "secret.h"

void recover(std::vector<uint64_t> &message, RlweCiphertext &cipher,
             Secret &new_secret);

#endif
