/*
 * Test runner for the minesweeper game logic (no raylib).
 *
 * Build with `make test`; exits non-zero if any test fails.
 */
#include "tests.h"
#include <stdio.h>

int g_tests_pass = 0;
int g_tests_fail = 0;

/* pcg32 */
extern void test_pcg32_determinism(void);
extern void test_pcg32_differs_across_seeds(void);
extern void test_pcg32_bounded_stays_in_range(void);

/* board */
extern void test_init_grid_is_blank(void);
extern void test_first_click_places_bombs_and_is_safe(void);
extern void test_same_seed_reproduces_layout(void);
extern void test_different_seed_differs(void);
extern void test_neighbour_counts(void);
extern void test_floodfill_expands_and_stops_at_numbers(void);
extern void test_reveal_mine_loses(void);
extern void test_flag_toggle(void);
extern void test_checkwin(void);
extern void test_reveal_seam_sets_won(void);
extern void test_reveal_seam_loses_does_not_win(void);
extern void test_can_reveal_hint_live_only(void);
extern void test_can_hint_ignores_budget(void);
extern void test_can_save_requires_live_log(void);

/* replay edge cases */
extern void test_save_replay_fails_before_first_click(void);
extern void test_load_missing_file_fails(void);
extern void test_newgame_resets_state_after_replay(void);
extern void test_save_refused_during_and_after_playback(void);

/* round trip */
extern void test_replay_roundtrip(void);
extern void test_replay_stops_at_hit_mine(void);

/* solver */
extern void test_rule1_zero_remaining_safe(void);
extern void test_rule2_unknown_is_all_mines(void);
extern void test_subset_rule_extra_cells_safe(void);
extern void test_hint_false_when_nothing_guaranteed(void);
extern void test_inconsistent_overflagged(void);
extern void test_hint_budget_starts_full(void);
extern void test_hint_no_info_consumes_nothing(void);
extern void test_hint_success_consumes_one(void);
extern void test_hint_same_cell_reshow_free(void);
extern void test_hint_inconsistent_consumes_nothing(void);
extern void test_hint_exhausted(void);
extern void test_hint_budget_reset_on_newgame(void);
extern void test_hint_clears_on_mutation(void);

typedef void (*TestFn)(void);
typedef struct {
  const char *name;
  TestFn fn;
} TestEntry;

int main(void) {
  static const TestEntry tests[] = {
      {"pcg32 determinism", test_pcg32_determinism},
      {"pcg32 differs across seeds", test_pcg32_differs_across_seeds},
      {"pcg32 bounded stays in range", test_pcg32_bounded_stays_in_range},
      {"init grid is blank", test_init_grid_is_blank},
      {"first click places bombs and is safe", test_first_click_places_bombs_and_is_safe},
      {"same seed reproduces layout", test_same_seed_reproduces_layout},
      {"different seed differs", test_different_seed_differs},
      {"neighbour counts", test_neighbour_counts},
      {"floodfill expands and stops at numbers",
       test_floodfill_expands_and_stops_at_numbers},
      {"reveal mine loses", test_reveal_mine_loses},
      {"flag toggle", test_flag_toggle},
      {"checkwin", test_checkwin},
      {"reveal seam sets won", test_reveal_seam_sets_won},
      {"reveal seam lose does not win", test_reveal_seam_loses_does_not_win},
      {"can reveal/hint live only", test_can_reveal_hint_live_only},
      {"can hint ignores budget", test_can_hint_ignores_budget},
      {"can save requires live log", test_can_save_requires_live_log},
      {"save replay fails before first click",
       test_save_replay_fails_before_first_click},
      {"load missing file fails", test_load_missing_file_fails},
      {"newgame resets state after replay", test_newgame_resets_state_after_replay},
      {"save refused during/after playback",
       test_save_refused_during_and_after_playback},
      {"replay round trip", test_replay_roundtrip},
      {"replay stops at hit mine", test_replay_stops_at_hit_mine},
      {"solver rule1 zero-remaining safe", test_rule1_zero_remaining_safe},
      {"solver rule2 unknown all mines", test_rule2_unknown_is_all_mines},
      {"solver subset rule extra cells safe", test_subset_rule_extra_cells_safe},
      {"solver hint false when nothing guaranteed",
       test_hint_false_when_nothing_guaranteed},
      {"solver inconsistent over-flagged", test_inconsistent_overflagged},
      {"hint budget starts full", test_hint_budget_starts_full},
      {"hint no-info costs nothing", test_hint_no_info_consumes_nothing},
      {"hint success costs one", test_hint_success_consumes_one},
      {"hint same-cell re-show is free", test_hint_same_cell_reshow_free},
      {"hint inconsistent costs nothing", test_hint_inconsistent_consumes_nothing},
      {"hint exhausted", test_hint_exhausted},
      {"hint budget resets on new game", test_hint_budget_reset_on_newgame},
      {"hint clears on mutation", test_hint_clears_on_mutation},
  };

  size_t n = sizeof(tests) / sizeof(tests[0]);
  int tests_passed = 0;
  int tests_failed = 0;
  for (size_t i = 0; i < n; i++) {
    int fail_before = g_tests_fail;
    printf("[ RUN ] %s\n", tests[i].name);
    tests[i].fn();
    if (g_tests_fail == fail_before) {
      tests_passed++;
      printf("[ PASS] %s\n", tests[i].name);
    } else {
      tests_failed++;
      printf("[ FAIL] %s\n", tests[i].name);
    }
  }

  /* g_tests_pass/g_tests_fail are assertion-level; the test count is the
   * meaningful "17 tests" figure. */
  printf("\n%zu tests: %d passed, %d failed (%d assertions: %d passed, %d failed)\n",
         n, tests_passed, tests_failed, g_tests_pass + g_tests_fail,
         g_tests_pass, g_tests_fail);
  return tests_failed ? 1 : 0;
}
