#include "query.h"

#include "encrypt.h"
#include "params.h"
#include "samples.h"

namespace kspir {
void query(RlweCiphertext &cipher, Secret &queryKey, uint64_t row) {
  uint64_t length = queryKey.getLength();

  // uint64_t* message = new uint64_t[length];
  std::vector<uint64_t> message(length);

  // TODO: check it
  if (row == 0)
    message[0] = Delta;
  else
    message[length - row] = bigMod - Delta;

    // store a in coefficient form
#ifdef INTEL_HEXL
  intel::hexl::NTT ntts = queryKey.getNTT();

  ntts.ComputeForward(message.data(), message.data(), 1, 1);
  encrypt(cipher.b.data(), cipher.a.data(), queryKey, message.data());

  ntts.ComputeInverse(cipher.a.data(), cipher.a.data(), 1, 1);
  cipher.setIsNtt(false); // note! b is ntt form, a is coefficient form.
#endif

  // delete[] message;
}
} // namespace kspir