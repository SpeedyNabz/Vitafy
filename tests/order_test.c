#include "model.h"
#include <stdio.h>
#include <string.h>

#define CHECK(x) do { if (!(x)) { fprintf(stderr,"failed line %d\n",__LINE__); return 1; } } while (0)
int main(void)
{
    vam_item items[5] = {0};
    strcpy(items[0].title,"Zulu"); strcpy(items[0].subtitle,"Alpha"); items[0].duration_ms=100;
    strcpy(items[1].title,"alpha"); strcpy(items[1].subtitle,"Zulu"); items[1].duration_ms=300;
    strcpy(items[2].title,"ALPHA"); strcpy(items[2].subtitle,"Beta"); items[2].duration_ms=200;
    strcpy(items[3].title,"Beta"); items[3].duration_ms=200;
    strcpy(items[4].title,"Zulu"); items[4].duration_ms=50;
    model_section s = {0}; s.items=items; s.count=3;
    uint32_t order[5];
    CHECK(model_order(&s,SORT_TITLE,false,order));
    CHECK(order[0]==1 && order[1]==2 && order[2]==0);
    CHECK(model_order(&s,SORT_TITLE,true,order));
    CHECK(order[0]==0 && order[1]==1 && order[2]==2);
    /* Appending changes the sorted positions, never the backend offsets. */
    s.count=5;
    CHECK(model_order(&s,SORT_TITLE,false,order));
    CHECK(order[0]==1 && order[1]==2 && order[2]==3 && order[3]==0 && order[4]==4);
    CHECK(!strcmp(items[0].title,"Zulu") && !strcmp(items[1].title,"alpha"));
    CHECK(model_order(&s,SORT_DURATION,true,order));
    CHECK(order[0]==1 && order[1]==2 && order[2]==3 && order[4]==4);
    CHECK(model_order(&s,SORT_CUSTOM,false,order));
    for (uint32_t i=0;i<5;i++) CHECK(order[i]==i);
    CHECK(model_order(&s,SORT_ARTIST,false,order));
    CHECK(order[0]==3 && order[1]==4 && order[2]==0 && order[3]==2 && order[4]==1);
    s.count=0; CHECK(model_order(&s,SORT_TITLE,false,NULL));
    s.count=5;
    items[0].added_at=0; items[1].added_at=100; items[2].added_at=300; items[3].added_at=100; items[4].added_at=0;
    CHECK(model_order(&s,SORT_ADDED,true,order));
    CHECK(order[0]==2 && order[1]==1 && order[2]==3 && order[3]==0 && order[4]==4);
    CHECK(model_order(&s,SORT_ADDED,false,order));
    CHECK(order[0]==1 && order[1]==3 && order[2]==2 && order[3]==0);
    strcpy(items[0].album_title,"Zeta"); strcpy(items[1].album_title,"Alpha");
    CHECK(model_order(&s,SORT_ALBUM,false,order)); CHECK(order[4]==0);
    vam_section first={0},next={0}; vam_page p={0},q={0};
    first.items=items; first.count=2; first.total=5;
    p.sections=&first; p.section_count=1; strcpy(p.revision,"one");
    model_page *m=model_from_vam(&p); CHECK(m && m->sections[0].count==2);
    next.items=items+2; next.count=2; next.total=5; next.offset=2;
    q.sections=&next; q.section_count=1; strcpy(q.revision,"two");
    CHECK(!model_append(m,&q) && m->sections[0].count==2);
    strcpy(q.revision,"one"); CHECK(model_append(m,&q) && m->sections[0].count==4);
    CHECK(!model_append(m,&q) && m->sections[0].count==4);
    model_free(m);
    puts("display order: stable ties, paging, original offsets and empty list passed");
    return 0;
}
