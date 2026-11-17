#include "app.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "gfx/images.h"
#include "gfx/text.h"
#include "lyrics.h"
#include "notifications.h"
#include "platform/platform.h"
#include "power.h"
#include "prefs.h"
#include "theme.h"

app A;

#define PENDING_SAVES 16
static struct { uint32_t req; char uri[VAM_URI_MAX]; bool prev; bool desired; } g_saves[PENDING_SAVES];
static uint32_t g_focus_before_overlay;
static float g_edit_changed_at = -1;

/* ------------------------------------------------------------------------ */
/* Views                                                                     */
/* ------------------------------------------------------------------------ */

static void view_init(view *v, view_type t, const char *uri)
{
    memset(v, 0, sizeof(*v));
    v->type = t;
    snprintf(v->uri, sizeof(v->uri), "%s", uri ? uri : "");
}

void app_cancel_order_upload(void){
    if(A.order_token)vam_order_cancel(A.order_token);
    A.order_token=0;free(A.order_upload);A.order_upload=NULL;A.order_page=NULL;A.order_view=NULL;
}
static void order_upload_frame(void){
    if(!A.order_token)return;
    view *v=A.order_view;
    if(app_view()!=v||v->page!=A.order_page){app_cancel_order_upload();return;}
    const model_section *s=&v->page->sections[0];const char *uris[VAM_ORDER_CHUNK];uint32_t n=0;
    while(A.order_cursor<s->count&&n<VAM_ORDER_CHUNK){uint32_t i=A.order_upload[A.order_cursor++];if(s->items[i].is_playable)uris[n++]=s->items[i].uri;}
    vam_result result=n?vam_order_append(A.order_token,uris,n):VAM_OK;
    if(result==VAM_OK&&A.order_cursor==s->count){result=vam_order_commit(A.order_token);if(result==VAM_OK){A.order_token=0;app_cancel_order_upload();app_toast("Starting sorted playback...");return;}}
    if(result!=VAM_OK){app_cancel_order_upload();v->original_order_fallback=true;app_toast("Couldn't prepare sorted playback. Original order is available.");}
}

static void view_free(view *v)
{
    if(A.order_view==v)app_cancel_order_upload();
    if (v->req) vam_cancel(v->req);
    if (v->more_req) vam_cancel(v->more_req);
    model_free(v->page);
    free(v->order);
    memset(v, 0, sizeof(*v));
}

static void reset_stacks(void)
{
    A.cache_request=0;
    for (int t = 0; t < TAB_COUNT; t++) {
        for (int d = 0; d < A.extent[t]; d++)
            view_free(&A.stacks[t][d]);
        A.depth[t] = A.extent[t] = 1;
    }
    view_init(&A.stacks[TAB_HOME][0], VIEW_HOME, "vam:home");
    view_init(&A.stacks[TAB_SEARCH][0], VIEW_SEARCH, "");
    view_init(&A.stacks[TAB_LIBRARY][0], VIEW_LIBRARY, "vam:library");
    view_init(&A.stacks[TAB_SETTINGS][0], VIEW_SETTINGS, "vam:settings");
    A.tab = TAB_HOME;
    A.np_open = false;
    A.sheet.open = false;
    memset(&A.playlist_scroll, 0, sizeof(A.playlist_scroll));
    ui_set_focus(0);
    ui_reset_focus_list();
}

view *app_view(void) { return &A.stacks[A.tab][A.depth[A.tab] - 1]; }
int app_depth(void) { return A.depth[A.tab]; }

const uint32_t *app_view_order(view *v)
{
    if (!v->page || !v->page->section_count) return NULL;
    model_section *s = &v->page->sections[0];
    if (v->order_count == s->count && v->order_sort == v->sort &&
        v->order_descending == v->descending && v->order_prefs_revision==prefs_item_revision()) return v->order;
    uint32_t *order = s->count ? malloc(sizeof(*order) * s->count) : NULL;
    if (s->count && (!order || !model_order(s, v->sort, v->descending, order))) {
        free(order);
        return NULL; /* original order remains usable on allocation failure */
    }
    if(v->type==VIEW_LIBRARY && s->count){
        uint32_t *partition=malloc(sizeof(*partition)*s->count);
        if(partition){
            uint32_t n=0;
            for(uint32_t i=0;i<s->count;i++){
                unsigned flags=prefs_item_flags(A.auth.username,s->items[i].uri);
                s->items[i].reserved=(s->items[i].reserved&9)|((flags&8)?2:0)|((flags&16)?4:0);
            }
            for(int pinned=1;pinned>=0;pinned--)for(uint32_t j=0;j<s->count;j++)
                if(!!(s->items[order[j]].reserved&2)==pinned)partition[n++]=order[j];
            memcpy(order,partition,sizeof(*order)*s->count);free(partition);
        }
    }
    free(v->order);
    v->order = order;
    v->order_count = s->count;
    v->order_prefs_revision=prefs_item_revision();
    v->order_sort = v->sort;
    v->order_descending = v->descending;
    return order;
}

void app_request_view(view *v)
{
    if(A.order_view==v)app_cancel_order_upload();
    char uri[VAM_URI_MAX + 256];
    if (v->type == VIEW_SEARCH) {
        if (!v->query[0])
            return;
        snprintf(uri, sizeof(uri), "vam:search:%s", v->query);
    } else {
        snprintf(uri, sizeof(uri), "%s", v->uri);
    }
    if (v->req)
        vam_cancel(v->req);
    if (v->more_req) {
        vam_cancel(v->more_req);
        v->more_req = 0;
    }
    v->failed = false;
    v->more_failed = false;
    v->req = vam_request_page(uri, 0, 50);
    if (!v->req) {
        v->failed = true;
        snprintf(v->error, sizeof(v->error), "Couldn't reach the player service.");
    }
}

void app_request_more(view *v)
{
    if (v->more_req || v->req || !model_has_more(v->page))
        return;
    v->more_failed = false;
    uint32_t count=v->page->sections[0].count, limit=50;
    v->more_req = vam_request_page(v->page->uri, count, limit);
    if (!v->more_req) v->more_failed = true;
}

static view *find_view_by_req(uint32_t id, bool *is_more)
{
    for (int t = 0; t < TAB_COUNT; t++) {
        for (int d = 0; d < A.extent[t]; d++) {
            view *v = &A.stacks[t][d];
            if (v->req == id) { *is_more = false; return v; }
            if (v->more_req == id) { *is_more = true; return v; }
        }
    }
    return NULL;
}

static void apply_saved(const char *uri, bool saved)
{
    for (int t = 0; t < TAB_COUNT; t++)
        for (int d = 0; d < A.extent[t]; d++)
            model_set_saved(A.stacks[t][d].page, uri, saved);
    if (A.player.has_track && strcmp(A.player.track.uri, uri) == 0)
        A.player.track.is_saved = saved;
    if (strcmp(A.sheet.item.uri, uri) == 0)
        A.sheet.item.is_saved = saved;
}

/* ------------------------------------------------------------------------ */
/* Navigation (deferred to end of frame)                                     */
/* ------------------------------------------------------------------------ */

void app_open_uri(const char *uri)
{
    if (!uri || !*uri)
        return;
    A.nav = NAV_PUSH;
    snprintf(A.nav_uri, sizeof(A.nav_uri), "%s", uri);
}

void app_go_back(void) { A.nav = NAV_POP; }
void app_go_forward(void) { A.nav = NAV_FORWARD; }

void app_switch_tab(int tab)
{
    if (tab < 0 || tab >= TAB_COUNT) return;
    A.nav = NAV_TAB;
    A.nav_tab = tab;
}

static void end_typing(bool commit);
static void close_overlays(void)
{
    if(A.typing) end_typing(false);
    A.np_open = false;
    A.sheet.open = false;
}

static void apply_nav(void)
{
    nav_kind k = A.nav;
    A.nav = NAV_NONE;
    if (k == NAV_NONE)
        return;
    view *cur = app_view();
    cur->saved_focus = ui_focus();
    int *depth = &A.depth[A.tab];

    if (k == NAV_PUSH) {
        close_overlays();
        while (A.extent[A.tab] > *depth) view_free(&A.stacks[A.tab][--A.extent[A.tab]]);
        if (strcmp(cur->uri, A.nav_uri) == 0 && cur->type == VIEW_PAGE)
            return;
        if (*depth >= MAX_DEPTH) {
            /* Drop the oldest non-root view to make room. */
            view_free(&A.stacks[A.tab][1]);
            memmove(&A.stacks[A.tab][1], &A.stacks[A.tab][2], sizeof(view) * (MAX_DEPTH - 2));
            memset(&A.stacks[A.tab][MAX_DEPTH - 1], 0, sizeof(view));
            (*depth)--;
        }
        view *v = &A.stacks[A.tab][(*depth)++];
        view_init(v, VIEW_PAGE, A.nav_uri);
        A.extent[A.tab] = *depth;
        ui_set_focus(0);
    } else if (k == NAV_POP) {
        if (*depth <= 1)
            return;
        (*depth)--;
        ui_set_focus(app_view()->saved_focus);
    } else if (k == NAV_FORWARD) {
        if (*depth >= A.extent[A.tab]) return;
        (*depth)++;
        ui_set_focus(app_view()->saved_focus);
    } else if (k == NAV_TAB) {
        close_overlays();
        if (A.nav_tab == A.tab) {
            /* Tapping the active tab returns to its root. */
            *depth = 1; /* retain forward history */
            ui_scroll_to(&app_view()->scroll, 0);
            if (A.tab == TAB_SEARCH)
                app_begin_typing();
        } else {
            A.tab = A.nav_tab;
        }
        ui_set_focus(app_view()->saved_focus);
    }
    ui_reset_focus_list();
}

/* ------------------------------------------------------------------------ */
/* Player helpers                                                            */
/* ------------------------------------------------------------------------ */

bool app_is_playing(void)
{
    return A.player.has_track &&
           (A.player.playback == VAM_PLAYBACK_PLAYING || A.player.playback == VAM_PLAYBACK_BUFFERING);
}

uint32_t app_position_ms(void)
{
    if (!A.player.has_track)
        return 0;
    uint64_t pos = A.player.position_ms;
    if (A.player.playback == VAM_PLAYBACK_PLAYING) {
        uint64_t now = vam_now_ms();
        if (now > A.player.position_timestamp_ms)
            pos += now - A.player.position_timestamp_ms;
    }
    if (A.player.track.duration_ms && pos > A.player.track.duration_ms)
        pos = A.player.track.duration_ms;
    return (uint32_t)pos;
}

bool app_is_current_track(const char *uri)
{
    return A.player.has_track && uri && *uri && strcmp(A.player.track.uri, uri) == 0;
}

bool app_is_current_context(const char *uri)
{
    return A.player.has_track && uri && *uri && strcmp(A.player.context_uri, uri) == 0;
}

void app_toast(const char *msg)
{
    if (!msg || !*msg)
        return;
    snprintf(A.toast, sizeof(A.toast), "%s", msg);
    A.toast_until = A.time + 2.6f;
}

static void check(vam_result r, const char *what)
{
    if (r == VAM_OK)
        return;
    char buf[160];
    if (r == VAM_ERR_NOT_PREMIUM)
        snprintf(buf, sizeof(buf), "Spotify Premium is required to %s.", what);
    else if (r == VAM_ERR_NETWORK)
        snprintf(buf, sizeof(buf), "No connection. Couldn't %s.", what);
    else
        snprintf(buf, sizeof(buf), "Couldn't %s.", what);
    app_toast(buf);
}

void app_play_item(const vam_item *it, const char *context_uri, uint32_t index)
{
    if (!it)
        return;
    if (!it->is_playable && (it->kind == VAM_KIND_TRACK || it->kind == VAM_KIND_EPISODE)) {
        app_toast("This song isn't available.");
        return;
    }
    switch (it->kind) {
    case VAM_KIND_TRACK:
    case VAM_KIND_EPISODE:
        if (context_uri && app_view()->page && strcmp(context_uri,app_view()->page->uri)==0 &&
            (app_view()->page->kind==VAM_KIND_PLAYLIST || app_view()->page->kind==VAM_KIND_COLLECTION) &&
            (app_view()->sort!=SORT_CUSTOM || app_view()->descending)) {
            app_play_ordered(app_view(),index);
        } else check(vam_play(context_uri, it->uri, index), "play this");
        break;
    default:
        check(vam_play(it->uri, NULL, 0), "play this");
        break;
    }
}

void app_toggle_saved(const char *uri, bool currently_saved)
{
    int slot = -1;
    for (int i = 0; i < PENDING_SAVES; i++) {
        if (g_saves[i].req && strcmp(g_saves[i].uri, uri) == 0) return;
        if (!g_saves[i].req && slot < 0) slot = i;
    }
    if (slot < 0) { app_toast("Library is busy. Try again shortly."); return; }
    uint32_t req = vam_set_saved(uri, !currently_saved);
    if (!req) {
        app_toast("Couldn't update your library.");
        return;
    }
    for (int i = 0; i < PENDING_SAVES; i++) {
        if (!g_saves[i].req) {
            g_saves[i].req = req;
            g_saves[i].prev = currently_saved;
            g_saves[i].desired = !currently_saved;
            snprintf(g_saves[i].uri, sizeof(g_saves[i].uri), "%s", uri);
            break;
        }
    }
    /* Optimistic: flip now, revert if the request fails. */
    apply_saved(uri, !currently_saved);
    app_toast("Updating Spotify library...");
}

void app_open_sheet(const vam_item *item, const char *context_uri, uint32_t index)
{
    if (!A.sheet.open && !A.np_open)
        g_focus_before_overlay = ui_focus();
    A.sheet.open = true;
    A.sheet.playlist_picker = false;
    A.sheet.confirm_remove = false;
    A.sheet.revision[0]=0;
    if(app_view()->page && ((context_uri && !strcmp(context_uri,app_view()->page->uri)) ||
        (app_view()->page->kind==VAM_KIND_QUEUE && (item->reserved&1))))
        snprintf(A.sheet.revision,sizeof(A.sheet.revision),"%s",app_view()->page->revision);
    A.sheet.scroll.pos = 0;
    A.sheet.item = *item;
    snprintf(A.sheet.context_uri, sizeof(A.sheet.context_uri), "%s", context_uri ? context_uri : "");
    A.sheet.index = (item->reserved&1)?item->original_index:index;
    ui_set_focus(0);
    ui_reset_focus_list();
}

void app_close_sheet(void)
{
    A.sheet.open = false;
    ui_set_focus(A.np_open ? 0 : g_focus_before_overlay);
    ui_reset_focus_list();
}

void app_set_now_playing(bool open)
{
    if (open == A.np_open)
        return;
    if (open && !A.player.has_track)
        return;
    if (open)
        g_focus_before_overlay = ui_focus();
    A.np_open = open;
    ui_set_focus(open ? 0 : g_focus_before_overlay);
    ui_reset_focus_list();
}

/* ------------------------------------------------------------------------ */
/* Search text entry                                                         */
/* ------------------------------------------------------------------------ */

static void load_recent(void)
{
    char path[512];
    snprintf(path, sizeof(path), "%srecent_searches.txt", platform_data_dir());
    FILE *f = fopen(path, "r");
    if (!f)
        return;
    char line[256];
    int n = 0;
    while (n < RECENT_SEARCHES && fgets(line, sizeof(line), f)) {
        line[strcspn(line, "\r\n")] = 0;
        if (line[0])
            snprintf(A.recent[n++], sizeof(A.recent[0]), "%s", line);
    }
    fclose(f);
}

static void save_recent(void)
{
    char path[512];
    snprintf(path, sizeof(path), "%srecent_searches.txt", platform_data_dir());
    FILE *f = fopen(path, "w");
    if (!f)
        return;
    for (int i = 0; i < RECENT_SEARCHES; i++)
        if (A.recent[i][0])
            fprintf(f, "%s\n", A.recent[i]);
    fclose(f);
}

static void add_recent(const char *q)
{
    int found = RECENT_SEARCHES - 1;
    for (int i = 0; i < RECENT_SEARCHES; i++) {
        if (strcmp(A.recent[i], q) == 0) {
            found = i;
            break;
        }
    }
    for (int i = found; i > 0; i--)
        memcpy(A.recent[i], A.recent[i - 1], sizeof(A.recent[0]));
    snprintf(A.recent[0], sizeof(A.recent[0]), "%s", q);
    save_recent();
}

static void end_typing(bool commit);

static void run_search(const char *q, bool remember)
{
    view *root = &A.stacks[TAB_SEARCH][0];
    while (A.extent[TAB_SEARCH] > 1)
        view_free(&A.stacks[TAB_SEARCH][--A.extent[TAB_SEARCH]]);
    A.depth[TAB_SEARCH] = 1;
    if (strcmp(root->query, q) == 0 && root->page)
        return;
    snprintf(root->query, sizeof(root->query), "%s", q);
    model_free(root->page);
    root->page = NULL;
    root->scroll.pos = 0;
    for (int i = 0; i < MAX_SHELVES; i++)
        root->shelves[i].pos = 0;
    if (q[0]) {
        app_request_view(root);
        if (remember)
            add_recent(q);
    }
}

void app_set_search(const char *query)
{
    if (A.typing)
        end_typing(false);
    run_search(query, true);
}

bool app_use_touch_keyboard(void)
{
    const char *forced=SDL_getenv("VAM_TOUCH_KEYBOARD");
    return platform_is_vita()||(forced&&forced[0]=='1');
}
void app_begin_typing(void)
{
    if(A.typing)return;
    A.keyboard_shift=false;A.keyboard_symbols=false;
    A.typing = true;
    snprintf(A.edit, sizeof(A.edit), "%s", A.filter_typing ? app_view()->filter_query : A.stacks[TAB_SEARCH][0].query);
    g_edit_changed_at = -1;
    if(!app_use_touch_keyboard()) { SDL_StartTextInput();ui_set_text_mode(true); }
    else {ui_set_focus(0);ui_reset_focus_list();}
}

static void end_typing(bool commit)
{
    A.typing = false;
    if(!app_use_touch_keyboard())SDL_StopTextInput();
    ui_set_text_mode(false);
    if (A.name_operation) {
        int operation=A.name_operation; A.name_operation=0;
        if(commit && A.edit[0]) app_submit_playlist_edit(operation,A.edit_target,A.edit);
        return;
    }
    if (A.filter_typing) {
        if (commit) {
            snprintf(app_view()->filter_query,sizeof(app_view()->filter_query),"%s",A.edit);
            app_view()->scroll.pos = 0;
        }
        A.filter_typing = false;
        return;
    }
    if (commit) {
        /* Trim surrounding spaces. */
        char *s = A.edit;
        while (*s == ' ') s++;
        size_t n = strlen(s);
        while (n > 0 && s[n - 1] == ' ') s[--n] = 0;
        run_search(s, true);
    }
}

static void utf8_pop(char *s)
{
    size_t n = strlen(s);
    while (n > 0) {
        n--;
        if (((unsigned char)s[n] & 0xC0) != 0x80)
            break;
    }
    s[n] = 0;
}

/* ------------------------------------------------------------------------ */
/* Events                                                                    */
/* ------------------------------------------------------------------------ */

void app_handle_sdl_event(const SDL_Event *e)
{
    if (e->type == SDL_QUIT) {
        A.quit = true;
        return;
    }
    if (power_handle_event(e))
        return;
    if (A.typing && e->type == SDL_TEXTINPUT) {
        if (strlen(A.edit) + strlen(e->text.text) < sizeof(A.edit) - 1) {
            strcat(A.edit, e->text.text);
            g_edit_changed_at = A.time;
        }
        return;
    }
    if (A.typing && !app_use_touch_keyboard() && e->type == SDL_KEYDOWN) {
        switch (e->key.keysym.sym) {
        case SDLK_BACKSPACE:
            utf8_pop(A.edit);
            g_edit_changed_at = A.time;
            break;
        case SDLK_RETURN:
        case SDLK_KP_ENTER:
            end_typing(true);
            break;
        case SDLK_ESCAPE:
            end_typing(false);
            break;
        default:
            break;
        }
        return;
    }
    ui_handle_event(e);
}

static void handle_vam_event(vam_event *ev)
{
    switch (ev->type) {
    case VAM_EV_AUTH_CHANGED: {
        int before = A.auth.status;
        vam_get_auth_state(&A.auth);
        if (before == VAM_AUTH_LOGGED_IN && A.auth.status != VAM_AUTH_LOGGED_IN) {
            for (int i = 0; i < PENDING_SAVES; i++)
                if (g_saves[i].req) vam_cancel(g_saves[i].req);
            memset(g_saves, 0, sizeof(g_saves));
            if(A.edit_request)vam_cancel(A.edit_request);
            A.edit_request=0; A.name_operation=0;
            reset_stacks();
            lyrics_reset();
        }
        break;
    }
    case VAM_EV_PLAYER_CHANGED:
        vam_get_player_state(&A.player);
        A.have_player = true;
        break;
    case VAM_EV_PAGE: {
        bool more = false;
        view *v = find_view_by_req(ev->request_id, &more);
        if (!v)
            break;
        if (more) {
            v->more_req = 0;
            v->more_failed = ev->result != VAM_OK || !ev->page;
            if (ev->result == VAM_OK && ev->page) {
                if (!model_append(v->page,ev->page)) {
                    v->more_failed = true;
                    v->collect_all = false;
                    app_toast("Playlist changed or couldn't append. Retry loading songs.");
                }
            }
            break;
        }
        v->req = 0;
        if (ev->result == VAM_OK && ev->page) {
            model_free(v->page);
            free(v->order);
            v->order = NULL;
            v->order_count = 0;
            v->page = model_from_vam(ev->page);
            v->failed = v->page == NULL;
            v->collect_all = false;
        } else if (!v->page) {
            v->failed = true;
            snprintf(v->error, sizeof(v->error), "%s",
                     ev->message[0] ? ev->message : "Something went wrong.");
        }
        break;
    }
    case VAM_EV_LIBRARY_CHANGED:
        for (int t=0;t<TAB_COUNT;t++) for(int d=0;d<A.extent[t];d++)
            A.stacks[t][d].saved_dirty = true;
        break;
    case VAM_EV_IMAGE:
        images_on_event(ev);
        break;
    case VAM_EV_LYRICS:
        lyrics_on_event(ev);
        break;
    case VAM_EV_REQUEST_DONE:
        if(A.cache_request && ev->request_id==A.cache_request){A.cache_request=0;app_toast(ev->result==VAM_OK?"Local browse cache cleared":"Couldn't clear browse cache");break;}

        if(A.edit_request && ev->request_id==A.edit_request) {
            A.edit_request=0;
            if(ev->result==VAM_OK) {
                app_toast(A.edit_queue?"Queue updated":"Playlist updated on Spotify");
                for(int t=0;t<TAB_COUNT;t++)for(int d=0;d<A.extent[t];d++) A.stacks[t][d].saved_dirty=true;
            } else app_toast(ev->message[0]?ev->message:"Playlist edit failed.");
            break;
        }
        for (int i = 0; i < PENDING_SAVES; i++) {
            if (g_saves[i].req != ev->request_id)
                continue;
            if (ev->result != VAM_OK) {
                apply_saved(g_saves[i].uri, g_saves[i].prev);
                app_toast(ev->message[0] ? ev->message : "Couldn't update your library.");
            }
            if (ev->result == VAM_OK) {
                apply_saved(g_saves[i].uri,g_saves[i].desired);
                app_toast(g_saves[i].desired ? "Saved to Spotify library" : "Removed from Spotify library");
            }
            g_saves[i].req = 0;
        }
        break;
    case VAM_EV_SAVED_CHANGED:
        apply_saved(ev->uri, ev->saved != 0);
        /* Library contents changed: refresh it in the background. */
        for (int t=0;t<TAB_COUNT;t++) for (int d=0;d<A.extent[t];d++) {
            view *changed = &A.stacks[t][d];
            if (changed->page && !changed->req && (changed->type == VIEW_LIBRARY || changed->page->kind == VAM_KIND_COLLECTION))
                app_request_view(changed);
        }
        break;
    case VAM_EV_ERROR:
    case VAM_EV_TOAST:
        app_toast(ev->message);
        break;
    default:
        break;
    }
}

/* ------------------------------------------------------------------------ */
/* Lifecycle                                                                 */
/* ------------------------------------------------------------------------ */

bool app_init(SDL_Window *w, SDL_Renderer *r)
{
    memset(&A, 0, sizeof(A));
    A.window = w;
    A.renderer = r;
    A.np_color = rgb(0x404040);
    reset_stacks();
    load_recent();
    prefs_load();
    power_init();
    notifications_reset();

    vam_config cfg;
    memset(&cfg, 0, sizeof(cfg));
    cfg.api_version = VAM_API_VERSION;
    cfg.device_name = platform_is_vita() ? "Vitafy PS Vita" : "Vitafy Desktop";
    cfg.data_dir = platform_data_dir();
    cfg.bitrate = prefs_get()->bitrate;
    cfg.audio_cache_mb = (uint32_t)prefs_get()->cache_mb;
    /* App gain is separate from speaker/Bluetooth hardware volume. Start at unity;
       Normalisation turns loud masters down by their ReplayGain; without it
       full-scale tracks clip the Vita's speaker above ~80% volume. */
    cfg.initial_volume = platform_is_vita() ? 65535 : 52000;
    cfg.normalize_volume = 1;
    vam_result res = vam_init(&cfg);
    if(res==VAM_OK)vam_set_metadata_cache_mb((uint32_t)prefs_get()->metadata_mb);
    if (res != VAM_OK)
        SDL_Log("vam_init failed: %d", res);
    vam_log("Vitafy " VAM_APP_VERSION);
    vam_get_auth_state(&A.auth);
    vam_get_player_state(&A.player);
    return true;
}

void app_shutdown(void)
{
    for (int t = 0; t < TAB_COUNT; t++)
        for (int d = 0; d < A.extent[t]; d++)
            view_free(&A.stacks[t][d]);
    prefs_flush();
    vam_shutdown();
}

static void global_buttons(void)
{
    if(A.typing){
        if(ui_btn(BTN_BACK)){ui_consume(BTN_BACK);end_typing(false);}
        return;
    }
    if (ui_btn(BTN_BACK)) {
        ui_consume(BTN_BACK);
        if (A.typing)
            end_typing(false);
        else if (A.sheet.open)
            app_close_sheet();
        else if (app_view()->sort_open) {
            app_view()->sort_open=false;ui_set_focus(app_view()->sort_focus);ui_reset_focus_list();
        }
        else if(A.stacks[TAB_LIBRARY][0].sort_open) {
            view *lib=&A.stacks[TAB_LIBRARY][0];lib->sort_open=false;ui_set_focus(lib->sort_focus);ui_reset_focus_list();
        }
        else if (A.np_open)
            app_set_now_playing(false);
        else if (app_depth() > 1)
            app_go_back();
    }
    if (ui_btn(BTN_START)) {
        ui_consume(BTN_START);
        app_set_now_playing(!A.np_open);
    }
    if (ui_btn(BTN_TRIANGLE)) {
        ui_consume(BTN_TRIANGLE);
        if (A.player.has_track)
            check(app_is_playing() ? vam_pause() : vam_resume(), "change playback");
    }
    if (ui_btn(BTN_L)) {
        ui_consume(BTN_L);
        if (A.player.has_track)
            check(vam_prev(), "skip back");
    }
    if (ui_btn(BTN_R)) {
        ui_consume(BTN_R);
        if (A.player.has_track)
            check(vam_next(), "skip");
    }
    if (ui_btn(BTN_SELECT)) {
        ui_consume(BTN_SELECT);
        app_open_uri("vam:queue");
    }
}

static void draw_toast(void)
{
    if (!A.toast[0] || A.time > A.toast_until)
        return;
    float left = A.toast_until - A.time;
    float a = left < 0.3f ? left / 0.3f : 1;
    int tw = text_width(A.toast, FS_M, FW_BOLD, 600);
    float w = tw + 40.0f, h = 44;
    float x = (SCREEN_W - w) / 2, y = SCREEN_H - PLAYER_H - h - 16;
    gfx_shadow(rect(x, y, w, h), 8, 10, (uint8_t)(120 * a));
    gfx_rrect(rect(x, y, w, h), 8, with_alpha(rgb(0xFFFFFF), a));
    text_draw_vc(A.toast, x + 20, y + h / 2, FS_M, FW_BOLD, with_alpha(rgb(0x111111), a), 600);
}

static void draw_main(void)
{
    bool editor = A.typing && (app_use_touch_keyboard() || A.filter_typing || A.name_operation);
    bool overlay = A.np_open || A.sheet.open || editor || app_view()->sort_open || A.stacks[TAB_LIBRARY][0].sort_open;
    ui_set_enabled(!overlay);

    rectf content = rect(CONTENT_X, CONTENT_Y, CONTENT_W, CONTENT_H);
    gfx_push_clip(content);
    ui_push_clip(content);
    view *v = app_view();v->last_used=A.time;
    if(!v->options_loaded && A.auth.username[0]) {
        prefs_view_load(A.auth.username,v->uri,&v->sort,&v->descending,&v->compact,&v->grid);
        v->stored_sort=v->sort; v->stored_flags=(v->descending?1:0)|(v->compact?2:0)|(v->grid?4:0); v->options_loaded=true;
    }
    if(v->type==VIEW_LIBRARY){v->compact=false;v->grid=false;v->filter_query[0]=0;}
    if(v->type==VIEW_PAGE && v->page && (v->page->kind==VAM_KIND_PLAYLIST || v->page->kind==VAM_KIND_COLLECTION)) {
        v->compact=false;v->grid=false;v->filter_query[0]=0;v->by_you=false;
    }
    if(v->saved_dirty && !v->req) { v->saved_dirty=false; if(v->page) app_request_view(v); }
    switch (v->type) {
    case VIEW_HOME: screen_home(v); break;
    case VIEW_SEARCH: screen_search(v); break;
    case VIEW_LIBRARY: screen_library(v); break;
    case VIEW_SETTINGS: screen_settings(v); break;
    case VIEW_PAGE: screen_page(v); break;
    }
    ui_pop_clip();
    gfx_pop_clip();

    screen_sidebar();
    unsigned flags=(v->descending?1:0)|(v->compact?2:0)|(v->grid?4:0);
    if(v->options_loaded && (v->stored_sort!=v->sort || v->stored_flags!=flags)) {
        prefs_view_save(A.auth.username,v->uri,v->sort,v->descending,v->compact,v->grid);
        v->stored_sort=v->sort; v->stored_flags=flags;
    }
    screen_player_bar();
    view *sort_view=v->sort_open?v:&A.stacks[TAB_LIBRARY][0];
    if(sort_view->sort_open && !A.np_open && !A.sheet.open && !editor) {
        ui_set_enabled(true);draw_sort_menu(sort_view);ui_set_enabled(false);
    }

    float k = A.dt * 9 > 1 ? 1 : A.dt * 9;
    A.np_anim += ((A.np_open ? 1.0f : 0.0f) - A.np_anim) * k;
    if (A.np_open && A.np_anim > 0.995f) A.np_anim = 1;
    if (!A.np_open && A.np_anim < 0.005f) A.np_anim = 0;
    A.sheet.anim += ((A.sheet.open ? 1.0f : 0.0f) - A.sheet.anim) * (A.dt * 14 > 1 ? 1 : A.dt * 14);
    if (!A.sheet.open && A.sheet.anim < 0.01f) A.sheet.anim = 0;

    if (A.np_anim > 0) {
        ui_set_enabled(A.np_open && !A.sheet.open);
        screen_now_playing();
    }
    if (A.sheet.anim > 0) {
        ui_set_enabled(A.sheet.open);
        screen_sheet();
    }
    ui_set_enabled(true);
    if(editor && app_use_touch_keyboard())screen_keyboard();
    else if(editor) {
        gfx_rect(rect(0,0,SCREEN_W,SCREEN_H),rgba(0x000000,190));
        gfx_rrect(rect(150,150,660,240),12,rgb(C_SURFACE_HI));
        text_draw(A.filter_typing?"Filter this view":A.name_operation==1?"New private playlist":"Rename playlist",174,174,FS_XL,FW_BOLD,rgb(C_TEXT),600);
        text_draw(A.edit,174,224,FS_M,FW_REGULAR,rgb(C_TEXT),600);
        if(pill_button(ui_id("editor.clear"),"Clear",174,312,false,NULL)) A.edit[0]=0;
        if(pill_button(ui_id("editor.cancel"),"Cancel",560,312,false,NULL)) end_typing(false);
        if(pill_button(ui_id("editor.done"),"Done",680,312,true,NULL)) end_typing(true);
    }
}

static void trim_page_copies(void){
    for(;;){size_t bytes=0;view *cold=NULL;
        for(int t=0;t<TAB_COUNT;t++)for(int d=0;d<A.extent[t];d++){
            view *v=&A.stacks[t][d];bytes+=model_resident_bytes(v->page);
            if(v->page&&v!=app_view()&&v!=&A.stacks[TAB_LIBRARY][0]&&(!cold||v->last_used<cold->last_used))cold=v;
        }
        if(bytes<=64*1024*1024||!cold)return;
        if(cold->req)vam_cancel(cold->req);if(cold->more_req)vam_cancel(cold->more_req);
        cold->req=cold->more_req=0;model_free(cold->page);cold->page=NULL;free(cold->order);cold->order=NULL;cold->order_count=0;
    }
}

bool app_drawing(void) { return platform_graphics_allowed() && !(platform_is_vita() && power_screen() == SCREEN_OFF); }

void app_frame(float dt)
{
    A.dt = dt;
    A.time += dt;

    vam_event ev;
    for (int i = 0; i < 64 && vam_poll_event(&ev); i++) {
        handle_vam_event(&ev);
        vam_event_release(&ev);
    }
    static float metrics_at=0;
    if(platform_graphics_allowed()&&A.time>=metrics_at){
        size_t pages=0;for(int t=0;t<TAB_COUNT;t++)for(int d=0;d<A.extent[t];d++)pages+=model_resident_bytes(A.stacks[t][d].page);
        uint32_t textures;uint64_t texture_bytes;images_measure(&textures,&texture_bytes);
        int user,cdram,phycont;platform_memory(&user,&cdram,&phycont);
        vam_record_metrics((uint32_t)pages,textures,texture_bytes,user,cdram,phycont);metrics_at=A.time+5;
    }
    trim_page_copies();
    order_upload_frame();
    /* Snapshot every frame: cheap, and keeps the progress bar honest. */
    vam_get_player_state(&A.player);
    if (A.player.has_track) apply_saved(A.player.track.uri,A.player.track.is_saved!=0);
    for (int i=0;i<PENDING_SAVES;i++) if (g_saves[i].req)
        apply_saved(g_saves[i].uri,g_saves[i].desired);
    if (notifications_update(&A.player, vam_now_ms(), prefs_get()->notifications && A.auth.status == VAM_AUTH_LOGGED_IN)) {
        char message[VAM_MSG_MAX];
        snprintf(message, sizeof(message), "Now playing: %.120s - %.100s", A.player.track.title, A.player.track.subtitle);
        app_toast(message);
    }
    if (!A.player.has_track && A.np_open)
        app_set_now_playing(false);

    if (A.typing && !app_use_touch_keyboard() && g_edit_changed_at >= 0 && A.time - g_edit_changed_at > 0.45f) {
        g_edit_changed_at = -1;
        if (!A.filter_typing && !A.name_operation) run_search(A.edit, false);
    }

    if (platform_graphics_allowed()) {
        if(A.typing)power_keep_awake();
        power_frame();
    }
    prefs_frame();
    lyrics_frame();
    if (!app_drawing()) {
        /* Screen off: keep playback and backend events going, draw nothing. */
        platform_frame(app_is_playing());
        return;
    }

    images_update();
    text_begin_frame();
    gfx_clear(rgb(C_BG));

    if (A.auth.status != VAM_AUTH_LOGGED_IN) {
        screen_login();
    } else if (!A.auth.is_premium) {
        screen_premium();
    } else {
        global_buttons();
        draw_main();
    }
    draw_toast();
    /* Desktop has no backlight: shade the window instead. */
    if (!platform_is_vita() && power_screen() != SCREEN_ON)
        gfx_rect(rect(0, 0, SCREEN_W, SCREEN_H), rgba(0x000000, power_screen() == SCREEN_OFF ? 255 : 150));
    apply_nav();
    platform_frame(app_is_playing());
}

void app_begin_filter(void)
{
    A.filter_typing = true;
    app_begin_typing();
}

static bool contains_folded(const char *text, const char *query)
{
    for (const char *p=text; ; p++) {
        const unsigned char *a=(const unsigned char *)p,*b=(const unsigned char *)query;
        while (*a && *b) {
            unsigned char x=*a, y=*b;
            if (x>='A' && x<='Z') x+='a'-'A';
            if (y>='A' && y<='Z') y+='a'-'A';
            if (x!=y) break;
            a++; b++;
        }
        if (!*b) return true;
        if (!*p) return false;
    }
}

bool app_matches_filter(const view *v,const vam_item *it)
{
    if (v->by_you && (it->kind != VAM_KIND_PLAYLIST || !it->is_owned)) return false;
    if (!v->filter_query[0]) return true;
    return contains_folded(it->title,v->filter_query) || contains_folded(it->subtitle,v->filter_query) ||
        contains_folded(it->album_title,v->filter_query) || contains_folded(it->creator,v->filter_query);
}

bool app_library_matches(const view *v,const vam_item *it)
{
    if(v->filter==LIB_PLAYLISTS && it->kind!=VAM_KIND_PLAYLIST && it->kind!=VAM_KIND_FOLDER && it->kind!=VAM_KIND_COLLECTION)return false;
    if(v->filter==LIB_ALBUMS && it->kind!=VAM_KIND_ALBUM)return false;
    if(v->filter==LIB_ARTISTS && it->kind!=VAM_KIND_ARTIST)return false;
    return app_matches_filter(v,it);
}

void app_play_ordered(view *v,uint32_t original_index)
{
    app_cancel_order_upload();
    if (!v->page || !v->page->section_count) return;
    if (model_has_more(v->page)) { app_toast("Songs are still loading. Try again when loading finishes."); return; }
    const model_section *s=&v->page->sections[0];
    const uint32_t *order=app_view_order(v);
    v->fallback_index=original_index;v->original_order_fallback=false;
    if(s->count&&!order){v->original_order_fallback=true;app_toast("Not enough memory for sorted playback. Original order is available.");return;}
    uint32_t n=0,selected=UINT32_MAX;
    for(uint32_t row=0;row<s->count;row++){
        uint32_t i=order[row];if(!s->items[i].is_playable)continue;
        if(i==original_index)selected=n;n++;
    }
    if(!n||selected==UINT32_MAX){app_toast("This song isn't available.");return;}
    if(n>VAM_ORDER_MAX_TRACKS){v->original_order_fallback=true;app_toast("Too many playable songs for sorted playback. Original order is available.");return;}
    uint32_t *snapshot=malloc(sizeof(*snapshot)*s->count);
    if(!snapshot){v->original_order_fallback=true;app_toast("Not enough memory to prepare sorted playback.");return;}
    memcpy(snapshot,order,sizeof(*snapshot)*s->count);
    uint32_t token=vam_order_begin(n,selected);
    if(!token){free(snapshot);v->original_order_fallback=true;app_toast("Couldn't prepare sorted playback. Original order is available.");return;}
    A.order_token=token;A.order_upload=snapshot;A.order_cursor=0;A.order_view=v;A.order_page=v->page;
    app_toast("Preparing sorted playback...");
}

void app_submit_playlist_edit(int operation,const char *target,const char *value)
{
    if(A.edit_request) {app_toast("Wait for the current playlist edit.");return;}
    A.edit_queue=false;
    A.edit_request=vam_playlist_edit(operation,target,value);
    app_toast(A.edit_request?"Updating playlist on Spotify...":"Couldn't start playlist edit.");
}
void app_begin_playlist_name(int operation,const char *uri,const char *name)
{
    if(A.edit_request){app_toast("Wait for the current playlist edit.");return;}
    app_begin_typing();
    A.name_operation=operation;
    snprintf(A.edit_target,sizeof(A.edit_target),"%s",uri?uri:"");
    snprintf(A.edit,sizeof(A.edit),"%s",name?name:"");
}

void app_submit_entry_edit(int operation)
{
    if(A.edit_request){app_toast("Wait for the current edit.");return;}
    sheet *s=&A.sheet;A.edit_queue=false;
    A.edit_request=vam_playlist_entry_edit(s->context_uri,s->revision,s->item.uri,s->item.entry_id,s->index,operation);
    app_toast(A.edit_request?"Updating playlist...":"Refresh the playlist before editing.");
}
void app_submit_queue_edit(const char *revision,uint32_t index,int operation)
{
    if(A.edit_request){app_toast("Wait for the current edit.");return;}
    A.edit_queue=true;A.edit_request=vam_queue_edit(revision,index,operation);
    app_toast(A.edit_request?"Updating queued songs...":"Refresh the queue before editing.");
}

void app_finish_typing(bool commit){end_typing(commit);}
