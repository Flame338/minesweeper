/*
 * pcg32 tests.
 */
#include "tests.h"
#include "../include/pcg32.h"

void test_pcg32_determinism(void) {
  pcg32_random a, b;
  pcg32_srandom_r(&a, 12345ull, 1);
  pcg32_srandom_r(&b, 12345ull, 1);
  for (int i = 0; i < 1000; i++) {
    u32 x = pcg32_random_r(&a);
    u32 y = pcg32_random_r(&b);
    CHECK(x == y);
  }
}

void test_pcg32_differs_across_seeds(void) {
  pcg32_random a, b;
  pcg32_srandom_r(&a, 1, 1);
  pcg32_srandom_r(&b, 2, 1);
  /* Not guaranteed, but astronomically unlikely for seeds 1 and 2. */
  CHECK(pcg32_random_r(&a) != pcg32_random_r(&b));
}

void test_pcg32_bounded_stays_in_range(void) {
  pcg32_random rng;
  pcg32_srandom_r(&rng, 99, 1);
  int hits[10] = {0};
  for (int i = 0; i < 10000; i++) {
    u32 v = pcg32_boundedrand_r(&rng, 10);
    CHECK(v < 10);
    hits[v]++;
  }
  /* Every bucket should have been hit (no severe bias). */
  for (int i = 0; i < 10; i++) {
    CHECK(hits[i] > 0);
  }
}
