#ifdef __vita__

#include "platform.h"
#include "vam_backend.h"

#include <psp2/apputil.h>
#include <psp2/notificationutil.h>
#include <psp2/sysmodule.h>
#include "../notifications.h"
#include <psp2/appmgr.h>
#include <psp2/ctrl.h>
#include <psp2/avconfig.h>
#include <psp2/io/fcntl.h>
#include <psp2/io/stat.h>
#include <psp2/kernel/processmgr.h>
#include <psp2/kernel/threadmgr.h>
#include <psp2/power.h>
#include <psp2/kernel/sysmem.h>
#include <psp2/registrymgr.h>
#include <psp2/system_param.h>
#include <stdio.h>
#include <string.h>

#define DATA_DIR "ux0:data/VitaSpot/"
/* Data folder used before the app was renamed from VitaAM. */
#define OLD_DATA_DIR "ux0:data/VitaAM"

/* Lower runs first (user range 64..191). Backend pthreads default to 159,
 * which used to outrank this thread's 160 and stall frames; the backend now
 * drops its network threads to 170 and lifts audio output to 80. */
#define PRIORITY_UI 150
#define PRIORITY_BACKGROUND 170

static bool g_swap_confirm;
static bool g_notifications_loaded;
static bool g_graphics_allowed = true;
static int power_callback_thread(SceSize args, void *argp);

void platform_init(void)
{
    /* Max clocks: UI is light but the backend decodes audio and runs TLS. */
    scePowerSetArmClockFrequency(444);
    scePowerSetBusClockFrequency(222);
    scePowerSetGpuClockFrequency(222);
    scePowerSetGpuXbarClockFrequency(166);

    sceKernelChangeThreadPriority(SCE_KERNEL_THREAD_ID_SELF, PRIORITY_UI);

    /* Above the UI so the backlight comes back before the app is suspended. */
    SceUID cb_thread = sceKernelCreateThread("vam_power", power_callback_thread, 100, 0x4000, 0, 0, NULL);
    if (cb_thread >= 0)
        sceKernelStartThread(cb_thread, 0, NULL);

    SceAppUtilInitParam init;
    SceAppUtilBootParam boot;
    memset(&init, 0, sizeof(init));
    memset(&boot, 0, sizeof(boot));
    sceAppUtilInit(&init, &boot);
    g_notifications_loaded = sceSysmoduleLoadModule(SCE_SYSMODULE_NOTIFICATION_UTIL) >= 0;

    int enter = SCE_SYSTEM_PARAM_ENTER_BUTTON_CROSS;
    sceAppUtilSystemParamGetInt(SCE_SYSTEM_PARAM_ID_ENTER_BUTTON, &enter);
    g_swap_confirm = enter == SCE_SYSTEM_PARAM_ENTER_BUTTON_CIRCLE;

    /* One-time move from the old folder so the saved login and caches
       survive the rename. Only when the new folder doesn't exist yet. */
    SceIoStat st;
    if (sceIoGetstat("ux0:data/VitaSpot", &st) < 0 && sceIoGetstat(OLD_DATA_DIR, &st) >= 0)
        sceIoRename(OLD_DATA_DIR, "ux0:data/VitaSpot");
    sceIoMkdir(DATA_DIR, 0777);
}

int platform_notify_song(const char *title, const char *artist)
{
    if (!g_notifications_loaded) return -1;
    /* SDK explicitly requires a 0x410-byte text buffer. Normal apps do not
       call BgAppInitialize. No history deletion or BGDL progress spoofing. */
    SceWChar16 text[0x410 / sizeof(SceWChar16)] = {0};
    char message[1024];
    snprintf(message, sizeof(message), "Vitafy - Now playing\n%s\n%s", title, artist);
    notifications_utf16(message, text, sizeof(text) / sizeof(text[0]));
    return sceNotificationUtilSendNotification(text);
}

void platform_shutdown(void)
{
    /* A development shutdown must not leave the next app's panel dark. */
    platform_set_screen(SCREEN_ON);
    if (g_notifications_loaded)
        sceSysmoduleUnloadModule(SCE_SYSMODULE_NOTIFICATION_UTIL);
    g_notifications_loaded = false;
    sceAppUtilShutdown();
}

void platform_memory(int *user,int *cdram,int *phycont){
    SceKernelFreeMemorySizeInfo info;memset(&info,0,sizeof(info));info.size=sizeof(info);
    *user=*cdram=*phycont=-1;
    if(sceKernelGetFreeMemorySize(&info)>=0){*user=info.size_user;*cdram=info.size_cdram;*phycont=info.size_phycont;}
}

void platform_frame(bool keep_awake)
{
    if (!g_graphics_allowed)
        return; /* Do not manage the shell's dimming/backlight policy. */
    (void)keep_awake;
    /* Foreground policy applies while browsing and paused as well as playing. */
    sceKernelPowerTick(SCE_KERNEL_POWER_TICK_DEFAULT);
}

void platform_poll_lifecycle(void)
{
    SceAppMgrAppState state;
    SceCtrlData pad;
    memset(&state, 0, sizeof(state));
    memset(&pad, 0, sizeof(pad));
    /* Version zero is used by existing native VitaSDK callers. */
    int rc = _sceAppMgrGetAppState(&state, sizeof(state), 0);
    int controls = sceCtrlPeekBufferPositive(0, &pad, 1);
    bool allowed = rc >= 0 && controls > 0 && !state.isSystemUiOverlaid &&
                   !(pad.buttons & SCE_CTRL_INTERCEPTED);
    if (allowed != g_graphics_allowed) {
        char message[160];
        snprintf(message, sizeof(message),
                 "lifecycle graphics=%d appstate=%#x overlay=%d controls=%d intercepted=%d",
                 allowed, rc, state.isSystemUiOverlaid, controls,
                 !!(pad.buttons & SCE_CTRL_INTERCEPTED));
        vam_log(message);
    }
    /* Unknown lifecycle/input state denies rendering until a valid sample. */
    g_graphics_allowed = allowed;
}

bool platform_graphics_allowed(void) { return g_graphics_allowed; }

bool platform_is_vita(void) { return true; }

static volatile screen_level g_screen = SCREEN_ON;
static volatile int g_woken;

/* The brightness the user set in Settings. Setting the backlight doesn't
   change it, so the full level can always be restored from here. */
static int user_brightness(void)
{
    int v = 0;
    if (sceRegMgrGetKeyInt("/CONFIG/DISPLAY", "brightness", &v) < 0 || v < 21)
        return 0;
    return v;
}

/* Leaving the app dark is the one thing that must not happen: the PS button,
   the power button and suspend all leave VitaSpot, and nothing would turn the
   backlight back on in the LiveArea or after resuming. The power callback
   hears about each of these and restores it first. */
#define WAKE_EVENTS                                                                                       (SCE_POWER_CB_BUTTON_PS_PRESS | SCE_POWER_CB_BUTTON_PS_HOLD | SCE_POWER_CB_BUTTON_PS_START_PRESS |        SCE_POWER_CB_BUTTON_PS_POWER_PRESS | SCE_POWER_CB_BUTTON_POWER_PRESS | SCE_POWER_CB_BUTTON_POWER_HOLD |      SCE_POWER_CB_APP_SUSPEND | SCE_POWER_CB_SYSTEM_SUSPEND | SCE_POWER_CB_APP_RESUME |                        SCE_POWER_CB_APP_RESUMING | SCE_POWER_CB_SYSTEM_RESUMING | SCE_POWER_CB_SYSTEM_RESUME)

static int power_callback(int notify_id, int notify_count, int notify_arg, void *user)
{
    (void)notify_id;
    (void)notify_count;
    (void)user;
    if ((notify_arg & WAKE_EVENTS) && g_screen != SCREEN_ON) {
        if (g_screen == SCREEN_OFF)
            scePowerRequestDisplayOn();
        int full = user_brightness();
        sceAVConfigSetDisplayBrightness(full ? full : 65536);
        g_screen = SCREEN_ON;
        g_woken = 1;
    }
    return 0;
}

/* Callbacks only run while their thread waits with a CB wait. */
static int power_callback_thread(SceSize args, void *argp)
{
    (void)args;
    (void)argp;
    SceUID cb = sceKernelCreateCallback("vam_power_cb", 0, power_callback, NULL);
    if (cb < 0 || scePowerRegisterCallback(cb) < 0)
        return 0;
    for (;;)
        sceKernelDelayThreadCB(60 * 1000 * 1000);
    return 0;
}

bool platform_take_wake(void)
{
    if (!g_woken)
        return false;
    g_woken = 0;
    return true;
}

/* Only the backlight changes. scePowerRequestDisplayOff() also silenced the
   app's audio, which is the whole point of turning the screen off. */
void platform_set_screen(screen_level level)
{
    if (level == g_screen)
        return;
    int full = user_brightness();
    int on = 0, rc;
    if (level == SCREEN_OFF) {
        rc = sceAVConfigSetDisplayBrightness(0); /* 0 turns the screen off */
    } else {
        /* Brightness 0 powers the panel down; a new level alone doesn't
           bring it back. */
        if (g_screen == SCREEN_OFF)
            on = scePowerRequestDisplayOn();
        int target = !full ? 65536 : level == SCREEN_DIM ? (full / 4 < 21 ? 21 : full / 4) : full;
        rc = sceAVConfigSetDisplayBrightness(target);
    }
    char msg[96];
    snprintf(msg, sizeof(msg), "display level %d (from %d): brightness rc %#x, display on rc %#x, user %d", level,
             g_screen, rc, on, full);
    vam_log(msg);
    g_screen = level;
}

const char *platform_btn_label(btn b)
{
    switch (b) {
    case BTN_UP: return "Up";
    case BTN_DOWN: return "Down";
    case BTN_LEFT: return "Left";
    case BTN_RIGHT: return "Right";
    case BTN_CONFIRM: return g_swap_confirm ? "Circle" : "Cross";
    case BTN_BACK: return g_swap_confirm ? "Cross" : "Circle";
    case BTN_TRIANGLE: return "Triangle";
    case BTN_SQUARE: return "Square";
    case BTN_L: return "L";
    case BTN_R: return "R";
    case BTN_START: return "Start";
    case BTN_SELECT: return "Select";
    default: return "?";
    }
}

void platform_lower_thread_priority(void)
{
    sceKernelChangeThreadPriority(SCE_KERNEL_THREAD_ID_SELF, PRIORITY_BACKGROUND);
}

const char *platform_asset_path(const char *rel, char *buf, size_t size)
{
    snprintf(buf, size, "app0:assets/%s", rel);
    return buf;
}

const char *platform_data_dir(void) { return DATA_DIR; }

/* SDL2's Vita joystick button order. */
int platform_map_vita_button(int b)
{
    switch (b) {
    case 0: return BTN_TRIANGLE;
    case 1: return g_swap_confirm ? BTN_CONFIRM : BTN_BACK;  /* Circle */
    case 2: return g_swap_confirm ? BTN_BACK : BTN_CONFIRM;  /* Cross  */
    case 3: return BTN_SQUARE;
    case 4: return BTN_L;
    case 5: return BTN_R;
    case 6: return BTN_DOWN;
    case 7: return BTN_LEFT;
    case 8: return BTN_UP;
    case 9: return BTN_RIGHT;
    case 10: return BTN_SELECT;
    case 11: return BTN_START;
    default: return -1;
    }
}

#endif
