/* Album / playlist / artist / Liked Songs / queue detail page. */

#include <stdio.h>
#include <string.h>

#include "../app.h"
#include "../gfx/images.h"
#include "../gfx/text.h"
#include "../theme.h"

#define COVER 128
#define HEADER_TOP 44

static vam_item page_as_item(const model_page *p)
{
    vam_item it;
    memset(&it, 0, sizeof(it));
    it.kind = p->kind;
    snprintf(it.uri, sizeof(it.uri), "%s", p->uri);
    snprintf(it.title, sizeof(it.title), "%s", p->title);
    snprintf(it.subtitle, sizeof(it.subtitle), "%s", p->subtitle);
    snprintf(it.image_url, sizeof(it.image_url), "%s", p->image_url);
    it.is_saved = p->is_saved;
    it.is_owned = p->can_edit;
    model_page *lib=A.stacks[TAB_LIBRARY][0].page;
    if(!p->is_stale && lib && !lib->is_stale && lib->section_count) for(uint32_t i=0;i<lib->sections[0].count;i++)
        if(!strcmp(lib->sections[0].items[i].uri,p->uri)) it.is_owned=lib->sections[0].items[i].is_owned;
    it.is_playable = 1;
    return it;
}

static bool can_save(int kind)
{
    return kind == VAM_KIND_ALBUM || kind == VAM_KIND_PLAYLIST || kind == VAM_KIND_ARTIST ||
           kind == VAM_KIND_SHOW;
}

static void play_page(const model_page *p)
{
    view *v=app_view();
    if((p->kind==VAM_KIND_PLAYLIST || p->kind==VAM_KIND_COLLECTION) &&
        (v->sort!=SORT_CUSTOM || v->descending) && p->section_count && p->sections[0].count) {
        const uint32_t *order=app_view_order(v);
        app_play_ordered(v,order?order[0]:0);
        return;
    }
    if (app_is_current_context(p->uri)) {
        if (app_is_playing())
            vam_pause();
        else
            vam_resume();
        return;
    }
    vam_item it = page_as_item(p);
    app_play_item(&it, NULL, 0);
}

static void action_row(view *v, float x, float y, color avg)
{
    (void)avg;
    model_page *p = v->page;
    bool playing_here = app_is_current_context(p->uri) && app_is_playing();
    if (round_button(ui_id("page.play"), playing_here ? ICON_PAUSE : ICON_PLAY, x + 28, y + 28, 28,
                     rgb(C_ACCENT), rgb(0x000000)))
        play_page(p);
    float cx = x + 88;
    color sh = A.player.shuffle ? rgb(C_ACCENT) : rgb(C_SUBTEXT);
    if (icon_button(ui_id("page.shuffle"), ICON_SHUFFLE, cx, y + 28, 26, 52, sh))
        vam_set_shuffle(!A.player.shuffle);
    if (A.player.shuffle)
        gfx_circle(cx, y + 48, 2, rgb(C_ACCENT));
    cx += 56;
    if (p->kind == VAM_KIND_ARTIST) {
        float w = 0;
        if (pill_button(ui_id("page.follow"), p->is_saved ? "Following" : "Follow", cx - 16, y + 8, false, &w))
            app_toggle_saved(p->uri, p->is_saved);
        cx += w + 16;
    } else if (can_save(p->kind)) {
        if (icon_button(ui_id("page.save"), p->is_saved ? ICON_HEART : ICON_HEART_OUTLINE, cx, y + 28, 26, 52,
                        p->is_saved ? rgb(C_ACCENT) : rgb(C_SUBTEXT)))
            app_toggle_saved(p->uri, p->is_saved);
        cx += 56;
    }
    if (p->kind != VAM_KIND_QUEUE && p->kind != VAM_KIND_COLLECTION) {
        if (icon_button(ui_id("page.more"), ICON_MORE, cx, y + 28, 24, 52, rgb(C_SUBTEXT))) {
            vam_item it = page_as_item(p);
            app_open_sheet(&it, NULL, 0);
        }
    }
}

static int title_size(const char *title, float w)
{
    if (text_width(title, FS_HUGE, FW_BOLD, 0) <= w) return FS_HUGE;
    if (text_width(title, FS_XXL, FW_BOLD, 0) <= w) return FS_XXL;
    return FS_XL;
}

static float draw_header(view *v, float y0, color avg)
{
    model_page *p = v->page;
    bool artist = p->kind == VAM_KIND_ARTIST;
    float cx = CONTENT_X + PAD;
    float top = y0 + HEADER_TOP;
    rectf cover = rect(cx, top, COVER, COVER);
    if (!artist)
        gfx_shadow(cover, 4, 14, 160);
    draw_cover(p->image_url, cover, artist ? COVER / 2.0f : 4, p->kind, p->title);

    float tx = cx + COVER + 24;
    float tw = CONTENT_X + CONTENT_W - PAD - tx;
    const char *label = p->kind == VAM_KIND_COLLECTION ? "Playlist" : kind_label(p->kind);
    float ty = top + 18;
    text_draw(label, tx, ty, FS_S, FW_BOLD, rgb(C_TEXT), tw);
    int ts = title_size(p->title, tw);
    text_draw(p->title, tx, ty + 22, ts, FW_BOLD, rgb(C_TEXT), tw);
    float by = top + COVER - 22;
    if (p->description[0]) {
        float description_y = top + 112;
        float description_h = text_draw_wrapped(p->description, tx, description_y, FS_S, FW_REGULAR,
                                                with_alpha(rgb(C_TEXT), 0.7f), tw, 2);
        by = description_y + description_h + 8;
    }
    text_draw(p->subtitle, tx, by, FS_S, FW_REGULAR, rgb(C_TEXT), tw);
    (void)avg;
    float bottom = by + text_line_height(FS_S) + 18;
    return bottom > top + COVER + 18 ? bottom : top + COVER + 18;
}

static void sticky_bar(view *v, color avg)
{
    float t = (v->scroll.pos - 180) / 60.0f;
    if (t <= 0)
        return;
    if (t > 1) t = 1;
    rectf bar = rect(CONTENT_X, 0, CONTENT_W, 60);
    ui_item(ui_id("page.sticky"), bar, UI_F_NOFOCUS);  /* swallow taps */
    gfx_rect(bar, with_alpha(color_scale(avg, 0.55f), t));
    if (t > 0.5f) {
        float a = (t - 0.5f) * 2;
        gfx_circle(CONTENT_X + 84, 30, 20, with_alpha(rgb(C_ACCENT), a));
        bool playing_here = app_is_current_context(v->page->uri) && app_is_playing();
        gfx_icon(playing_here ? ICON_PAUSE : ICON_PLAY, CONTENT_X + 85, 30, 20, with_alpha(rgb(0x000000), a));
        rectf hit = rect(CONTENT_X + 60, 6, TOUCH_MIN, TOUCH_MIN);
        if (ui_item(ui_id("page.sticky.play"), hit, UI_F_NOFOCUS).clicked)
            play_page(v->page);
        text_draw_vc(v->page->title, CONTENT_X + 116, 30, FS_L, FW_BOLD, with_alpha(rgb(C_TEXT), a),
                     CONTENT_W - 150);
    }
}

void screen_page(view *v)
{
    rectf vp = rect(CONTENT_X, CONTENT_Y, CONTENT_W, CONTENT_H);
    if (!v->page && !v->req && !v->failed)
        app_request_view(v);
    if (!v->page) {
        if (v->failed) {
            if (draw_error(vp, v->error))
                app_request_view(v);
        } else {
            draw_loading(vp);
        }
        draw_back_button(v, CONTENT_X + 14, 12);
        return;
    }
    model_page *p = v->page;

    /* The queue follows the player; refresh it when playback changes. */
    if (p->kind == VAM_KIND_QUEUE && v->player_seq != A.player.seq && !v->req) {
        v->player_seq = A.player.seq;
        app_request_view(v);
    }

    color avg = rgb(0x535353);
    /* The artist header draws its picture round: share that texture. */
    if (p->kind == VAM_KIND_ARTIST)
        images_get_round(p->image_url, &avg);
    else
        images_get(p->image_url, &avg);
    bool queue = p->kind == VAM_KIND_QUEUE;
    bool folder = p->kind == VAM_KIND_FOLDER;
    if (p->kind == VAM_KIND_COLLECTION && !p->image_url[0])
        avg = color_scale(rgb(C_LIKED_TOP), 0.8f);

    ui_scroll_begin(&v->scroll, vp, v->content_h, AXIS_Y, true);
    float y0 = CONTENT_Y - v->scroll.pos;
    if (y0 > 0)
        gfx_rect(rect(CONTENT_X, 0, CONTENT_W, y0), avg);
    gfx_vgradient(rect(CONTENT_X, y0, CONTENT_W, 300), avg, color_scale(avg, 0.35f));
    gfx_vgradient(rect(CONTENT_X, y0 + 300, CONTENT_W, 160), color_scale(avg, 0.35f), rgb(C_BG));

    float y;
    if(folder) {
        text_draw(p->title,CONTENT_X+PAD+52,y0+16,FS_XL,FW_BOLD,rgb(C_TEXT),CONTENT_W-100);y=y0+70;
    } else if (queue) {
        text_draw("Queue", CONTENT_X + PAD + 52, y0 + 16, FS_XL, FW_BOLD, rgb(C_TEXT), CONTENT_W);
        y = y0 + 62;
        if(pill_button(ui_id("queue.clear"),v->confirm_clear?"Confirm clear":"Clear queued songs",CONTENT_X+PAD,y,false,NULL)) {
            if(v->confirm_clear){app_submit_queue_edit(p->revision,0,4);v->confirm_clear=false;}
            else v->confirm_clear=true;
        }
        if(v->confirm_clear && pill_button(ui_id("queue.cancel"),"Cancel",CONTENT_X+PAD+200,y,false,NULL))v->confirm_clear=false;
        y+=52;
    } else {
        y = draw_header(v, y0, avg);
        action_row(v, CONTENT_X + PAD, y, avg);
        y += 70;
    }

    bool sortable = p->kind == VAM_KIND_PLAYLIST || p->kind == VAM_KIND_COLLECTION;
    if (sortable) {
        if(model_has_more(p) && !v->more_failed) app_request_more(v);
        draw_sort_button(v,CONTENT_X+CONTENT_W-PAD-220,y);
        char scope[128];
        uint32_t count=p->section_count?p->sections[0].count:0;
        if(model_has_more(p))snprintf(scope,sizeof(scope),"%s %u / %u songs",v->more_failed?"Couldn't load all songs:":"Loading",count,p->sections[0].total);
        else snprintf(scope,sizeof(scope),"%u songs",count);
        text_draw_vc(scope,CONTENT_X+PAD,y+20,FS_XS,FW_REGULAR,rgb(C_SUBTEXT),CONTENT_W-280);
        y+=48;
        if(A.order_token&&A.order_view==v){
            if(pill_button(ui_id("page.cancel_order"),"Cancel preparing playback",CONTENT_X+PAD,y,false,NULL))app_cancel_order_upload();y+=48;
        }
        if(v->original_order_fallback){
            if(pill_button(ui_id("page.original_order"),"Play original playlist order",CONTENT_X+PAD,y,false,NULL)){
                uint32_t i=v->fallback_index;if(p->section_count&&i<p->sections[0].count){
                    vam_result result=vam_play(p->uri,p->sections[0].items[i].uri,i);
                    app_toast(result==VAM_OK?"Playing in original playlist order":"Couldn't start original order");
                    if(result==VAM_OK)v->original_order_fallback=false;
                }
            }y+=48;
        }
        if(v->more_failed) {
            if(pill_button(ui_id("page.retry.all"),"Retry loading songs",CONTENT_X+PAD,y,false,NULL))app_request_view(v);
            y+=48;
        }
    }

    bool numbered = p->kind == VAM_KIND_ALBUM || p->kind == VAM_KIND_ARTIST;
    sections_ctx c = { v, queue || folder ? NULL : p->uri, numbered, !numbered, vp, CONTENT_X + PAD - ROW_INSET,
                       CONTENT_W - 2 * PAD + 2 * ROW_INSET };
    float before = y;
    y = draw_sections(&c, p, 0, y);
    if (y == before && !v->req)
        text_draw_aligned(queue ? "Your queue is empty" : "Nothing here yet", CONTENT_X + CONTENT_W / 2.0f,
                          y + 30, FS_M, FW_BOLD, rgb(C_SUBTEXT), 0, ALIGN_CENTER);
    v->content_h = y + v->scroll.pos - CONTENT_Y + 24;
    ui_scroll_end();

    if (!queue && !folder)
        sticky_bar(v, avg);
    draw_back_button(v, CONTENT_X + 14, 12);
}
