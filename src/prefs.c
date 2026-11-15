#include "prefs.h"
#include "power.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <SDL.h>

/* How long prefs must stay unchanged before they are written. */
#define SAVE_DELAY_MS 1000

const int POWER_DIM_CHOICES[] = {0,15,30,60,120,300};
const int POWER_OFF_CHOICES[] = {0,30,60,120,300,600};
const int POWER_CHOICE_COUNT = 6;

const int CACHE_CHOICES[CACHE_CHOICE_COUNT] = { 256, 512, 768, 1024 };

static void save_views(void);
static prefs g_prefs;
static bool g_dirty;
static Uint64 g_changed_at;

/* Names in settings.txt; stable even if the btn enum is reordered. */
static const char *const KEYS[BTN_COUNT] = { "up", "down", "left", "right", "confirm", "back",
                                             "triangle", "square", "l", "r", "start", "select" };

static int key_btn(const char *s)
{
    for (int i = 0; i < BTN_COUNT; i++)
        if (strcmp(KEYS[i], s) == 0)
            return i;
    return -1;
}

static void settings_path(char *buf, size_t n) { snprintf(buf, n, "%ssettings.txt", platform_data_dir()); }

/* The new file is written here first, then renamed over settings.txt. */
static void temp_path(char *buf, size_t n) { snprintf(buf, n, "%ssettings.txt.tmp", platform_data_dir()); }

void prefs_load(void)
{
    g_prefs.notifications = true;
    g_prefs.combo[0] = BTN_SELECT;
    g_prefs.combo[1] = BTN_START;
    g_prefs.dim_after = 0;
    g_prefs.off_after = 0;
    g_prefs.bitrate = 0; /* automatic: lossless first, highest available fallback */
    g_prefs.cache_mb = 512;
    g_prefs.metadata_mb=32;

    char path[512];
    settings_path(path, sizeof(path));
    FILE *f = fopen(path, "r");
    if (!f) {
        /* A save that stopped between removing settings.txt and renaming the
           finished temp file over it leaves only the temp file. */
        temp_path(path, sizeof(path));
        f = fopen(path, "r");
    }
    if (!f)
        return;
    char line[128];
    while (fgets(line, sizeof(line), f)) {
        line[strcspn(line, "\r\n")] = 0;
        char *eq = strchr(line, '=');
        if (!eq)
            continue;
        *eq = 0;
        const char *v = eq + 1;
        if (strcmp(line, "screen_off_combo") == 0) {
            char a[16], b[16];
            if (sscanf(v, "%15[^+]+%15s", a, b) == 2) {
                int x = key_btn(a), y = key_btn(b);
                if (x >= 0 && y >= 0 && x != y) {
                    g_prefs.combo[0] = (btn)x;
                    g_prefs.combo[1] = (btn)y;
                }
            }
        } else if (strcmp(line, "dim_after") == 0) {
            g_prefs.dim_after = atoi(v);
            bool valid=false;for(int i=0;i<POWER_CHOICE_COUNT;i++)if(g_prefs.dim_after==POWER_DIM_CHOICES[i])valid=true;
            if(!valid)g_prefs.dim_after=0;
        } else if (strcmp(line, "screen_off_after") == 0) {
            g_prefs.off_after = atoi(v);
            bool valid=false;for(int i=0;i<POWER_CHOICE_COUNT;i++)if(g_prefs.off_after==POWER_OFF_CHOICES[i])valid=true;
            if(!valid)g_prefs.off_after=0;
        } else if (strcmp(line, "audio_quality") == 0) {
            int k = atoi(v);
            g_prefs.bitrate = (k==96||k==160)?k:0;
        } else if(!strcmp(line,"metadata_cache_mb")){
            int mb=atoi(v);g_prefs.metadata_mb=mb==16?16:32;
        } else if (strcmp(line, "audio_cache_mb") == 0) {
            int mb = atoi(v);
            for (int i = 0; i < CACHE_CHOICE_COUNT; i++)
                if (CACHE_CHOICES[i] == mb)
                    g_prefs.cache_mb = mb;
        } else if (strcmp(line, "notifications") == 0) {
            g_prefs.notifications = atoi(v) != 0;
        } else if (strcmp(line, "lyrics") == 0) {
            g_prefs.lyrics = atoi(v) != 0;
        }
    }
    fclose(f);
}

prefs *prefs_get(void) { return &g_prefs; }

/* Writes a complete temp file, then swaps it in, so a crash or power-off
   mid-save never leaves a truncated settings.txt behind. */
static void save(void)
{
    char path[512], tmp[512];
    settings_path(path, sizeof(path));
    temp_path(tmp, sizeof(tmp));
    FILE *f = fopen(tmp, "w");
    if (!f)
        return;
    fprintf(f, "screen_off_combo=%s+%s\n", KEYS[g_prefs.combo[0]], KEYS[g_prefs.combo[1]]);
    fprintf(f, "dim_after=%d\n", g_prefs.dim_after);
    fprintf(f, "screen_off_after=%d\n", g_prefs.off_after);
    fprintf(f, "audio_quality=%d\n", g_prefs.bitrate);
    fprintf(f, "audio_cache_mb=%d\n", g_prefs.cache_mb);
    fprintf(f,"metadata_cache_mb=%d\n",g_prefs.metadata_mb);
    fprintf(f, "notifications=%d\n", g_prefs.notifications ? 1 : 0);
    fprintf(f, "lyrics=%d\n", g_prefs.lyrics ? 1 : 0);
    bool ok = !ferror(f);
    if (fclose(f) != 0 || !ok) {
        remove(tmp);
        return;
    }
    /* The Vita and Windows refuse to rename over an existing file. */
    if (rename(tmp, path) != 0) {
        remove(path);
        rename(tmp, path);
    }
}

void prefs_changed(void)
{
    g_dirty = true;
    g_changed_at = SDL_GetTicks64();
}

void prefs_frame(void)
{
    if (g_dirty && SDL_GetTicks64() - g_changed_at >= SAVE_DELAY_MS)
        prefs_flush();
}

void prefs_flush(void)
{
    if (!g_dirty)
        return;
    g_dirty = false;
    save();
    save_views();
}

/* Fixed-size records, bounded to 64 recent account/page preferences. */
typedef struct view_pref { char account[192],uri[96]; int sort; unsigned flags; } view_pref;
static view_pref g_views[64];
static unsigned g_view_count;
static bool g_views_loaded,g_views_dirty;
static uint32_t g_item_revision=1;

static void view_path(char *p,size_t n) { snprintf(p,n,"%sview-options.bin",platform_data_dir()); }
static void load_views(void)
{
    if(g_views_loaded) return;
    g_views_loaded=true;
    char p[512]; view_path(p,sizeof(p)); FILE *f=fopen(p,"rb"); if(!f)return;
    unsigned magic=0,n=0;
    if(fread(&magic,sizeof(magic),1,f)==1 && magic==0x56504631 && fread(&n,sizeof(n),1,f)==1 && n<=64) {
        if(fread(g_views,sizeof(view_pref),n,f)==n) g_view_count=n;
    }
    fclose(f);
    for(unsigned i=0;i<g_view_count;i++) { g_views[i].account[191]=0; g_views[i].uri[95]=0; }
}
void prefs_view_load(const char *account,const char *uri,int *sort,bool *desc,bool *compact,bool *grid)
{
    load_views();
    for(unsigned i=0;i<g_view_count;i++) if(!strcmp(g_views[i].account,account)&&!strcmp(g_views[i].uri,uri)) {
        if(g_views[i].sort>=0 && g_views[i].sort<9) *sort=g_views[i].sort;
        *desc=(g_views[i].flags&1)!=0; *compact=(g_views[i].flags&2)!=0; *grid=(g_views[i].flags&4)!=0; return;
    }
}
void prefs_view_save(const char *account,const char *uri,int sort,bool desc,bool compact,bool grid)
{
    load_views(); unsigned i;
    for(i=0;i<g_view_count;i++) if(!strcmp(g_views[i].account,account)&&!strcmp(g_views[i].uri,uri)) break;
    if(i==64) { memmove(g_views,g_views+1,sizeof(view_pref)*63); i=63; memset(&g_views[i],0,sizeof(view_pref)); }
    else if(i==g_view_count) {g_view_count++;memset(&g_views[i],0,sizeof(view_pref));}
    snprintf(g_views[i].account,sizeof(g_views[i].account),"%s",account);
    snprintf(g_views[i].uri,sizeof(g_views[i].uri),"%s",uri);
    g_views[i].sort=sort; g_views[i].flags=(g_views[i].flags&24)|(desc?1:0)|(compact?2:0)|(grid?4:0);
    g_views_dirty=true; prefs_changed();
}
static void save_views(void)
{
    if(!g_views_dirty) return;
    char p[512],tmp[520];view_path(p,sizeof(p));snprintf(tmp,sizeof(tmp),"%s.tmp",p);
    FILE *f=fopen(tmp,"wb");if(!f)return;
    unsigned magic=0x56504631;
    bool ok=fwrite(&magic,sizeof(magic),1,f)==1 && fwrite(&g_view_count,sizeof(g_view_count),1,f)==1 &&
        fwrite(g_views,sizeof(view_pref),g_view_count,f)==g_view_count;
    if(fclose(f)!=0)ok=false;
    if(ok) { if(rename(tmp,p)!=0) {remove(p);ok=rename(tmp,p)==0;} }
    if(ok)g_views_dirty=false;
}

unsigned prefs_item_flags(const char *account,const char *uri)
{
    load_views();for(unsigned i=0;i<g_view_count;i++)
        if(!strcmp(g_views[i].account,account)&&!strcmp(g_views[i].uri,uri))return g_views[i].flags;
    return 0;
}
uint32_t prefs_item_revision(void){return g_item_revision;}
void prefs_item_toggle(const char *account,const char *uri,unsigned flag)
{
    if(flag!=8 && flag!=16)return;
    load_views();unsigned i;
    for(i=0;i<g_view_count;i++)if(!strcmp(g_views[i].account,account)&&!strcmp(g_views[i].uri,uri))break;
    if(i==64){memmove(g_views,g_views+1,sizeof(view_pref)*63);i=63;memset(&g_views[i],0,sizeof(view_pref));g_views[i].sort=-1;}
    else if(i==g_view_count){g_view_count++;memset(&g_views[i],0,sizeof(view_pref));g_views[i].sort=-1;}
    snprintf(g_views[i].account,sizeof(g_views[i].account),"%s",account);snprintf(g_views[i].uri,sizeof(g_views[i].uri),"%s",uri);
    g_views[i].flags^=flag;g_views_dirty=true;g_item_revision++;prefs_changed();
}
