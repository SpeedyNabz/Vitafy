#ifndef __vita__

#include "platform.h"

#include <SDL.h>
#include <stdio.h>

static bool g_graphics_allowed = true;
static char g_base[1024];
static char g_data[1024];

void platform_init(void)
{
    char *base = SDL_GetBasePath();
    snprintf(g_base, sizeof(g_base), "%s", base ? base : "./");
    SDL_free(base);

    /* VAM_DATA_DIR overrides the data folder, e.g. for scripted test runs. */
    const char *override = SDL_getenv("VAM_DATA_DIR");
    if (override && override[0]) {
        snprintf(g_data, sizeof(g_data), "%s", override);
        return;
    }
    char *pref = SDL_GetPrefPath("Vitafy", "Vitafy");
    snprintf(g_data, sizeof(g_data), "%s", pref ? pref : "./");
    SDL_free(pref);
}

void platform_shutdown(void) {}
int platform_notify_song(const char *title, const char *artist)
{
    (void)title; (void)artist;
    SDL_Log("Vitafy mock song notification");
    return 0;
}

void platform_memory(int *user,int *cdram,int *phycont){*user=*cdram=*phycont=-1;}

void platform_frame(bool keep_awake) { (void)keep_awake; }

void platform_poll_lifecycle(void) {}
bool platform_graphics_allowed(void) { return g_graphics_allowed; }
void platform_test_overlay(bool overlaid)
{
    g_graphics_allowed = !overlaid;
    SDL_Log("test overlay=%d", overlaid);
}

bool platform_is_vita(void) { return false; }

void platform_set_screen(screen_level level) { (void)level; }

bool platform_take_wake(void) { return false; }

const char *platform_btn_label(btn b)
{
    static const char *const names[BTN_COUNT] = { "Up", "Down", "Left", "Right", "Cross", "Circle",
                                                  "Triangle", "Square", "L", "R", "Start", "Select" };
    return b >= 0 && b < BTN_COUNT ? names[b] : "?";
}

void platform_lower_thread_priority(void) { SDL_SetThreadPriority(SDL_THREAD_PRIORITY_LOW); }

const char *platform_asset_path(const char *rel, char *buf, size_t size)
{
    snprintf(buf, size, "%sassets/%s", g_base, rel);
    return buf;
}

const char *platform_data_dir(void) { return g_data; }

int platform_map_vita_button(int b)
{
    (void)b;
    return -1;
}

#endif
