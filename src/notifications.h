#ifndef VITAFY_NOTIFICATIONS_H
#define VITAFY_NOTIFICATIONS_H
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "vam_backend.h"
void notifications_reset(void);
/* Coalesces skips, suppresses progress/pause events, never retries in a loop.
   True means a new song notification was attempted; UI also shows a toast. */
bool notifications_update(const vam_player_state *state, uint64_t now, bool enabled);
/* Bounded, replacement-safe UTF-8 decoding; always terminates when capacity > 0. */
size_t notifications_utf16(const char *src, uint16_t *dst, size_t capacity);
#endif
