#ifndef DEBUG
#define PCG32_H

#include <stdint.h>

/*
 * Why this instead of libc's rand()/srand():
 * rand() has no portability guarantee — its period, quality, and even its
 * output for a given seed can differ across platforms and standard
 * library versions. PCG32 is a fully specified algorithm: same seed, same
 * stream selector -> same output, everywhere, forever. That's what lets
 * the replay system store just a seed instead of a full board snapshot
 * (see replay.h).
 * */

typedef uint32_t u32;
typedef uint64_t u64;
typedef int32_t i32;

typedef struct {
  u64 state;
  u64 inc;
} pcg32_random;

// Seed the generator. 'initSeq' selects an independent output stream; pass a
// fixed constant (not per-run entropy) if you need reproducible sequences
void pcg32_srandom_r(pcg32_random *rng, u64 initState, u64 initSeq);

// Gnerate a uniformly distributed 32-bit unsigned integer
u32 pcg32_random_r(pcg32_random *rng);

// Generate an unbiased random integer in [0, bound). Avoids the modulo bias
// that plain `rand() % bound` has
u32 pcg32_boundedrand_r(pcg32_random *rng, u32 bound);

#endif // !DEBUG PCG32_H
