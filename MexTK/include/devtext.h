#ifndef MEX_H_DEVTEXT
#define MEX_H_DEVTEXT

#include "structs.h"
#include "gx.h"

/*** Structs ***/

struct DevText
{
    int x0;                   // 0x0
    u8 width;                 // 0x4
    u8 height;                // 0x5
    u8 cursor_x;              // 0x6
    u8 cursor_y;              // 0x7
    Vec2 scale;               // 0x8
    GXColor bg_color;         // 0x10
    int x14;                  // 0x14
    int x18;                  // 0x18
    int x1c;                  // 0x1c
    int x20;                  // 0x20
    char x24;                 // 0x24
    char x25;                 // 0x25
    char show_text : 1;       // 0x26
    char show_background : 1; // 0x26
    char x26_20 : 1;          // 0x26
    char show_cursor : 1;     // 0x26
    char x27;                 // 0x27
    u8 *text_data;            // 0x28
    int x2c;                  // 0x2c
    DevText *next;            // 0x30
    int x34;                  // 0x34
    int x38;                  // 0x38
    int x3c;                  // 0x3c
    int x40;                  // 0x40
    int x44;                  // 0x44
    int x48;                  // 0x48
    int x4c;                  // 0x4c
    int x50;                  // 0x50
    int x54;                  // 0x54
    int x58;                  // 0x58
    int x5c;                  // 0x5c
};

/*** Functions ***/

DevText *DevelopText_CreateDataTable(int unk1, int x, int y, int width, int height, void *alloc);
void DevelopText_Activate(void *unk, DevText *text);
void DevelopText_Deactivate(void *unk);
void DevelopText_AddString(DevText *text, ...);
void DevelopText_EraseAllText(DevText *text);
void DevelopText_ResetCursorXY(DevText *text, int x, int y);
void DevelopText_StoreTextColor(DevText *text, GXColor *RGBA);
void DevelopText_StoreBGColor(DevText *text, GXColor *RGBA);
void DevelopText_ShowText(DevText *text);
void DevelopText_HideText(DevText *text);
void DevelopText_ShowBG(DevText *text);
void DevelopText_HideBG(DevText *text);
void DevelopText_StoreTextScale(DevText *text, float x, float y);
void Develop_DrawSphere(float size, Vec3 *pos1, Vec3 *pos2, GXColor *diffuse, GXColor *ambient);
void Develop_UpdateMatchHotkeys();

static int *stc_dblevel = R13_OFFSET(-0x6C98);

#endif
