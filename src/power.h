#ifndef VAM_POWER_H
#define VAM_POWER_H

/* Optional foreground dim/off timers; Never by default. Home belongs to the shell. */

#include <SDL.h>
#include <stdbool.h>

#include "platform/platform.h"

/* Applies the combo from prefs. */
void power_init(void);
/* Re-apply after the combo in prefs changed. */
void power_apply_prefs(void);

/* Every SDL event goes through here first. Returns true when the event only
 * woke the screen and must not reach the UI. */
bool power_handle_event(const SDL_Event *e);
/* Consume lifecycle wake state; no idle dim/off timers. */
void power_frame(void);
void power_keep_awake(void);
screen_level power_screen(void);

/* Choices offered in Settings, in seconds (0 = never). */
extern const int POWER_DIM_CHOICES[];
extern const int POWER_OFF_CHOICES[];
extern const int POWER_CHOICE_COUNT;

#endif
