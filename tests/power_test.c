/* Regression: legacy idle timers and screen-off combo cannot darken the app. */
#include "power.h"
#include "prefs.h"
#include "ui/ui.h"
#include <stdio.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"power check failed: %d\n",__LINE__);return 1;}}while(0)
static prefs legacy;
static Uint64 elapsed;
static int dark_requests;
static btn combo_a,combo_b;
Uint64 SDL_GetTicks64(void){return elapsed;}
prefs *prefs_get(void){return &legacy;}
void ui_set_combo(btn a,btn b){combo_a=a;combo_b=b;}
bool ui_combo_fired(void){return true;}
bool ui_capturing_combo(void){return false;}
void ui_capture_combo(bool on){(void)on;}
bool platform_take_wake(void){return false;}
void platform_set_screen(screen_level level){if(level!=SCREEN_ON)dark_requests++;}
int main(void)
{
    power_init();CHECK(combo_a==BTN_COUNT&&combo_b==BTN_COUNT);
    for(elapsed=0;elapsed<=3600000;elapsed+=1000){power_frame();CHECK(power_screen()==SCREEN_ON);}
    CHECK(!dark_requests);
    legacy.dim_after=15;legacy.off_after=30;power_apply_prefs();
    elapsed+=15000;power_frame();CHECK(power_screen()==SCREEN_DIM);
    SDL_Event event={0};event.type=SDL_JOYAXISMOTION;event.jaxis.value=1;
    CHECK(!power_handle_event(&event));CHECK(power_screen()==SCREEN_DIM);
    elapsed+=15000;power_frame();CHECK(power_screen()==SCREEN_OFF);
    event.type=SDL_FINGERDOWN;CHECK(power_handle_event(&event));CHECK(power_screen()==SCREEN_OFF);
    event.type=SDL_KEYDOWN;CHECK(power_handle_event(&event));CHECK(power_screen()==SCREEN_ON);
    elapsed+=14000;power_keep_awake();elapsed+=2000;power_frame();CHECK(power_screen()==SCREEN_ON);
    puts("power: Never default, optional dim/off, drift rejection and consumed wake input passed");return 0;
}
