#include <stdio.h>

#include "../app.h"
#include "../gfx/images.h"
#include "../gfx/text.h"
#include "../theme.h"

void draw_vitafy_logo(float cx, float cy, float size)
{
    float s = size / 100.0f;
    color ink = rgb(0x08130C);
    gfx_circle(cx, cy, size / 2, rgb(C_ACCENT));
    for (int row = 0; row < 3; row++) {
        float w = 25 - row * 3, y = cy + (-16 + row * 12) * s;
        for (int i = 0; i < 20; i++) {
            float x0 = -w + 2*w*i/20, x1 = -w + 2*w*(i+1)/20;
            gfx_line(cx+x0*s, y+(x0*x0/100)*s, cx+x1*s, y+(x1*x1/100)*s, (5-row*.5f)*s, ink);
        }
    }
    gfx_line(cx-34*s,cy+16*s,cx-20*s,cy+16*s,4*s,ink);
    gfx_line(cx-27*s,cy+9*s,cx-27*s,cy+23*s,4*s,ink);
    gfx_circle(cx+27*s,cy+10*s,2.7f*s,ink);
    gfx_circle(cx+34*s,cy+17*s,2.7f*s,ink);
    gfx_circle(cx+20*s,cy+17*s,2.7f*s,ink);
    gfx_circle(cx+27*s,cy+24*s,2.7f*s,ink);
}

void fmt_duration(uint32_t ms, char *buf, size_t n)
{
    uint32_t s = ms / 1000;
    if (s >= 3600)
        snprintf(buf, n, "%u:%02u:%02u", s / 3600, (s / 60) % 60, s % 60);
    else
        snprintf(buf, n, "%u:%02u", s / 60, s % 60);
}

const char *kind_label(int kind)
{
    switch (kind) {
    case VAM_KIND_FOLDER: return "Folder";
    case VAM_KIND_TRACK: return "Song";
    case VAM_KIND_EPISODE: return "Episode";
    case VAM_KIND_ALBUM: return "Album";
    case VAM_KIND_PLAYLIST: return "Playlist";
    case VAM_KIND_ARTIST: return "Artist";
    case VAM_KIND_SHOW: return "Podcast";
    case VAM_KIND_COLLECTION: return "Playlist";
    default: return "";
    }
}

void draw_cover(const char *url, rectf r, float radius, int kind, const char *title)
{
    (void)title;
    bool round = radius >= (r.w < r.h ? r.w : r.h) * 0.5f - 0.01f;
    SDL_Texture *t = round ? images_get_round(url, NULL) : images_get(url, NULL);
    if (t) {
        gfx_image(t, r, 255);
        return;
    }
    float s = r.w < r.h ? r.w : r.h;
    if (kind == VAM_KIND_COLLECTION) {
        /* Liked Songs tile: own gradient + heart. */
        gfx_rrect(r, radius, rgb(C_LIKED_TOP));
        gfx_vgradient(rect_inset(r, radius > 2 ? radius * 0.3f : 0), rgba(C_LIKED_TOP, 0),
                      rgba(C_LIKED_BOT, 180));
        gfx_icon(ICON_HEART, r.x + r.w / 2, r.y + r.h / 2, s * 0.42f, rgb(0xFFFFFF));
        return;
    }
    gfx_rrect(r, radius, rgb(C_SURFACE_HI));
    icon_id ic = ICON_NOTE;
    if (kind == VAM_KIND_ARTIST) ic = ICON_PERSON;
    else if (kind == VAM_KIND_ALBUM) ic = ICON_DISC;
    else if (kind == VAM_KIND_FOLDER) ic = ICON_LIBRARY;
    gfx_icon(ic, r.x + r.w / 2, r.y + r.h / 2, s * 0.4f, rgb(C_MUTED));
}

void draw_focus_ring(rectf r, float radius)
{
    gfx_rrect_outline(rect_inset(r, -3), radius + 3, 2.5f, rgb(0xFFFFFF));
}

bool icon_button(uint32_t id, icon_id icon, float cx, float cy, float size, float hit, color c)
{
    if (hit < TOUCH_MIN)
        hit = TOUCH_MIN;
    rectf r = rect(cx - hit / 2, cy - hit / 2, hit, hit);
    ui_res res = ui_item(id, r, 0);
    float s = res.pressed ? size * 0.9f : size;
    if (res.pressed)
        c = with_alpha(c, 0.7f);
    if (res.focused)
        gfx_circle(cx, cy, hit / 2, rgba(0xFFFFFF, 40));
    gfx_icon(icon, cx, cy, s, c);
    if (res.focused)
        draw_focus_ring(r, hit / 2);
    return res.clicked;
}

static const int playlist_sorts[] = { SORT_CUSTOM, SORT_TITLE, SORT_ARTIST, SORT_ALBUM, SORT_ADDED, SORT_RELEASE, SORT_DURATION };
static const char *playlist_sort_names[] = { "Custom order", "Title", "Artist", "Album", "Recently added", "Release date", "Duration" };
static const int library_sorts[] = { SORT_CUSTOM, SORT_RECENTS, SORT_ADDED, SORT_TITLE, SORT_CREATOR };
static const char *library_sort_names[] = { "Custom order", "Recents", "Recently added", "Alphabetical", "Creator" };

static void sort_options(const view *v, const int **modes, const char *const **names, int *count)
{
    bool library=v->type==VIEW_LIBRARY;
    *modes=library?library_sorts:playlist_sorts;
    *names=library?library_sort_names:playlist_sort_names;
    *count=library?5:7;
}

static void sort_direction(float x,float y,bool descending,color c)
{
    float tip=y+(descending?6:-6),tail=y+(descending?-6:6);
    gfx_line(x,tail,x,tip,2,c);
    gfx_line(x,tip,x-5,tip+(descending?-5:5),2,c);
    gfx_line(x,tip,x+5,tip+(descending?-5:5),2,c);
}

void draw_sort_button(view *v,float x,float y)
{
    const int *modes;const char *const *names;int count;
    sort_options(v,&modes,&names,&count);
    int pick=0;for(int i=0;i<count;i++)if(v->sort==modes[i])pick=i;
    rectf r=rect(x,y,220,44);
    ui_res res=ui_item(ui_id("sort.open"),r,0);
    if(res.focused||res.pressed)gfx_rrect(r,5,rgb(C_SURFACE_HI));
    text_draw_vc(names[pick],x+10,y+22,FS_S,FW_REGULAR,rgb(C_SUBTEXT),172);
    sort_direction(x+202,y+22,v->descending,rgb(C_SUBTEXT));
    if(res.clicked){
        v->sort_anchor=r;v->sort_open=true;v->sort_scroll.pos=0;
        v->sort_focus=ui_focus();ui_set_focus(0);ui_reset_focus_list();
    }
}

void draw_sort_menu(view *v)
{
    const int *modes;const char *const *names;int count;
    sort_options(v,&modes,&names,&count);
    float h=44+count*44.0f;
    float x=v->sort_anchor.x+v->sort_anchor.w-240,y=v->sort_anchor.y+46;
    if(y+h>SCREEN_H-PLAYER_H-8)y=SCREEN_H-PLAYER_H-h-8;
    if(y<8)y=8;
    if(x<CONTENT_X)x=CONTENT_X;
    bool close=ui_item(ui_id("sort.dismiss"),rect(0,0,SCREEN_W,SCREEN_H),UI_F_NOFOCUS).clicked;
    rectf panel=rect(x,y,240,h);
    ui_item(ui_id("sort.panel"),panel,UI_F_NOFOCUS);
    gfx_shadow(panel,6,10,130);gfx_rrect(panel,6,rgb(C_SURFACE_HI));
    text_draw("Sort by",x+16,y+14,FS_XS,FW_BOLD,rgb(C_SUBTEXT),208);
    rectf options=rect(x+4,y+40,232,h-44);
    gfx_push_clip(options);ui_push_clip(options);
    ui_scroll_begin(&v->sort_scroll,options,count*44.0f,AXIS_Y,true);
    for(int i=0;i<count;i++){
        rectf row=rect(x+4,y+40+i*44-v->sort_scroll.pos,232,44);
        ui_res res=ui_item(ui_idx(ui_id("sort.option"),i),row,0);
        bool selected=v->sort==modes[i];color c=rgb(selected?C_ACCENT:C_TEXT);
        if(res.focused||res.pressed)gfx_rrect(row,4,rgb(C_SURFACE_HI2));
        text_draw_vc(names[i],x+16,row.y+22,FS_S,FW_REGULAR,c,184);
        if(selected)sort_direction(x+216,row.y+22,v->descending,c);
        if(res.clicked){
            if(selected)v->descending=!v->descending;
            else {v->sort=modes[i];v->descending=v->sort==SORT_ADDED||v->sort==SORT_RELEASE||v->sort==SORT_RECENTS;}
            v->scroll.pos=0;close=true;
        }
    }
    ui_scroll_end();ui_pop_clip();gfx_pop_clip();
    if(close){v->sort_open=false;ui_set_focus(v->sort_focus);ui_reset_focus_list();}
}

bool round_button(uint32_t id, icon_id icon, float cx, float cy, float radius, color bg, color fg)
{
    rectf r = rect(cx - radius, cy - radius, radius * 2, radius * 2);
    ui_res res = ui_item(id, r, 0);
    float k = res.pressed ? 0.94f : 1.0f;
    gfx_circle(cx, cy, radius * k, res.pressed ? color_scale(bg, 0.85f) : bg);
    /* Play triangles look centred when nudged right. */
    float nudge = icon == ICON_PLAY ? radius * 0.08f : 0;
    gfx_icon(icon, cx + nudge, cy, radius * 0.95f * k, fg);
    if (res.focused)
        draw_focus_ring(r, radius);
    return res.clicked;
}

bool pill_button(uint32_t id, const char *label, float x, float y, bool active, float *out_w)
{
    float w = text_width(label, FS_S, FW_REGULAR, 0) + 32.0f, h = 40;
    rectf r = rect(x, y, w, h);
    ui_res res = ui_item(id, rect(x,y,w,48), 0);
    color bg = active ? rgb(C_ACCENT) : rgb(res.pressed ? C_SURFACE_HI : C_SURFACE_HI2);
    color fg = active ? rgb(0x000000) : rgb(C_TEXT);
    gfx_rrect(r, h / 2, bg);
    text_draw_vc(label, x + 16, y + h / 2, FS_S, FW_REGULAR, fg, 0);
    if (res.focused)
        draw_focus_ring(r, h / 2);
    if (out_w)
        *out_w = w;
    return res.clicked;
}

void draw_back_button(view *v, float x, float y)
{
    (void)v;
    float r = 20;
    rectf hit = rect(x - 4, y - 4, r * 2 + 8, r * 2 + 8);
    ui_res res = ui_item(ui_id("nav.back"), hit, 0);
    gfx_circle(x + r, y + r, r, rgba(0x000000, res.pressed ? 200 : 150));
    gfx_icon(ICON_BACK, x + r - 1, y + r, 22, rgb(0xFFFFFF));
    if (res.focused)
        draw_focus_ring(rect(x, y, r * 2, r * 2), r);
    if (res.clicked) app_go_back();
    if (A.depth[A.tab] < A.extent[A.tab]) {
        if (pill_button(ui_id("nav.forward"),"Forward",x+48,y,false,NULL)) app_go_forward();
    }
}

void draw_loading(rectf area)
{
    gfx_spinner(area.x + area.w / 2, area.y + area.h / 2, 18, A.time, rgb(C_SUBTEXT));
}

bool draw_error(rectf area, const char *msg)
{
    float cy = area.y + area.h / 2;
    text_draw_aligned("Couldn't load this", area.x + area.w / 2, cy - 62, FS_L, FW_BOLD,
                      rgb(C_TEXT), area.w - 40, ALIGN_CENTER);
    text_draw_aligned(msg && *msg ? msg : "Check your connection and try again.", area.x + area.w / 2,
                      cy - 30, FS_S, FW_REGULAR, rgb(C_SUBTEXT), area.w - 40, ALIGN_CENTER);
    float w = text_width("Try again", FS_S, FW_REGULAR, 0) + 32.0f;
    return pill_button(ui_id("error.retry"), "Try again", area.x + (area.w - w) / 2, cy + 6, false, NULL);
}
