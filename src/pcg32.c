#include "../include/pcg32.h"

u32 pcg32_random_r(pcg32_random *rng) {
  u64 oldstate = rng->state;
  rng->state = oldstate * 6364136223846793005ULL + (rng->inc | 1);
  u32 xorshifted = (u32)(((oldstate >> 18u) ^ oldstate) >> 27u);
  u32 rot = (u32)(oldstate >> 59u);
  return (xorshifted >> rot) | (xorshifted << ((-rot) & 31));
}

void pcg32_srandom_r(pcg32_random *rng, u64 initState, u64 initSeq) {
  rng->state = 0U;
  rng->inc = (initSeq << 1u) | 1u;
  pcg32_random_r(rng);
  rng->state += initState;
  pcg32_random_r(rng);
}

u32 pcg32_boundedrand_r(pcg32_random *rng, u32 bound) {
  u32 threshold = (u32)(-(i32)bound) % bound;
  for (;;) {
    u32 r = pcg32_random_r(rng);
    if (r >= threshold)
      return r % bound;
  }
}
