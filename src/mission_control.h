#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// Since macOS 27, Mission Control is drawn by the WindowManager process and
// WindowServer no longer hides helper windows while it is shown. While it is
// open, WindowManager places one backdrop window at this level covering each
// display. Borders are suppressed for as long as any of those backdrops exist.
#define MISSION_CONTROL_OWNER "WindowManager"
#define MISSION_CONTROL_BACKDROP_LEVEL 19
#define MISSION_CONTROL_MAX_BACKDROPS 16

// Process-wide runtime suppression, checked next to border_style=none.
extern bool g_mission_control_active;

struct mission_control_state {
  uint32_t backdrops[MISSION_CONTROL_MAX_BACKDROPS];
  size_t count;
};

enum mission_control_transition {
  MISSION_CONTROL_TRANSITION_NONE = 0,
  MISSION_CONTROL_TRANSITION_ENTER,
  MISSION_CONTROL_TRANSITION_EXIT,
};

static inline bool mission_control_tracks(const struct mission_control_state* state,
                                          uint32_t wid) {
  for (size_t i = 0; i < state->count; ++i) {
    if (state->backdrops[i] == wid) return true;
  }
  return false;
}

// Records a confirmed backdrop. Returns ENTER for the first one.
static inline enum mission_control_transition mission_control_add_backdrop(
    struct mission_control_state* state,
    uint32_t wid) {
  if (!wid
      || mission_control_tracks(state, wid)
      || state->count >= MISSION_CONTROL_MAX_BACKDROPS) {
    return MISSION_CONTROL_TRANSITION_NONE;
  }
  state->backdrops[state->count++] = wid;
  return state->count == 1
         ? MISSION_CONTROL_TRANSITION_ENTER
         : MISSION_CONTROL_TRANSITION_NONE;
}

// Drops backdrops that no longer exist. WindowServer also sends a destroy
// event when a backdrop merely moves between Spaces, so existence is checked
// on the window itself. Returns EXIT once the last backdrop is gone.
static inline enum mission_control_transition mission_control_prune(
    struct mission_control_state* state,
    bool (*exists)(uint32_t wid, void* context),
    void* context) {
  if (!state->count) return MISSION_CONTROL_TRANSITION_NONE;

  size_t count = 0;
  for (size_t i = 0; i < state->count; ++i) {
    uint32_t wid = state->backdrops[i];
    if (exists(wid, context)) state->backdrops[count++] = wid;
  }
  state->count = count;
  return count
         ? MISSION_CONTROL_TRANSITION_NONE
         : MISSION_CONTROL_TRANSITION_EXIT;
}
