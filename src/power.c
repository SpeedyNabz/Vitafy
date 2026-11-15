/* Optional foreground idle timers. Both default to Never. */
#include "power.h"
#include "prefs.h"
#include "ui/ui.h"
#include <stdlib.h>
static screen_level level=SCREEN_ON;
static Uint64 last_input;
static void set_level(screen_level next){if(level!=next){level=next;platform_set_screen(next);}}
void power_apply_prefs(void){ui_set_combo(BTN_COUNT,BTN_COUNT);last_input=SDL_GetTicks64();set_level(SCREEN_ON);}
void power_init(void){level=SCREEN_ON;power_apply_prefs();}
screen_level power_screen(void){return level;}
bool power_handle_event(const SDL_Event *e)
{
    bool button=e->type==SDL_KEYDOWN||e->type==SDL_CONTROLLERBUTTONDOWN||e->type==SDL_JOYBUTTONDOWN;
    switch(e->type){
    case SDL_KEYDOWN:case SDL_CONTROLLERBUTTONDOWN:case SDL_JOYBUTTONDOWN:
    case SDL_FINGERDOWN:case SDL_FINGERMOTION:case SDL_FINGERUP:
    case SDL_MOUSEBUTTONDOWN:case SDL_MOUSEBUTTONUP:case SDL_MOUSEMOTION:case SDL_MOUSEWHEEL:break;
    case SDL_JOYAXISMOTION:if(abs(e->jaxis.value)<8000)return level==SCREEN_OFF;break;
    case SDL_CONTROLLERAXISMOTION:if(abs(e->caxis.value)<8000)return level==SCREEN_OFF;break;
    default:return false;
    }
    if(level==SCREEN_OFF&&!button)return true;
    bool wake=level==SCREEN_OFF;last_input=SDL_GetTicks64();set_level(SCREEN_ON);return wake;
}
void power_frame(void)
{
    Uint64 now=SDL_GetTicks64();
    if(platform_take_wake()){level=SCREEN_ON;last_input=now;}
    const prefs *p=prefs_get();Uint64 idle=now-last_input;
    if(p->off_after>0&&idle>=(Uint64)p->off_after*1000)set_level(SCREEN_OFF);
    else if(p->dim_after>0&&idle>=(Uint64)p->dim_after*1000&&level==SCREEN_ON)set_level(SCREEN_DIM);
}

void power_keep_awake(void){last_input=SDL_GetTicks64();set_level(SCREEN_ON);}
