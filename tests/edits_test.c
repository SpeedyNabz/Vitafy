/* Offline contract checks: duplicates, stale snapshots and context isolation. */
#include "vam_backend.h"
#include <SDL.h>
#include <stdio.h>
#include <string.h>
#define CHECK(x) do {if(!(x)){fprintf(stderr,"edit check failed at %d\n",__LINE__);return 1;}}while(0)

static int reply(uint32_t request,vam_event *out)
{
    uint64_t end=vam_now_ms()+3000;
    while(vam_now_ms()<end){
        vam_event ev;
        while(vam_poll_event(&ev)){
            if(ev.request_id==request){*out=ev;return 1;}
            vam_event_release(&ev);
        }
        SDL_Delay(10);
    }
    return 0;
}
static int done(uint32_t request,int result)
{
    vam_event ev;if(!request||!reply(request,&ev))return 0;
    int ok=ev.type==VAM_EV_REQUEST_DONE&&ev.result==result;vam_event_release(&ev);return ok;
}
int main(int argc,char **argv)
{
    CHECK(argc==2);SDL_Init(SDL_INIT_TIMER);
    vam_config config={0};config.api_version=VAM_API_VERSION;config.device_name="Edit contract mock";config.data_dir=argv[1];
    CHECK(vam_init(&config)==VAM_OK);
    vam_event ev;CHECK(reply(vam_request_page("spotify:playlist:mock00",0,100),&ev));CHECK(ev.page&&ev.result==VAM_OK);
    unsigned count=ev.page->sections[0].count;CHECK(count>2);
    char first[VAM_URI_MAX],second[VAM_URI_MAX],revision[VAM_URI_MAX];
    snprintf(first,sizeof(first),"%s",ev.page->sections[0].items[0].uri);
    snprintf(second,sizeof(second),"%s",ev.page->sections[0].items[1].uri);
    snprintf(revision,sizeof(revision),"%s",ev.page->revision);vam_event_release(&ev);
    CHECK(done(vam_playlist_edit(3,"spotify:playlist:mock00",first),VAM_OK));
    CHECK(done(vam_playlist_entry_edit("spotify:playlist:mock00",revision,first,"",0,1),VAM_ERR_UNAVAILABLE));
    CHECK(reply(vam_request_page("spotify:playlist:mock00",0,100),&ev));CHECK(ev.page->sections[0].count==count+1);
    snprintf(revision,sizeof(revision),"%s",ev.page->revision);vam_event_release(&ev);
    CHECK(done(vam_playlist_entry_edit("spotify:playlist:mock00",revision,first,"",count,1),VAM_OK));
    CHECK(reply(vam_request_page("spotify:playlist:mock00",0,100),&ev));CHECK(ev.page->sections[0].count==count);
    CHECK(!strcmp(ev.page->sections[0].items[0].uri,first));snprintf(revision,sizeof(revision),"%s",ev.page->revision);vam_event_release(&ev);
    CHECK(done(vam_playlist_entry_edit("spotify:playlist:mock00",revision,first,"",0,3),VAM_OK));
    CHECK(reply(vam_request_page("spotify:playlist:mock00",0,100),&ev));CHECK(!strcmp(ev.page->sections[0].items[0].uri,second));vam_event_release(&ev);
    CHECK(vam_play("spotify:playlist:mock00",NULL,0)==VAM_OK);
    CHECK(vam_add_to_queue(first)==VAM_OK&&vam_add_to_queue(first)==VAM_OK);
    CHECK(reply(vam_request_page("vam:queue",0,50),&ev));CHECK(ev.page->section_count==3&&ev.page->sections[1].count==2);
    unsigned context_count=ev.page->sections[2].count;CHECK(context_count>0);
    snprintf(revision,sizeof(revision),"%s",ev.page->revision);vam_event_release(&ev);
    CHECK(done(vam_queue_edit("stale",0,1),VAM_ERR_UNAVAILABLE));
    CHECK(done(vam_queue_edit(revision,0,1),VAM_OK));
    CHECK(reply(vam_request_page("vam:queue",0,50),&ev));CHECK(ev.page->sections[1].count==1&&ev.page->sections[2].count==context_count);
    snprintf(revision,sizeof(revision),"%s",ev.page->revision);vam_event_release(&ev);
    CHECK(done(vam_queue_edit(revision,0,4),VAM_OK));
    CHECK(reply(vam_request_page("vam:queue",0,50),&ev));CHECK(ev.page->sections[1].count==0&&ev.page->sections[2].count==context_count);vam_event_release(&ev);
    vam_shutdown();SDL_Quit();puts("revision edits: duplicate identity, conflict rejection and context preservation passed");return 0;
}
