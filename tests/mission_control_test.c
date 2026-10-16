#include <assert.h>

#include "../src/mission_control.h"

bool g_mission_control_active = false;

struct existing_windows {
  uint32_t wids[4];
  size_t count;
};

static bool window_exists(uint32_t wid, void* context) {
  struct existing_windows* existing = context;
  for (size_t i = 0; i < existing->count; ++i) {
    if (existing->wids[i] == wid) return true;
  }
  return false;
}

int main(void) {
  struct mission_control_state state = { 0 };
  struct existing_windows existing = { .wids = { 10, 11 }, .count = 2 };

  // Nothing tracked: pruning never reports an exit.
  assert(mission_control_prune(&state, window_exists, &existing)
         == MISSION_CONTROL_TRANSITION_NONE);

  // The first backdrop enters, further displays and duplicates do not.
  assert(!mission_control_add_backdrop(&state, 0));
  assert(mission_control_add_backdrop(&state, 10)
         == MISSION_CONTROL_TRANSITION_ENTER);
  assert(mission_control_add_backdrop(&state, 11)
         == MISSION_CONTROL_TRANSITION_NONE);
  assert(mission_control_add_backdrop(&state, 10)
         == MISSION_CONTROL_TRANSITION_NONE);
  assert(state.count == 2);
  assert(mission_control_tracks(&state, 11));
  assert(!mission_control_tracks(&state, 12));

  // A destroy event for a backdrop that still exists (it moved between
  // Spaces) keeps Mission Control active.
  assert(mission_control_prune(&state, window_exists, &existing)
         == MISSION_CONTROL_TRANSITION_NONE);
  assert(state.count == 2);

  // One display's backdrop goes away first.
  existing.count = 1;
  assert(mission_control_prune(&state, window_exists, &existing)
         == MISSION_CONTROL_TRANSITION_NONE);
  assert(state.count == 1);
  assert(mission_control_tracks(&state, 10));

  // The last backdrop exits exactly once.
  existing.count = 0;
  assert(mission_control_prune(&state, window_exists, &existing)
         == MISSION_CONTROL_TRANSITION_EXIT);
  assert(mission_control_prune(&state, window_exists, &existing)
         == MISSION_CONTROL_TRANSITION_NONE);

  // Re-entering after an exit works, and capacity is bounded.
  for (uint32_t wid = 1; wid <= MISSION_CONTROL_MAX_BACKDROPS; ++wid) {
    assert(mission_control_add_backdrop(&state, wid)
           == (wid == 1 ? MISSION_CONTROL_TRANSITION_ENTER
                        : MISSION_CONTROL_TRANSITION_NONE));
  }
  assert(mission_control_add_backdrop(&state, 100)
         == MISSION_CONTROL_TRANSITION_NONE);
  assert(state.count == MISSION_CONTROL_MAX_BACKDROPS);

  return 0;
}
