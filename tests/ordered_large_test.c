#include "vam_backend.h"
#include <SDL.h>
#include <stdio.h>
#include <string.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"ordered check failed: %d\n",__LINE__);return 1;}}while(0)
static int wait_page(uint32_t request,vam_event *ev){
    uint64_t end=vam_now_ms()+3000;
    while(vam_now_ms()<end){while(vam_poll_event(ev)){if(ev->request_id==request)return 1;vam_event_release(ev);}SDL_Delay(10);}return 0;
}
int main(int argc,char **argv){
    CHECK(argc==2);SDL_Init(SDL_INIT_TIMER);vam_config cfg={0};cfg.api_version=VAM_API_VERSION;cfg.device_name="ordered mock";cfg.data_dir=argv[1];
    CHECK(vam_init(&cfg)==VAM_OK);vam_event ev;
    CHECK(wait_page(vam_request_page("spotify:playlist:mock00",0,100),&ev));CHECK(ev.page);
    char uris[2][VAM_URI_MAX];snprintf(uris[0],sizeof(uris[0]),"%s",ev.page->sections[0].items[0].uri);
    uris[1][0]=0;for(uint32_t i=1;i<ev.page->sections[0].count;i++)if(strcmp(uris[0],ev.page->sections[0].items[i].uri)){snprintf(uris[1],sizeof(uris[1]),"%s",ev.page->sections[0].items[i].uri);break;}
    vam_event_release(&ev);CHECK(uris[1][0]);
    const unsigned counts[]={2048,2049,5000,VAM_ORDER_MAX_TRACKS};
    for(unsigned c=0;c<4;c++){
        unsigned count=counts[c],selected=count/2;uint32_t token=vam_order_begin(count,selected);CHECK(token);
        CHECK(vam_order_commit(token)!=VAM_OK);
        for(unsigned offset=0;offset<count;){const char *chunk[VAM_ORDER_CHUNK];unsigned n=0;while(n<VAM_ORDER_CHUNK&&offset+n<count){chunk[n]=uris[(offset+n)%2];n++;}CHECK(vam_order_append(token,chunk,n)==VAM_OK);offset+=n;}
        CHECK(vam_order_commit(token)==VAM_OK);CHECK(vam_order_commit(token)!=VAM_OK);
        vam_player_state state;vam_get_player_state(&state);CHECK(!strcmp(state.track.uri,uris[selected%2]));
        CHECK(vam_next()==VAM_OK);vam_get_player_state(&state);CHECK(!strcmp(state.track.uri,uris[(selected+1)%2]));
        CHECK(vam_prev()==VAM_OK);vam_get_player_state(&state);CHECK(!strcmp(state.track.uri,uris[selected%2]));
    }
    CHECK(vam_order_begin(VAM_ORDER_MAX_TRACKS+1,0)==0);uint32_t token=vam_order_begin(1,0);CHECK(token);vam_order_cancel(token);CHECK(vam_order_commit(token)!=VAM_OK);
    vam_shutdown();SDL_Quit();puts("large ordered contexts: duplicate order, selection, Next/Previous and cancellation passed");return 0;
}
