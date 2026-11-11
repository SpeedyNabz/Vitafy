#include "notifications.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned calls;
int platform_notify_song(const char *title, const char *artist) { (void)title; (void)artist; calls++; return -1; }
void vam_log(const char *message) { (void)message; }
#define CHECK(x) do { if (!(x)) { fprintf(stderr, "line %d: %s\n", __LINE__, #x); exit(1); } } while (0)
int main(void)
{
    vam_player_state s = {0};
    s.has_track = 1; s.is_active_device = 1; s.playback = VAM_PLAYBACK_PLAYING;
    strcpy(s.track.uri, "spotify:track:a"); strcpy(s.track.title, "Track A");
    notifications_reset();
    CHECK(notifications_update(&s, 0, true));
    CHECK(!notifications_update(&s, 200, true));
    s.position_ms = 1000;
    CHECK(!notifications_update(&s, 1000, true));
    strcpy(s.track.uri, "spotify:track:b");
    CHECK(!notifications_update(&s, 1500, true));
    strcpy(s.track.uri, "spotify:track:c");
    CHECK(notifications_update(&s, 5000, true));
    CHECK(calls == 2); /* latest skip wins, failed sends are not retried */
    s.playback = VAM_PLAYBACK_PAUSED;
    CHECK(!notifications_update(&s, 9000, true));
    s.playback = VAM_PLAYBACK_PLAYING;
    CHECK(!notifications_update(&s, 10000, true));
    CHECK(!notifications_update(&s, 10001, false));
    CHECK(notifications_update(&s, 10002, true));
    s.is_active_device = 0;
    CHECK(!notifications_update(&s, 15000, true));
    s.is_active_device = 1; s.has_track = 0;
    CHECK(!notifications_update(&s, 16000, true));
    uint16_t out[16];
    CHECK(notifications_utf16("Beyonc\xc3\xa9 \xf0\x9f\x8e\xb5", out, 16) == 10);
    CHECK(out[6] == 0xe9 && out[8] == 0xd83c && out[9] == 0xdfb5 && out[10] == 0);
    CHECK(notifications_utf16("\xf0\x9f\x8e\xb5", out, 2) == 0 && out[0] == 0);
    CHECK(notifications_utf16("\xe2", out, 16) == 1 && out[0] == 0xfffd);
    CHECK(notifications_utf16("\xed\xa0\x80", out, 16) == 3 && out[0] == 0xfffd);
    CHECK(notifications_utf16("abcdef", out, 4) == 3 && out[3] == 0);
    CHECK(notifications_utf16(NULL, out, 16) == 0 && out[0] == 0);
    CHECK(notifications_utf16("x", NULL, 0) == 0);
    puts("Vitafy notification coalescing, failure, privacy and Unicode checks passed");
    return 0;
}
