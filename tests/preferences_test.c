#include "prefs.h"
#include <stdio.h>
#include <string.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"preference check failed at %d\n",__LINE__);return 1;}}while(0)
static const char *test_dir;
const char *platform_data_dir(void){return test_dir;}
bool platform_is_vita(void){return false;}
int main(int argc,char **argv)
{
    CHECK(argc==3);test_dir=argv[1];
    if(!strcmp(argv[2],"write")){
        prefs_load();CHECK(prefs_get()->dim_after==0&&prefs_get()->off_after==0&&prefs_get()->bitrate==0);
        char path[512];snprintf(path,sizeof(path),"%ssettings.txt",test_dir);FILE *f=fopen(path,"w");CHECK(f);
        fputs("audio_quality=96\nmetadata_cache_mb=16\ndim_after=15\nscreen_off_after=30\nnotifications=0\nscreen_off_combo=select+start\n",f);fclose(f);
        prefs_load();CHECK(prefs_get()->bitrate==96&&!prefs_get()->notifications&&prefs_get()->dim_after==15&&prefs_get()->off_after==30&&prefs_get()->metadata_mb==16);
        for(int i=0;i<64;i++){char uri[96];snprintf(uri,sizeof(uri),"spotify:playlist:test%d",i);prefs_view_save("old",uri,1,false,false,false);}
        prefs_item_toggle("old","spotify:playlist:test63",8);
        prefs_view_save("new","spotify:playlist:new",2,false,false,false);
        CHECK(!(prefs_item_flags("new","spotify:playlist:new")&8));
        CHECK(prefs_item_flags("old","spotify:playlist:test63")&8);
        prefs_item_toggle("persist","spotify:playlist:saved",8);
        prefs_item_toggle("persist","spotify:playlist:saved",16);
        prefs_view_save("persist","spotify:playlist:saved",4,true,true,true);
        CHECK((prefs_item_flags("persist","spotify:playlist:saved")&24)==24);prefs_flush();
    }else{
        prefs_load();CHECK(prefs_get()->bitrate==96&&!prefs_get()->notifications&&prefs_get()->dim_after==15&&prefs_get()->off_after==30&&prefs_get()->metadata_mb==16);
        int sort=0;bool descending=false,compact=false,grid=false;
        prefs_view_load("persist","spotify:playlist:saved",&sort,&descending,&compact,&grid);
        CHECK(sort==4&&descending&&compact&&grid);
        CHECK((prefs_item_flags("persist","spotify:playlist:saved")&24)==24);
        sort=0;descending=compact=grid=false;
        prefs_view_load("other-account","spotify:playlist:saved",&sort,&descending,&compact,&grid);
        CHECK(sort==0&&!descending&&!compact&&!grid);
        CHECK(!prefs_item_flags("other-account","spotify:playlist:saved"));
    }
    puts("preferences: quality migration, eviction, persistence and account isolation passed");return 0;
}
