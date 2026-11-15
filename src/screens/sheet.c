/* Item context menu (long-press / Square / "..." button). */

#include <stdio.h>
#include <string.h>
#include <time.h>

#include "../app.h"
#include "../gfx/text.h"
#include "../theme.h"
#include "../prefs.h"

typedef enum act { ACT_PLAY, ACT_SAVE, ACT_QUEUE, ACT_ALBUM, ACT_ARTIST, ACT_ADD_PLAYLIST, ACT_RENAME, ACT_REMOVE, ACT_UP, ACT_DOWN, ACT_QREMOVE, ACT_QUP, ACT_QDOWN, ACT_PIN, ACT_CANCEL } act;

typedef struct option { act a; icon_id icon; const char *label; } option;

static int build_options(const vam_item *it, option *o)
{
    int n = 0;
    if(A.sheet.confirm_remove){
        o[n++]=(option){ACT_REMOVE,ICON_MORE,"Confirm removal from playlist"};
        o[n++]=(option){ACT_CANCEL,ICON_CLOSE,"Cancel"};return n;
    }
    bool track = it->kind == VAM_KIND_TRACK || it->kind == VAM_KIND_EPISODE;
    if (track) {
        o[n++] = (option){ ACT_SAVE, it->is_saved ? ICON_HEART : ICON_HEART_OUTLINE,
                           it->is_saved ? "Remove from Liked Songs" : "Add to Liked Songs" };
        o[n++] = (option){ ACT_QUEUE, ICON_QUEUE, "Add to queue" };
        if(it->kind==VAM_KIND_TRACK) o[n++] = (option){ ACT_ADD_PLAYLIST, ICON_LIBRARY, "Add to playlist" };
        model_page *active=app_view()->page;
        bool owned=active&&active->can_edit&&!strcmp(active->uri,A.sheet.context_uri);
        model_page *lib=A.stacks[TAB_LIBRARY][0].page;
        if((!active||!active->is_stale)&&lib&&!lib->is_stale&&lib->section_count)for(uint32_t i=0;i<lib->sections[0].count;i++)
            if(!strcmp(lib->sections[0].items[i].uri,A.sheet.context_uri))owned=lib->sections[0].items[i].is_owned!=0;
        if(owned && A.sheet.revision[0] && !strncmp(A.sheet.context_uri,"spotify:playlist:",17)) {
            o[n++]=(option){ACT_REMOVE,ICON_MORE,"Remove this entry"};
            o[n++]=(option){ACT_UP,ICON_PREV,"Move entry up"};
            o[n++]=(option){ACT_DOWN,ICON_NEXT,"Move entry down"};
        }
        if((it->reserved&1) && A.sheet.revision[0]){
            o[n++]=(option){ACT_QREMOVE,ICON_MORE,"Remove from queue"};
            o[n++]=(option){ACT_QUP,ICON_PREV,"Move queued song up"};
            o[n++]=(option){ACT_QDOWN,ICON_NEXT,"Move queued song down"};
        }
        if (it->album_uri[0])
            o[n++] = (option){ ACT_ALBUM, ICON_DISC, "Go to album" };
        if (it->artist_uri[0])
            o[n++] = (option){ ACT_ARTIST, ICON_PERSON, "Go to artist" };
        return n;
    }
    if(it->kind==VAM_KIND_FOLDER||it->kind==VAM_KIND_PLAYLIST)
        o[n++]=(option){ACT_PIN,ICON_LIBRARY,(prefs_item_flags(A.auth.username,it->uri)&8)?"Unpin on this Vita":"Pin on this Vita"};
    if(it->kind==VAM_KIND_FOLDER)return n;
    o[n++] = (option){ ACT_PLAY, ICON_PLAY, "Play" };
    if(it->kind==VAM_KIND_PLAYLIST && it->is_owned && !(it->reserved&8)) o[n++] = (option){ACT_RENAME,ICON_MORE,"Rename playlist"};
    if (it->kind == VAM_KIND_ARTIST)
        o[n++] = (option){ ACT_SAVE, ICON_PERSON, it->is_saved ? "Unfollow" : "Follow" };
    else if (it->kind != VAM_KIND_COLLECTION)
        o[n++] = (option){ ACT_SAVE, it->is_saved ? ICON_HEART : ICON_HEART_OUTLINE,
                           it->is_saved ? "Remove from Your Library" : "Save to Your Library" };
    if (it->artist_uri[0])
        o[n++] = (option){ ACT_ARTIST, ICON_PERSON, "Go to artist" };
    return n;
}

static void run(act a, const vam_item *it)
{
    char uri[VAM_URI_MAX];
    switch (a) {
    case ACT_PIN:
        prefs_item_toggle(A.auth.username,it->uri,8);app_close_sheet();app_toast("Updated pins on this Vita");break;
    case ACT_CANCEL:A.sheet.confirm_remove=false;break;
    case ACT_REMOVE:
        if(!A.sheet.confirm_remove){A.sheet.confirm_remove=true;A.sheet.scroll.pos=0;}
        else {app_submit_entry_edit(1);app_close_sheet();}break;
    case ACT_UP:app_submit_entry_edit(2);app_close_sheet();break;
    case ACT_DOWN:app_submit_entry_edit(3);app_close_sheet();break;
    case ACT_QREMOVE:app_submit_queue_edit(A.sheet.revision,A.sheet.index,1);app_close_sheet();break;
    case ACT_QUP:app_submit_queue_edit(A.sheet.revision,A.sheet.index,2);app_close_sheet();break;
    case ACT_QDOWN:app_submit_queue_edit(A.sheet.revision,A.sheet.index,3);app_close_sheet();break;
    case ACT_ADD_PLAYLIST: A.sheet.playlist_picker=true; A.sheet.scroll.pos=0; break;
    case ACT_RENAME: app_close_sheet(); app_begin_playlist_name(2,it->uri,it->title); break;
    case ACT_PLAY:
        app_play_item(it, NULL, 0);
        app_close_sheet();
        break;
    case ACT_SAVE:
        app_toggle_saved(it->uri, it->is_saved);
        app_close_sheet();
        break;
    case ACT_QUEUE:
        if (vam_add_to_queue(it->uri) == VAM_OK)
            app_toast("Added to queue");
        else
            app_toast("Couldn't add to queue.");
        app_close_sheet();
        break;
    case ACT_ALBUM:
        snprintf(uri, sizeof(uri), "%s", it->album_uri);
        app_open_uri(uri);
        break;
    case ACT_ARTIST:
        snprintf(uri, sizeof(uri), "%s", it->artist_uri);
        app_open_uri(uri);
        break;
    }
}

void screen_sheet(void)
{
    sheet *s = &A.sheet;
    float t = s->anim;
    float e = 1 - (1 - t) * (1 - t);
    gfx_rect(rect(0, 0, SCREEN_W, SCREEN_H), rgba(0x000000, (uint8_t)(160 * e)));
    if (ui_item(ui_id("sheet.dismiss"), rect(0, 0, SCREEN_W, SCREEN_H), UI_F_NOFOCUS).clicked)
        app_close_sheet();

    if(s->playlist_picker) {
        rectf panel=rect(180,50,600,450);gfx_rrect(panel,12,rgb(C_SURFACE_HI));
        text_draw("Add to your playlist",200,64,FS_L,FW_BOLD,rgb(C_TEXT),500);
        rectf vp=rect(188,108,584,380);gfx_push_clip(vp);ui_push_clip(vp);
        model_page *lib=A.stacks[TAB_LIBRARY][0].page;
        model_section *entries=lib && lib->section_count?&lib->sections[0]:NULL;
        int count=0;if(entries)for(uint32_t i=0;i<entries->count;i++)if(entries->items[i].is_owned)count++;
        ui_scroll_begin(&s->scroll,vp,count*52.0f,AXIS_Y,true);float y=vp.y-s->scroll.pos;
        if(entries)for(uint32_t i=0;i<entries->count;i++) {
            vam_item *it=&entries->items[i];if(!it->is_owned)continue;
            if(pill_button(ui_idx(ui_id("playlist.pick"),i),it->title,200,y,false,NULL)) {
                app_submit_playlist_edit(3,it->uri,s->item.uri);app_close_sheet();
            }
            y+=52;
        }
        ui_scroll_end();ui_pop_clip();gfx_pop_clip();
        if(!count) text_draw("No owned playlists loaded. Open Your Library first.",200,120,FS_S,FW_REGULAR,rgb(C_SUBTEXT),550);
        return;
    }
    option opts[12];
    int n = build_options(&s->item, opts);
    bool track=s->item.kind==VAM_KIND_TRACK;
    const float row_h = 58, head_h = track?148:84, w = 560;
    float h = head_h + n * row_h + 16;
    if(h>SCREEN_H-48)h=SCREEN_H-48;
    float x = (SCREEN_W - w) / 2, y = SCREEN_H - h * e;
    rectf panel = rect(x, y, w, h + 20);
    ui_item(ui_id("sheet.panel"), panel, UI_F_NOFOCUS);  /* taps on the panel don't dismiss */
    gfx_rrect(panel, 14, rgb(C_SURFACE_HI));

    float rad = s->item.kind == VAM_KIND_ARTIST ? 26 : 4;
    draw_cover(s->item.image_url, rect(x + 18, y + 16, 52, 52), rad, s->item.kind, s->item.title);
    text_draw(s->item.title, x + 84, y + 17, FS_M, FW_BOLD, rgb(C_TEXT), w - 100);
    text_draw(s->item.subtitle, x + 84, y + 44, FS_S, FW_REGULAR, rgb(C_SUBTEXT), w - 100);
    if(track) {
        char line[256],added[32]="Unknown added date";
        uint32_t d=s->item.release_date;
        snprintf(line,sizeof(line),"%s%s%u",s->item.album_title[0]?s->item.album_title:"Unknown album",d?" - released ":"",d?d/10000:0);
        if(!d)snprintf(line,sizeof(line),"%s - release date unknown",s->item.album_title[0]?s->item.album_title:"Unknown album");
        text_draw(line,x+18,y+82,FS_S,FW_REGULAR,rgb(C_SUBTEXT),w-36);
        if(s->item.added_at>0) {
            time_t stamp=(time_t)(s->item.added_at>100000000000LL?s->item.added_at/1000:s->item.added_at);
            struct tm *tm=gmtime(&stamp);
            if(tm)strftime(added,sizeof(added),"Added %Y-%m-%d",tm);
        }
        text_draw(added,x+18,y+110,FS_S,FW_REGULAR,rgb(C_SUBTEXT),w-36);
    }
    gfx_rect(rect(x, y + head_h - 1, w, 1), rgb(C_DIVIDER));

    rectf options=rect(x+8,y+head_h+6,w-16,h-head_h-12);
    gfx_push_clip(options);ui_push_clip(options);
    ui_scroll_begin(&s->scroll,options,n*row_h,AXIS_Y,true);
    for (int i = 0; i < n; i++) {
        rectf r = rect(x + 8, y + head_h + 6 + i * row_h-s->scroll.pos, w - 16, row_h);
        ui_res res = ui_item(ui_idx(ui_id("sheet.opt"), i), r, 0);
        if (res.pressed || res.focused)
            gfx_rrect(r, 6, rgb(C_SURFACE_HI2));
        bool green = opts[i].a == ACT_SAVE && s->item.is_saved;
        gfx_icon(opts[i].icon, r.x + 26, r.y + r.h / 2, 24, green ? rgb(C_ACCENT) : rgb(C_SUBTEXT));
        text_draw_vc(opts[i].label, r.x + 60, r.y + r.h / 2, FS_M, FW_REGULAR, rgb(C_TEXT), r.w - 70);
        if (res.focused)
            draw_focus_ring(r, 6);
        if (res.clicked && s->open)
            run(opts[i].a, &s->item);
    }
    ui_scroll_end();ui_pop_clip();gfx_pop_clip();

}
