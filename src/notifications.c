#include "notifications.h"
#include "platform/platform.h"
#include <stdio.h>
#include <string.h>

static char last_uri[VAM_URI_MAX];
static uint64_t sent_at;
static bool sent;

void notifications_reset(void) { last_uri[0] = 0; sent_at = 0; sent = false; }

bool notifications_update(const vam_player_state *s, uint64_t now, bool enabled)
{
    if (!enabled || !s->has_track || !s->is_active_device) {
        notifications_reset();
        return false;
    }
    if (s->playback != VAM_PLAYBACK_PLAYING || !s->track.uri[0] || !s->track.title[0]) return false;
    if (!strcmp(last_uri, s->track.uri)) return false;
    if (sent && now >= sent_at && now - sent_at < 5000) return false;
    snprintf(last_uri, sizeof(last_uri), "%s", s->track.uri);
    sent_at = now;
    sent = true;
    int rc = platform_notify_song(s->track.title, s->track.subtitle);
    if (rc < 0) {
        char message[80];
        snprintf(message, sizeof(message), "song notification unavailable rc=%#x", (unsigned)rc);
        vam_log(message);
    }
    return true;
}

size_t notifications_utf16(const char *src, uint16_t *dst, size_t cap)
{
    size_t out = 0;
    if (!cap) return 0;
    const unsigned char *s = (const unsigned char *)(src ? src : "");
    while (*s) {
        uint32_t cp;
        unsigned count;
        if (*s < 0x80) { cp = *s; count = 1; }
        else if (*s >= 0xc2 && *s <= 0xdf) { cp = *s & 31; count = 2; }
        else if (*s >= 0xe0 && *s <= 0xef) { cp = *s & 15; count = 3; }
        else if (*s >= 0xf0 && *s <= 0xf4) { cp = *s & 7; count = 4; }
        else { cp = 0xfffd; count = 1; }
        bool valid = true;
        for (unsigned i = 1; i < count; i++) {
            if (!s[i] || (s[i] & 0xc0) != 0x80) { valid = false; break; }
            cp = (cp << 6) | (s[i] & 63);
        }
        if (!valid || (count == 2 && cp < 0x80) || (count == 3 && cp < 0x800) ||
            (count == 4 && cp < 0x10000) || cp > 0x10ffff || (cp >= 0xd800 && cp <= 0xdfff)) {
            cp = 0xfffd; count = 1;
        }
        size_t units = cp > 0xffff ? 2 : 1;
        if (out + units >= cap) break;
        if (units == 2) {
            cp -= 0x10000;
            dst[out++] = (uint16_t)(0xd800 + (cp >> 10));
            dst[out++] = (uint16_t)(0xdc00 + (cp & 1023));
        } else dst[out++] = (uint16_t)cp;
        s += count;
    }
    dst[out] = 0;
    return out;
}
