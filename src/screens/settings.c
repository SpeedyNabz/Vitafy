/* Settings tab: the signed-in account (with log out), screen power, audio quality and cache, and about. */

#include <stdio.h>

#include "../app.h"
#include "../gfx/images.h"
#include "../gfx/text.h"
#include "../power.h"
#include "../prefs.h"
#include "../theme.h"

/* Log out asks once more before it forgets the saved login. */
static bool g_confirm_logout;
static float g_last_drawn;

static float pill_w(const char *label) { return text_width(label, FS_S, FW_REGULAR, 0) + 32.0f; }

static float account_card(float x, float y, float w)
{
    rectf card = rect(x, y, w, 96);
    gfx_rrect(card, 8, rgb(C_SURFACE));

    rectf pic = rect(card.x + 16, card.y + 16, 64, 64);
    SDL_Texture *avatar = images_get_round(A.auth.image_url, NULL);
    if (avatar) {
        gfx_image(avatar, pic, 255);
    } else {
        gfx_circle(pic.x + 32, pic.y + 32, 32, rgb(C_SURFACE_HI2));
        gfx_icon(ICON_PERSON, pic.x + 32, pic.y + 32, 32, rgb(C_SUBTEXT));
    }

    /* Buttons sit at the right; the text gets what is left. */
    const char *confirm = "Log out", *cancel = "Cancel";
    float by = card.y + (card.h - 40) / 2, right = card.x + card.w - 16;
    float buttons_w = g_confirm_logout ? pill_w(cancel) + 8 + pill_w(confirm) : pill_w(confirm);
    float tx = pic.x + pic.w + 16, tw = right - buttons_w - 16 - tx;

    const char *name = A.auth.display_name[0] ? A.auth.display_name : A.auth.username;
    text_draw(name, tx, card.y + 22, FS_L, FW_BOLD, rgb(C_TEXT), tw);
    if (g_confirm_logout)
        text_draw("Log out of Spotify on this Vita?", tx, card.y + 54, FS_S, FW_REGULAR, rgb(C_TEXT), tw);
    else
        text_draw(A.auth.is_premium ? "Spotify Premium" : "Spotify Free", tx, card.y + 54, FS_S, FW_REGULAR,
                  rgb(C_SUBTEXT), tw);

    if (g_confirm_logout) {
        float cx = right - buttons_w;
        if (pill_button(ui_id("settings.cancel"), cancel, cx, by, false, NULL)) {
            g_confirm_logout = false;
            ui_set_focus(ui_id("settings.logout"));
        }
        if (pill_button(ui_id("settings.confirm"), confirm, cx + pill_w(cancel) + 8, by, true, NULL)) {
            g_confirm_logout = false;
            if (vam_logout() != VAM_OK)
                app_toast("Couldn't log out. Try again.");
        }
    } else if (pill_button(ui_id("settings.logout"), confirm, right - buttons_w, by, false, NULL)) {
        g_confirm_logout = true;
        ui_set_focus(ui_id("settings.cancel"));
    }
    return card.h;
}

#define SCREEN_ROW_H 64

static void timer_row(uint32_t id,const char *label,int *seconds,const int *choices,float x,float y,float right)
{
    char value[32];if(!*seconds)snprintf(value,sizeof(value),"Never");else snprintf(value,sizeof(value),"%d seconds",*seconds);
    text_draw_vc(label,x,y+32,FS_M,FW_REGULAR,rgb(C_TEXT),300);
    if(pill_button(id,value,right-pill_w(value),y+12,false,NULL)){
        int pick=0;for(int i=0;i<POWER_CHOICE_COUNT;i++)if(*seconds==choices[i])pick=i;
        *seconds=choices[(pick+1)%POWER_CHOICE_COUNT];prefs_changed();power_apply_prefs();
    }
}
static float screen_card(float x,float y,float w)
{
    gfx_rrect(rect(x,y,w,128),8,rgb(C_SURFACE));
    gfx_rect(rect(x+16,y+64,w-32,1),rgb(C_DIVIDER));
    prefs *p=prefs_get();
    timer_row(ui_id("settings.dim"),"Dim screen after",&p->dim_after,POWER_DIM_CHOICES,x+16,y,x+w-16);
    timer_row(ui_id("settings.off"),"Turn screen off after",&p->off_after,POWER_OFF_CHOICES,x+16,y+64,x+w-16);
    text_draw("Default: Never. A button wakes the screen; Home uses the shell's policy.",x,y+138,FS_S,FW_REGULAR,rgb(C_MUTED),w);
    return 138+text_line_height(FS_S);
}

static void fmt_cache(int mb, char *buf, size_t n)
{
    if (mb >= 1024)
        snprintf(buf, n, "%d GB", mb / 1024);
    else
        snprintf(buf, n, "%d MB", mb);
}

static float audio_card(float x, float y, float w)
{
    prefs *p = prefs_get();
    rectf card = rect(x, y, w, SCREEN_ROW_H * 2);
    gfx_rrect(card, 8, rgb(C_SURFACE));
    gfx_rect(rect(card.x + 16, card.y + SCREEN_ROW_H, card.w - 32, 1), rgb(C_DIVIDER));
    float cy = card.y + SCREEN_ROW_H / 2.0f, right = card.x + card.w - 16;
    text_draw_vc("Audio quality", card.x + 16, cy, FS_M, FW_REGULAR, rgb(C_TEXT), 260);
    const char *quality=p->bitrate==96?"Low (96 kbps)":p->bitrate==160?"Medium (160 kbps)":"Best available";
    if(pill_button(ui_id("settings.quality"),quality,right-pill_w(quality),cy-20,false,NULL)){
        int next=p->bitrate==0?160:p->bitrate==160?96:0;
        if(vam_set_bitrate(next)==VAM_OK){p->bitrate=next;prefs_changed();app_toast("Quality applies from the next loaded song.");}
        else app_toast("Couldn't change audio quality.");
    }

    /* The backend opens the cache at start, so a new size waits for a restart. */
    cy += SCREEN_ROW_H;
    text_draw_vc("Audio cache", card.x + 16, cy, FS_M, FW_REGULAR, rgb(C_TEXT), 260);
    char size[16];
    fmt_cache(p->cache_mb, size, sizeof(size));
    if (pill_button(ui_id("settings.cache"), size, right - pill_w(size), cy - 20, false, NULL)) {
        int i = 0;
        while (i < CACHE_CHOICE_COUNT && CACHE_CHOICES[i] != p->cache_mb)
            i++;
        p->cache_mb = CACHE_CHOICES[(i + 1) % CACHE_CHOICE_COUNT];
        prefs_changed();
    }

    float ny = card.y + card.h + 10, lh = text_line_height(FS_S);
    text_draw("Best prefers lossless. Medium and Low limit source bitrate to save bandwidth.", x, ny, FS_S,
              FW_REGULAR, rgb(C_MUTED), w);
    text_draw("Played songs are kept on the memory card. A new cache size applies after a restart.", x, ny + lh,
              FS_S, FW_REGULAR, rgb(C_MUTED), w);
    float cy2=ny+2*lh+8;
    if(pill_button(ui_id("settings.clear_metadata"),A.cache_request?"Clearing...":"Clear browse cache",x,cy2,false,NULL)&&!A.cache_request)A.cache_request=vam_clear_metadata_cache();
    char budget[64];snprintf(budget,sizeof(budget),"Browse cache: %d MB",p->metadata_mb);
    if(pill_button(ui_id("settings.metadata_budget"),budget,x+230,cy2,false,NULL)){
        int mb=p->metadata_mb==32?16:32;if(vam_set_metadata_cache_mb(mb)==VAM_OK){p->metadata_mb=mb;prefs_changed();}
    }
    return card.h+10+2*lh+56;
}

static float about_card(float x, float y, float w)
{
    rectf card = rect(x, y, w, 96);
    gfx_rrect(card, 8, rgb(C_SURFACE));
    text_draw("Vitafy", card.x + 16, card.y + 20, FS_M, FW_BOLD, rgb(C_TEXT), card.w - 32);
    text_draw_aligned("Version " VAM_APP_VERSION, card.x + card.w - 16, card.y + 22, FS_S, FW_REGULAR,
                      rgb(C_SUBTEXT), 0, ALIGN_RIGHT);
    text_draw("An unofficial Spotify client for the PS Vita. Not affiliated with Spotify.", card.x + 16,
              card.y + 50, FS_S, FW_REGULAR, rgb(C_SUBTEXT), card.w - 32);
    return card.h;
}

void screen_settings(view *v)
{
    /* Coming back to the tab starts from the plain Log out button. */
    if (A.time - g_last_drawn > 0.25f) {
        g_confirm_logout = false;
        if (ui_capturing_combo())
            ui_capture_combo(false);
    }
    g_last_drawn = A.time;

    rectf vp = rect(CONTENT_X, CONTENT_Y, CONTENT_W, CONTENT_H);
    gfx_vgradient(rect(CONTENT_X, 0, CONTENT_W, 280), rgb(0x2A2A2A), rgb(C_BG));
    ui_scroll_begin(&v->scroll, vp, v->content_h, AXIS_Y, true);
    float x = CONTENT_X + PAD, w = CONTENT_W - 2 * PAD;
    float y = CONTENT_Y + 22 - v->scroll.pos;
    text_draw("Settings", x, y, FS_XXL, FW_BOLD, rgb(C_TEXT), w);
    if (pill_button(ui_id("settings.exit"), "Exit Vitafy", x+w-124, y, false, NULL))
        A.quit = true; /* normal app_shutdown joins the playback backend */
    y += 68;

    text_draw("Account", x, y, FS_L, FW_BOLD, rgb(C_TEXT), w);
    y += 38;
    y += account_card(x, y, w) + 32;

    text_draw("Screen", x, y, FS_L, FW_BOLD, rgb(C_TEXT), w);
    y += 38;
    y += screen_card(x, y, w) + 32;

    text_draw("Audio", x, y, FS_L, FW_BOLD, rgb(C_TEXT), w);
    y += 38;
    y += audio_card(x, y, w) + 32;

    text_draw("Notifications", x, y, FS_L, FW_BOLD, rgb(C_TEXT), w);
    y += 38;
    text_draw("Show song and artist when the track changes", x, y, FS_S, FW_REGULAR, rgb(C_SUBTEXT), w-100);
    if (pill_button(ui_id("settings.notifications"), prefs_get()->notifications ? "On" : "Off", x+w-80, y-8, prefs_get()->notifications, NULL)) {
        prefs_get()->notifications = !prefs_get()->notifications;
        prefs_changed();
    }
    y += 68;
    text_draw("About", x, y, FS_L, FW_BOLD, rgb(C_TEXT), w);
    y += 38;
    y += about_card(x, y, w);

    v->content_h = y + v->scroll.pos - CONTENT_Y + 24;
    ui_scroll_end();
}
