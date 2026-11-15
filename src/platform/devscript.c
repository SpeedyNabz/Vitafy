/*
 * Desktop-only UI automation for screenshots and smoke tests.
 *
 *   VAM_SCRIPT=script.txt ./VitaSpot
 *
 * One command per line:
 *   wait <ms>           let the app run
 *   key <name>          press+release a key (SDL key name: Down, Return, ...)
 *   keydown <name>      press and hold a key (for combos), until keyup <name>
 *   text <string>       type text (when a text field is active)
 *   click <x> <y>       left click in 960x544 logical coordinates
 *   drag <x0> <y0> <x1> <y1>
 *   touchdrag <x0> <y0> <x1> <y1>   same, as front-touch finger events (Vita path)
 *   shot <file.bmp>     save the next rendered frame
 *   quit
 */
#ifndef __vita__

#include "devscript.h"
#include "../app.h"
#include "../prefs.h"
#include "../power.h"
#include "platform.h"
#include "../gfx/gfx.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_LINES 512

static char *g_lines[MAX_LINES];
static int g_n, g_pc;
static Uint64 g_wait_until;
static char g_shot[512];
static bool g_active;

/* Pending pointer sequence, one step per frame so the UI sees each phase. */
static int g_ptr_step;
static float g_px0, g_py0, g_px1, g_py1;
static bool g_touch;

bool devscript_init(void)
{
    const char *path = SDL_getenv("VAM_SCRIPT");
    if (!path)
        return false;
    FILE *f = fopen(path, "r");
    if (!f) {
        SDL_Log("devscript: can't open %s", path);
        return false;
    }
    char line[512];
    while (g_n < MAX_LINES && fgets(line, sizeof(line), f)) {
        line[strcspn(line, "\r\n")] = 0;
        if (line[0] && line[0] != '#')
            g_lines[g_n++] = SDL_strdup(line);
    }
    fclose(f);
    g_active = true;
    return true;
}

static void push_mouse(Uint32 type, float x, float y)
{
    SDL_Event e;
    memset(&e, 0, sizeof(e));
    e.type = type;
    if (type == SDL_MOUSEMOTION) {
        e.motion.x = (int)x;
        e.motion.y = (int)y;
        e.motion.state = g_ptr_step > 0 ? SDL_BUTTON_LMASK : 0;
    } else {
        e.button.button = SDL_BUTTON_LEFT;
        e.button.state = type == SDL_MOUSEBUTTONDOWN ? SDL_PRESSED : SDL_RELEASED;
        e.button.x = (int)x;
        e.button.y = (int)y;
        e.button.clicks = 1;
    }
    SDL_PushEvent(&e);
}

/* Finger events in normalised coordinates, like SDL's Vita touch driver. */
static void push_finger(Uint32 type, float x, float y)
{
    SDL_Event e;
    memset(&e, 0, sizeof(e));
    e.type = type;
    e.tfinger.touchId = SDL_GetTouchDevice(0);
    e.tfinger.fingerId = 1;
    e.tfinger.x = x / 960.0f;
    e.tfinger.y = y / 544.0f;
    e.tfinger.pressure = 1.0f;
    SDL_PushEvent(&e);
}

static void push_pointer(int phase, float x, float y)
{
    static const Uint32 mouse[] = { SDL_MOUSEBUTTONDOWN, SDL_MOUSEMOTION, SDL_MOUSEBUTTONUP };
    static const Uint32 finger[] = { SDL_FINGERDOWN, SDL_FINGERMOTION, SDL_FINGERUP };
    if (g_touch)
        push_finger(finger[phase], x, y);
    else
        push_mouse(mouse[phase], x, y);
}

static void push_key(SDL_Keycode k, bool down)
{
    SDL_Event e;
    memset(&e, 0, sizeof(e));
    e.type = down ? SDL_KEYDOWN : SDL_KEYUP;
    e.key.state = down ? SDL_PRESSED : SDL_RELEASED;
    e.key.keysym.sym = k;
    e.key.keysym.scancode = SDL_GetScancodeFromKey(k);
    SDL_PushEvent(&e);
}

static SDL_Keycode g_key_up;

bool devscript_frame(bool *quit)
{
    if (!g_active)
        return false;
    Uint64 now = SDL_GetTicks64();

    if (g_key_up) {
        push_key(g_key_up, false);
        g_key_up = 0;
        return true;
    }
    /* Pointer sequences: down, move x4, up, spread over frames. */
    if (g_ptr_step > 0) {
        if (g_ptr_step == 1) {
            if (!g_touch)
                push_mouse(SDL_MOUSEMOTION, g_px0, g_py0);
            push_pointer(0, g_px0, g_py0);
        } else if (g_ptr_step <= 9) {
            float t = (g_ptr_step - 1) / 8.0f;
            push_pointer(1, g_px0 + (g_px1 - g_px0) * t, g_py0 + (g_py1 - g_py0) * t);
        } else {
            push_pointer(2, g_px1, g_py1);
            g_ptr_step = 0;
            return true;
        }
        g_ptr_step++;
        return true;
    }
    if (now < g_wait_until || g_shot[0])
        return true;
    if (g_pc >= g_n) {
        g_active = false;
        return false;
    }

    const char *l = g_lines[g_pc++];
    char arg[480];
    int a, b, c, d;
    if (sscanf(l, "wait %d", &a) == 1) {
        g_wait_until = now + (Uint64)a;
    } else if (sscanf(l, "keydown %479s", arg) == 1 || sscanf(l, "keyup %479s", arg) == 1) {
        push_key(SDL_GetKeyFromName(arg), l[3] == 'd');
    } else if (sscanf(l, "key %479s", arg) == 1) {
        SDL_Keycode k = SDL_GetKeyFromName(arg);
        if (k == SDLK_UNKNOWN)
            SDL_Log("devscript: unknown key %s", arg);
        push_key(k, true);
        g_key_up = k;
    } else if(sscanf(l,"wheel %d",&a)==1){
        SDL_Event e={0};e.type=SDL_MOUSEWHEEL;e.wheel.y=a;e.wheel.preciseY=(float)a;SDL_PushEvent(&e);
    } else if (strncmp(l, "text ", 5) == 0) {
        SDL_Event e;
        memset(&e, 0, sizeof(e));
        e.type = SDL_TEXTINPUT;
        snprintf(e.text.text, sizeof(e.text.text), "%s", l + 5);
        SDL_PushEvent(&e);
    } else if (sscanf(l, "touchdrag %d %d %d %d", &a, &b, &c, &d) == 4) {
        g_px0 = (float)a; g_py0 = (float)b;
        g_px1 = (float)c; g_py1 = (float)d;
        g_touch = true;
        g_ptr_step = 1;
    } else if (sscanf(l, "click %d %d", &a, &b) == 2) {
        g_touch = false;
        g_px0 = g_px1 = (float)a;
        g_py0 = g_py1 = (float)b;
        g_ptr_step = 1;
    } else if (sscanf(l, "drag %d %d %d %d", &a, &b, &c, &d) == 4) {
        g_touch = false;
        g_px0 = (float)a; g_py0 = (float)b;
        g_px1 = (float)c; g_py1 = (float)d;
        g_ptr_step = 1;
    } else if (sscanf(l,"open %479s",arg)==1) {
        app_open_uri(arg);
    } else if (sscanf(l,"mock_saved %d",&a)==1) {
#ifdef VAM_MOCK_TEST_CONTROLS
        extern void vam_mock_external_saved(const char *,int);
        vam_mock_external_saved(A.player.track.uri,a);
#endif
    } else if (sscanf(l, "state %479s", arg) == 1) {
        FILE *f = fopen(arg, "w");
        if (f) {
            fprintf(f, "tab=%d\ndepth=%d\nnp=%d\ntyping=%d\nplayback=%d\nuri=%s\nquery=%s\n",
                    A.tab, app_depth(), A.np_open, A.typing, A.player.playback, app_view()->uri, app_view()->query);
            fclose(f);
            f = fopen(arg, "a");
            if (f) {
                int row_saved=-1;
                model_page *page=app_view()->page;
                if(page) for(uint32_t si=0;si<page->section_count;si++) for(uint32_t j=0;j<page->sections[si].count;j++)
                    if(!strcmp(page->sections[si].items[j].uri,A.player.track.uri))row_saved=page->sections[si].items[j].is_saved;
                fprintf(f, "sort=%d\ndescending=%d\nplayer_saved=%d\nrow_saved=%d\nfilter=%s\ncompact=%d\nextent=%d\n",
                    app_view()->sort,app_view()->descending,A.player.track.is_saved,row_saved,app_view()->filter_query,app_view()->compact,A.extent[A.tab]);
                unsigned playlists=0; const char *last="";
                model_page *lib=A.stacks[TAB_LIBRARY][0].page;
                if(lib&&lib->section_count)for(uint32_t i=0;i<lib->sections[0].count;i++)if(lib->sections[0].items[i].kind==VAM_KIND_PLAYLIST){playlists++;last=lib->sections[0].items[i].title;}
                fprintf(f,"playlists=%u\nlast_playlist=%s\nedit_pending=%u\n",playlists,last,A.edit_request);
                fprintf(f,"quality=%d\ndim_after=%d\noff_after=%d\nscreen_level=%d\n",prefs_get()->bitrate,prefs_get()->dim_after,prefs_get()->off_after,power_screen());
                fprintf(f,"cache_pending=%u\nmetadata_mb=%d\norder_pending=%u\noriginal_fallback=%d\n",A.cache_request,prefs_get()->metadata_mb,A.order_token,app_view()->original_order_fallback);
                unsigned loaded=page&&page->section_count?page->sections[0].count:0;
                unsigned total=page&&page->section_count?page->sections[0].total:0,matches=0;
                if(lib&&lib->section_count)for(uint32_t i=0;i<lib->sections[0].count;i++)if(app_library_matches(&A.stacks[TAB_LIBRARY][0],&lib->sections[0].items[i]))matches++;
                fprintf(f,"grid=%d\nloaded=%u\ntotal=%u\nsort_open=%d\ncategory=%d\nby_you=%d\ndownloaded=%d\nlibrary_matches=%u\n",app_view()->grid,loaded,total,app_view()->sort_open,A.stacks[TAB_LIBRARY][0].filter,A.stacks[TAB_LIBRARY][0].by_you,0,matches);
                fclose(f);
            }
        }
    } else if (sscanf(l, "shot %479s", arg) == 1) {
        snprintf(g_shot, sizeof(g_shot), "%s", arg);
    } else if (strcmp(l, "background") == 0) {
        platform_test_overlay(true);
    } else if (strcmp(l, "foreground") == 0) {
        platform_test_overlay(false);
    } else if (sscanf(l, "capture %479s", arg) == 1) {
        /* Test-only readback, including while simulated graphics are gated. */
        snprintf(g_shot, sizeof(g_shot), "%s", arg);
        devscript_after_render(gfx_renderer());
    } else if (sscanf(l, "quitshot %479s", arg) == 1) {
        snprintf(g_shot, sizeof(g_shot), "%s", arg);
        SDL_Event event;
        memset(&event, 0, sizeof(event));
        event.type = SDL_QUIT;
        SDL_PushEvent(&event);
    } else if (strcmp(l, "quit") == 0) {
        *quit = true;
    } else {
        SDL_Log("devscript: bad line: %s", l);
    }
    return true;
}

void devscript_after_render(SDL_Renderer *r)
{
    if (!g_shot[0])
        return;
    int w, h;
    SDL_GetRendererOutputSize(r, &w, &h);
    SDL_Surface *s = SDL_CreateRGBSurfaceWithFormat(0, w, h, 32, SDL_PIXELFORMAT_ARGB8888);
    if (s && SDL_RenderReadPixels(r, NULL, SDL_PIXELFORMAT_ARGB8888, s->pixels, s->pitch) == 0) {
        SDL_SaveBMP(s, g_shot);
        SDL_Log("devscript: saved %s", g_shot);
    }
    SDL_FreeSurface(s);
    g_shot[0] = 0;
}

#endif
