#include "model.h"

#include <stdlib.h>
#include <string.h>

static int text_compare(const char *a, const char *b)
{
    /* Deterministic ASCII folding; UTF-8 is preserved, not locale-collated. */
    for (;;) {
        unsigned char x = (unsigned char)*a++, y = (unsigned char)*b++;
        if (x >= 'A' && x <= 'Z') x += 'a' - 'A';
        if (y >= 'A' && y <= 'Z') y += 'a' - 'A';
        if (x != y) return x < y ? -1 : 1;
        if (!x) return 0;
    }
}

static int order_compare(const model_section *s, uint32_t a, uint32_t b, int mode, bool desc)
{
    const vam_item *x = &s->items[a], *y = &s->items[b];
    int c = 0;
    if (mode == SORT_TITLE) c = text_compare(x->title, y->title);
    if (mode == SORT_ARTIST) c = text_compare(x->subtitle, y->subtitle);
    if (mode == SORT_DURATION) c = (x->duration_ms > y->duration_ms) - (x->duration_ms < y->duration_ms);
    if (mode == SORT_ALBUM) c = text_compare(x->album_title, y->album_title);
    if (mode == SORT_CREATOR) c = text_compare(x->creator, y->creator);
    /* Missing dates/history stay last for either direction. */
    if (mode == SORT_ADDED || mode == SORT_RELEASE || mode == SORT_RECENTS) {
        int64_t a = mode == SORT_ADDED ? x->added_at : mode == SORT_RELEASE ? x->release_date : x->recent_rank;
        int64_t b = mode == SORT_ADDED ? y->added_at : mode == SORT_RELEASE ? y->release_date : y->recent_rank;
        if (!a && b) return 1;
        if (a && !b) return -1;
        c = (a > b) - (a < b);
    }
    if (mode == SORT_CUSTOM) c = (a > b) - (a < b);
    if (c) return desc ? -c : c;
    return (a > b) - (a < b); /* stable ties, including duplicate tracks */
}

bool model_order(const model_section *s, int mode, bool descending, uint32_t *order)
{
    if (!s || (s->count && (!s->items || !order))) return false;
    for (uint32_t i = 0; i < s->count; i++) order[i] = i;
    if (s->count < 2) return true;
    uint32_t *tmp = malloc(sizeof(*tmp) * s->count);
    if (!tmp) return false;
    for (uint32_t width = 1; width < s->count; width *= 2) {
        for (uint32_t lo = 0; lo < s->count; lo += width * 2) {
            uint32_t mid = lo + width < s->count ? lo + width : s->count;
            uint32_t hi = mid + width < s->count ? mid + width : s->count;
            uint32_t a = lo, b = mid, k = lo;
            while (a < mid && b < hi)
                tmp[k++] = order_compare(s, order[a], order[b], mode, descending) <= 0 ? order[a++] : order[b++];
            while (a < mid) tmp[k++] = order[a++];
            while (b < hi) tmp[k++] = order[b++];
        }
        memcpy(order, tmp, sizeof(*tmp) * s->count);
        if (width > s->count / 2) break;
    }
    free(tmp);
    return true;
}

static void copy_str(char *dst, const char *src, size_t n)
{
    size_t len = strnlen(src, n - 1);
    memcpy(dst, src, len);
    dst[len] = 0;
}

static bool reserve(model_section *s, uint32_t want)
{
    if (want <= s->cap)
        return true;
    if (want > SIZE_MAX / sizeof(vam_item) || (size_t)want*sizeof(vam_item)>48*1024*1024) return false;
    uint32_t cap = s->cap ? s->cap : 32;
    while (cap < want) {
        if(cap>UINT32_MAX/2){cap=want;break;}
        cap *= 2;
    }
    if (cap > SIZE_MAX / sizeof(vam_item) || (size_t)cap*sizeof(vam_item)>48*1024*1024) cap=want;
    vam_item *n = realloc(s->items, sizeof(vam_item) * cap);
    if (!n)
        return false;
    s->items = n;
    s->cap = cap;
    return true;
}

model_page *model_from_vam(const vam_page *p)
{
    model_page *m = calloc(1, sizeof(*m));
    if (!m)
        return NULL;
    m->kind = p->kind;
    copy_str(m->uri, p->uri, sizeof(m->uri));
    copy_str(m->title, p->title, sizeof(m->title));
    copy_str(m->subtitle, p->subtitle, sizeof(m->subtitle));
    copy_str(m->description, p->description, sizeof(m->description));
    copy_str(m->image_url, p->image_url, sizeof(m->image_url));
    m->is_saved = p->is_saved != 0;
    m->can_edit=p->can_edit!=0;
    m->is_stale=p->is_stale!=0;
    copy_str(m->revision,p->revision,sizeof(m->revision));
    if (p->section_count && p->sections) {
        m->sections = calloc(p->section_count, sizeof(model_section));
        if (!m->sections) {
            free(m);
            return NULL;
        }
        m->section_count = p->section_count;
        for (uint32_t i = 0; i < p->section_count; i++) {
            const vam_section *src = &p->sections[i];
            model_section *dst = &m->sections[i];
            copy_str(dst->title, src->title, sizeof(dst->title));
            dst->layout = src->layout;
            dst->total = src->total > src->count ? src->total : src->count;
            if(src->count && (!src->items || !reserve(dst,src->count))){model_free(m);return NULL;}
            if (src->count) {
                memcpy(dst->items, src->items, sizeof(vam_item) * src->count);
                dst->count = src->count;
            }
        }
    }
    return m;
}

void model_free(model_page *m)
{
    if (!m)
        return;
    for (uint32_t i = 0; i < m->section_count; i++)
        free(m->sections[i].items);
    free(m->sections);
    free(m);
}

bool model_append(model_page *m, const vam_page *p)
{
    if (!m || !m->section_count || !p->section_count || !p->sections)
        return false;
    if (strcmp(m->revision,p->revision) != 0) return false;
    const vam_section *src = &p->sections[0];
    model_section *dst = &m->sections[0];
    if (src->offset != dst->count || !src->count || !src->items)
        return false;
    if (src->count > UINT32_MAX-dst->count || !reserve(dst, dst->count + src->count))
        return false;
    memcpy(dst->items + dst->count, src->items, sizeof(vam_item) * src->count);
    dst->count += src->count;
    if (src->total > dst->total)
        dst->total = src->total;
    if (dst->count > dst->total)
        dst->total = dst->count;
    return true;
}

bool model_has_more(const model_page *m)
{
    return m && m->section_count && m->sections[0].count < m->sections[0].total;
}

void model_set_saved(model_page *m, const char *uri, bool saved)
{
    if (!m)
        return;
    if (strcmp(m->uri, uri) == 0)
        m->is_saved = saved;
    for (uint32_t i = 0; i < m->section_count; i++)
        for (uint32_t j = 0; j < m->sections[i].count; j++)
            if (strcmp(m->sections[i].items[j].uri, uri) == 0)
                m->sections[i].items[j].is_saved = saved;
}

size_t model_resident_bytes(const model_page *p){if(!p)return 0;size_t n=sizeof(*p)+p->section_count*sizeof(model_section);for(uint32_t i=0;i<p->section_count;i++)n+=(size_t)p->sections[i].cap*sizeof(vam_item);return n;}
