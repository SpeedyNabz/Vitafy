#ifndef VAM_PREFS_H
#define VAM_PREFS_H

#include <stdint.h>

/* User settings, saved as key=value lines in <data dir>/settings.txt. */

#include "platform/platform.h"

typedef struct prefs {
    btn combo[2];   /* screen-off combo                    */
    int dim_after;  /* seconds without input, 0 = never     */
    int off_after;  /* seconds without input, 0 = never     */
    int bitrate;    /* 0: best available, 160: medium, 96: low */
    int metadata_mb; /* browse metadata cache budget, 16/32 MiB */
    int cache_mb;   /* audio cache cap, one of CACHE_CHOICES */
    bool notifications; /* track-change song/artist notifications */
    bool lyrics;    /* Now Playing shows lyrics, not the art */
} prefs;

#define CACHE_CHOICE_COUNT 4
extern const int CACHE_CHOICES[CACHE_CHOICE_COUNT];

/* Defaults, then whatever settings.txt has. Call before vam_init. */
void prefs_load(void);
prefs *prefs_get(void);
/* Call after changing prefs: the file is written once the changes settle, so
 * cycling through options doesn't hit the memory card on every press. */
void prefs_changed(void);
/* Once per frame: writes settings.txt when a change has settled. */
void prefs_frame(void);
/* Writes pending changes now (at shutdown). */
void prefs_flush(void);
unsigned prefs_item_flags(const char *account,const char *uri);
void prefs_item_toggle(const char *account,const char *uri,unsigned flag);
uint32_t prefs_item_revision(void);
void prefs_view_load(const char *account,const char *uri,int *sort,bool *desc,bool *compact,bool *grid);
void prefs_view_save(const char *account,const char *uri,int sort,bool desc,bool compact,bool grid);

#endif
