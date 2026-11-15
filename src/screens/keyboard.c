/* In-app keyboard: no system overlay or blocking native dialog. */
#include "../app.h"
#include "../gfx/text.h"
#include "../theme.h"
#include <string.h>
#include <stdio.h>

static bool key(uint32_t id,const char *label,rectf r,bool active)
{
    ui_res hit=ui_item(id,r,0);
    gfx_rrect(r,5,rgb(hit.pressed||hit.focused?C_SURFACE_HI2:active?C_ACCENT:C_SURFACE_HI));
    text_draw_aligned(label,r.x+r.w/2,r.y+(r.h-text_line_height(FS_L))/2,FS_L,FW_BOLD,rgb(active?0x000000:C_TEXT),r.w-4,ALIGN_CENTER);
    if(hit.focused)draw_focus_ring(r,5);
    return hit.clicked;
}
void screen_keyboard(void)
{
    gfx_rect(rect(0,0,SCREEN_W,SCREEN_H),rgb(C_BG));
    const char *title=A.filter_typing?"Filter library":A.name_operation==1?"New playlist":A.name_operation==2?"Rename playlist":"Search music";
    text_draw(title,16,10,FS_L,FW_BOLD,rgb(C_TEXT),920);
    gfx_rrect(rect(12,54,936,58),6,rgb(C_SURFACE_HI));
    text_draw_vc(A.edit,26,83,FS_L,FW_REGULAR,rgb(C_TEXT),908);
    static const char *letters[]={"qwertyuiop","asdfghjkl","zxcvbnm,.'","1234567890"};
    static const char *symbols[]={"!@#$%^&*()","-_=+[]{}<>","/\\:;\"?~`|.","1234567890"};
    for(int row=0;row<4;row++){
        const char *chars=A.keyboard_symbols?symbols[row]:letters[row];
        size_t count=strlen(chars);float x=12+(936.0f-(float)count*94.0f)/2;
        for(size_t col=0;col<count;col++){
            char label[2]={chars[col],0};
            if(A.keyboard_shift&&label[0]>='a'&&label[0]<='z')label[0]-='a'-'A';
            if(key(ui_idx(ui_id("keyboard.key"),row*10+(uint32_t)col),label,rect(x+col*94,124+row*64,88,58),false)){
                size_t n=strlen(A.edit);if(n+1<sizeof(A.edit)){A.edit[n]=label[0];A.edit[n+1]=0;}
            }
        }
    }
    float y=388;
    if(key(ui_id("keyboard.shift"),"Shift",rect(12,y,120,64),A.keyboard_shift))A.keyboard_shift=!A.keyboard_shift;
    if(key(ui_id("keyboard.symbols"),"#+=",rect(140,y,100,64),A.keyboard_symbols))A.keyboard_symbols=!A.keyboard_symbols;
    if(key(ui_id("keyboard.space"),"Space",rect(248,y,340,64),false)){
        size_t n=strlen(A.edit);if(n+1<sizeof(A.edit)){A.edit[n]=' ';A.edit[n+1]=0;}
    }
    if(key(ui_id("keyboard.delete"),"Delete",rect(596,y,170,64),false)){
        size_t n=strlen(A.edit);if(n){do{n--;}while(n&&((unsigned char)A.edit[n]&0xc0)==0x80);A.edit[n]=0;}
    }
    if(key(ui_id("keyboard.clear"),"Clear",rect(774,y,174,64),false))A.edit[0]=0;
    if(key(ui_id("keyboard.cancel"),"Cancel",rect(12,468,160,64),false))app_finish_typing(false);
    if(key(ui_id("keyboard.done"),"Done",rect(748,468,200,64),true))app_finish_typing(true);
    text_draw("Touch keys or use the D-pad and Confirm. Back cancels.",188,490,FS_XS,FW_REGULAR,rgb(C_SUBTEXT),536);
}
