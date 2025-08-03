#ifndef MEX_H_TEXT
#define MEX_H_TEXT

#include "structs.h"
#include "datatypes.h"
#include "obj.h"
#include "gx.h"

/*** Structs ***/

struct SISData
{
    u8 *image_data_arr;   // array of I4 image data for characters, stride is (32*32) / 2
    u8 *kerning_data_arr; // array of kerning data for characters, stride is 0x2, kerning is at 0x0
};

struct TextCanvas
{
    TextCanvas *next; // 0x0
    GOBJ *cam_gobj;   // 0x4,
    u16 size;         // 0x8 data remaining after this alloc?
    u16 sis_idx;      // 0xa sis group this canvas belongs to
    // data after this....
    u8 p_link;  // 0xC
    u8 xd;      // 0xD
    u8 gx_link; // 0xE
    u8 gx_pri;  // 0xF
};

struct Text
{
    Vec3 trans;                               // 0x0-0xC, affects entire viewport
    Vec2 aspect;                              // 0xC-0x14 CHANGE THIS TO BOUND, affects entire viewport
    float scissor_top;                        // 0x14, is_scissor must be enabled
    float scissor_bot;                        // 0x18, is_scissor must be enabled
    float scissor_left;                       // 0x1c, is_scissor must be enabled
    float scissor_right;                      // 0x20, is_scissor must be enabled
    Vec2 viewport_scale;                      // 0x24-0x2C, scales entire viewport including text (was orignally scale)
    GXColor viewport_color;                   // 0x2c, if the alpha value is != 0, uses an additional GXSetTevAlphaIn call @ 803a87f8 if enabled. background is the size of the aspect
    GXColor color;                            // 0x30
    Vec2 scale;                               // 0x34-0x3C scales just the text. (was originally stretch)
    float x3c;                                // 0x3c
    float x40;                                // 0x40
    u16 x44;                                  // 0x44
    u16 x46;                                  // 0x46
    u8 use_aspect;                            // 0x48, will fit the text to the bounds defined in aspect
    u8 kerning;                               // 0x49
    u8 align;                                 // 0x4a
    u8 x4b;                                   //
    u8 is_depth_compare;                      // 0x4C, if set to 1, updates the depth buffer when rendering, uses GX_LEQUAL (803a85a0)
    u8 hidden;                                // 0x4D
    u8 is_scissor;                            // 0x4E, crops the text within the bounding box 803a88e8
    u8 sis_id;                                // 0x4F, id of the premade text file to use
    void *x50;                                // some buffer alloc'd @ 803a5ba0
    GOBJ *gobj;                               // 0x54
    void (*render_callback)(GOBJ *text_gobj); // 0x58, read at 803a878c
    u8 *text_start;                           // 0x5C, start parsing text data at this ptr
    u8 *text_end;                             // 0x60, stops parsing text data at this ptr
    TextCanvas *allocInfo;                    // 0x64
    void *x68;                                // 0x68, some alloc used for dialogue?
    u16 x6c;                                  // 0x6c, flags of some kind
    u16 x6e;                                  // 0x6e
    float x70;                                // 0x70
    float x74;                                // 0x74
    float x78;                                // 0x78
    float x7c;                                // 0x7c
    float x80;                                // 0x80
    float x84;                                // 0x84
    float x88;                                // 0x88
    GXColor x8c;                              // 0x8c, temp color storage when processing text opcodes. 803a8d7c
    u16 x90;                                  // 0x90
    u16 x92;                                  // 0x92
    int x94;                                  // 0x94
    int char_display_num;                     // 0x98, how many characters to display
    u8 is_fit;                                // 0x9c
    u8 x9d;                                   // 0x9d
    u8 x9e;                                   // 0x9e, checked per character, 1 and 2 are valid inputs. updates the float @ x70
    u8 x9f;                                   // 0x9f
    int xa0;                                  // 0xa0
};

/*** Functions ***/

int Text_CreateCanvas(int sis_id, GOBJ *cam_gobj, int gobj_entityclass, int gobj_plink, int gobj_ppriority, int gxlink, int gxpri, int cobj_gxpri); // the optional gobj and cobj_gxlink are used to create a cobj as well. set gobj
Text *Text_CreateText(int sis_id, int canvasID);
Text *Text_CreateText2(int sis_id, int canvasID, float pos_x, float pos_y, float pos_z, float limit_x, float limit_y);
void Text_Destroy(Text *text);
int Text_AddSubtext(Text *text, float xPos, float yPos, char *string, ...);
void Text_SetScale(Text *text, int subtext, float x, float y);
void Text_SetColor(Text *text, int subtext, GXColor *color);
void Text_SetPosition(Text *text, int subtext, float x, float y);
void Text_SetText(Text *text, int subtext, const char *string, ...);
u8 *Text_Alloc(int size);
void Text_DestroyAlloc(u8 *alloc);
void Text_DestroyAllAlloc(Text *text);
int Text_ConvertToMenuText(char *out, char *in);
void Text_GX(GOBJ *gobj, int pass);
void Text_LoadSdFile(int index, char *filename, char *symbol);
void Text_SetSisText(Text *text, int text_index);
void Text_DestroyAllSisCanvas(int sis_id);
void Text_DestroyCanvas(TextCanvas *);
void Text_InitSisHeap();

/*** Variables ***/
// Text data
static int *stc_textheap_size = (int *)(R13_OFFSET(-0x3d38));
static TextCanvas **stc_textheap_start = (TextCanvas **)(R13_OFFSET(-0x3d34));
static TextCanvas **stc_textheap_next = (TextCanvas **)(R13_OFFSET(-0x3d30));
static TextCanvas **stc_textheap_first = (TextCanvas **)(R13_OFFSET(-0x3d2c));

// Text object
static Text **stc_text_first = (Text **)(R13_OFFSET(-0x3d28));

// Text canvas
static TextCanvas **stc_textcanvas_first = (TextCanvas **)(R13_OFFSET(-0x3d24));

// Sis Library
static HSD_Archive **stc_sis_archives = (HSD_Archive **)0x804d1110; // array of sis file archive pointers
static SISData **stc_sis_data = (void *)0x804d1124;                 // array of currently loaded sis data, indexed by sis_id

#endif
