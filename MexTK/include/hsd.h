#ifndef MEX_H_HSD
#define MEX_H_HSD

#include "structs.h"
#include "datatypes.h"
#include "obj.h"
#include "gx.h"

// button bits
#define PAD_BUTTON_DPAD_LEFT 0x1
#define PAD_BUTTON_DPAD_RIGHT 0x2
#define PAD_BUTTON_DPAD_DOWN 0x4
#define PAD_BUTTON_DPAD_UP 0x8
#define PAD_TRIGGER_Z 0x10
#define PAD_TRIGGER_R 0x20
#define PAD_TRIGGER_L 0x40
#define PAD_BUTTON_A 0x100
#define PAD_BUTTON_B 0x200
#define PAD_BUTTON_X 0x400
#define PAD_BUTTON_Y 0x800
#define PAD_BUTTON_START 0x1000
#define PAD_BUTTON_UP 0x10000
#define PAD_BUTTON_DOWN 0x20000
#define PAD_BUTTON_LEFT 0x40000
#define PAD_BUTTON_RIGHT 0x80000

#define HSD_BUTTON_DPAD_LEFT 0x0001
#define HSD_BUTTON_DPAD_RIGHT 0x0002
#define HSD_BUTTON_DPAD_DOWN 0x0004
#define HSD_BUTTON_DPAD_UP 0x0008
#define HSD_TRIGGER_Z 0x0010
#define HSD_TRIGGER_R 0x0020
#define HSD_TRIGGER_L 0x0040
#define HSD_BUTTON_A 0x0100
#define HSD_BUTTON_B 0x0200
#define HSD_BUTTON_X 0x0400
#define HSD_BUTTON_Y 0x0800
#define HSD_BUTTON_START 0x1000
#define HSD_BUTTON_UP 0x10000
#define HSD_BUTTON_DOWN 0x20000
#define HSD_BUTTON_LEFT 0x40000
#define HSD_BUTTON_RIGHT 0x80000

#define PAD_ERR_NONE 0
#define PAD_ERR_NO_CONTROLLER -1
#define PAD_ERR_NOT_READY -2
#define PAD_ERR_TRANSFER -3

enum DebugLevel
{
    DB_MASTER,        // off
    DB_NO_DEBUG_ROM,  //
    DB_DEBUG_DEVELOP, //
    DB_DEBUG_ROM,     //
    DB_DEVELOP,       //
};

typedef enum PauseKind
{
    PAUSEKIND_NONE,     // not paused
    PAUSEKIND_SYS,      // debug pause (uses Z to frame advance)
    PAUSEKIND_GAME,     // match pause (i dont think any other scene uses this?) allows p_links 0(sys),2,16,18(matchcam),19(misccam),20(hudcam),21(coincam),22(screenflashcam),24(devtext)+ to run
    PAUSEKIND_3,        // unknown what uses this, it whitelists everything
    PAUSEKIND_MATCHEND, // is used when the match ends, it allows p_links 0(sys),2,12(effect2),13(mapmisc),14(misc),15(hud),16,17,18(matchcam),19,20,21,22,24+ to run
    PAUSEKIND_5,        //
    PAUSEKIND_6,        //
    PAUSEKIND_7,        //
    PAUSEKIND_8,        //
} PauseKind;

/*** Structs ***/

struct HSD_IDEntry
{
    struct HSD_IDEntry *next; // 0x00
    u32 id;                   // 0x04
    void *data;               // 0x08
};

struct HSD_IDTable
{
    struct HSD_IDEntry *table[101];
};

struct HSD_ClassInfoHead
{
    void (*info_init)();   // 0x00
    u32 flags;             // 0x04, 0x01 = is initialized
    char *library_name;    // 0x08
    char *class_name;      // 0x0C
    s16 obj_size;          // 0x10
    s16 info_size;         // 0x12
    HSD_ClassInfo *parent; // 0x14
    HSD_ClassInfo *next;   // 0x18
    HSD_ClassInfo *child;  // 0x1C
    u32 nb_exist;          // 0x20
    u32 nb_peak;           // 0x24
};

struct _HSD_Class;

struct HSD_ClassInfo
{
    HSD_ClassInfoHead head;
    void *(*alloc)(struct HSD_ClassInfo *o);     // 0x28
    void (*init)(struct _HSD_Class *o);          // 0x2C
    void (*release)(struct _HSD_Class *o);       // 0x30
    void (*destroy)(struct _HSD_Class *o);       // 0x34
    void (*amnesia)(struct HSD_ClassInfo *info); // 0x38
};

typedef struct _HSD_ObjAllocLink
{
    struct _HSD_ObjAllocLink *next;
} HSD_ObjAllocLink;

struct HSD_ObjAllocData
{
    u32 flags;                     // 0x00 - Technically 2 diff flags
    HSD_ObjAllocLink *freehead;    // 0x04
    u32 used;                      // 0x08
    u32 free;                      // 0x0C
    u32 peak;                      // 0x10
    u32 num_limit;                 // 0x14
    u32 heap_limit_size;           // 0x18
    u32 heap_limit_num;            // 0x1C
    u32 size;                      // 0x20
    u32 align;                     // 0x24
    struct HSD_ObjAllocData *next; // 0x28
};

struct HSD_Material
{
    GXColor ambient;
    GXColor diffuse;
    GXColor specular;
    float alpha;
    float shininess;
};

struct HSD_Pad
{
    int held;            // 0x0
    int heldPrev;        // 0x4
    int down;            // 0x8
    int repeat;          // 0xc
    int up;              // 0x10
    int repeat_timer;    // 0x14
    s8 stickX;           // 0x18
    s8 stickY;           // 0x19
    s8 substickX;        // 0x1a
    s8 substickY;        // 0x1b
    u8 triggerLeft;      // 0x1c
    u8 triggerRight;     // 0x1d
    u8 triggerA;         // 0x1e
    u8 triggerB;         // 0x1f
    float fstickX;       // 0x20
    float fstickY;       // 0x24
    float fsubstickX;    // 0x28
    float fsubstickY;    // 0x2c
    float ftriggerLeft;  // 0x30
    float ftriggerRight; // 0x34
    float ftriggerA;     // 0x30
    float ftriggerB;     // 0x34
    u8 cross_dir;        // 0x40
    s8 error;            // 0x41   0 = plugged, -1 = unplugged
};

struct HSD_Pads
{
    HSD_Pad pad[4];
};

struct HSD_Update
{
    // 0x80479d58
    u32 sys_frames_pre;                   // 0x0
    u32 sys_frames_post;                  // 0x4
    u32 engine_frames;                    // 0x8
    u32 change_scene;                     // 0xC
    u8 pause_kind;                        // 0x10, see enum PauseKind
    unsigned char flag9 : 1;              // 0x11 - 0x80
    unsigned char flag10 : 1;             // 0x11 - 0x40
    unsigned char flag11 : 1;             // 0x11 - 0x20
    unsigned char flag12 : 1;             // 0x11 - 0x10
    unsigned char flag13 : 1;             // 0x11 - 0x08
    unsigned char flag14 : 1;             // 0x11 - 0x04
    unsigned char pause_game_prev : 1;    // 0x11 - 0x02
    unsigned char pause_develop_prev : 1; // 0x11 - 0x01
    unsigned char flag17 : 1;             // 0x12 - 0x80
    unsigned char flag18 : 1;             // 0x12 - 0x40
    unsigned char flag19 : 1;             // 0x12 - 0x20
    unsigned char flag20 : 1;             // 0x12 - 0x10
    unsigned char flag21 : 1;             // 0x12 - 0x08
    unsigned char flag22 : 1;             // 0x12 - 0x04
    unsigned char flag23 : 1;             // 0x12 - 0x02
    unsigned char advance : 1;            // 0x12 - 0x01
    unsigned char flag24 : 1;             // 0x12 - 0x80
    unsigned char flag25 : 1;             // 0x12 - 0x40
    unsigned char flag26 : 1;             // 0x12 - 0x20
    unsigned char flag27 : 1;             // 0x12 - 0x10
    unsigned char flag28 : 1;             // 0x12 - 0x08
    unsigned char flag29 : 1;             // 0x12 - 0x04
    unsigned char flag30 : 1;             // 0x12 - 0x02
    unsigned char advance_prev : 1;       // 0x12 - 0x01
    int (*checkPause)();                  // 0x14 returns 1 when toggling pause
    int (*checkAdvance)();                // 0x18 returns 1 when advancing frame
    u32 x1c;                              // 0x1C
    u64 plink_whitelist;                  // 0x20, code @ 801a4eac determines which gobj plinks to allow when changing the pause state.
    u32 x28;
    u32 x2c;
    void (*onFrame)(); // 0x30
};

struct HSD_VI
{
    int x0;
    int x4;
    int is_prog;
};

// struct HSD_Archive
// {
//     int file_size;       // size of file
//     int *reloc_offset;   // pointer to relocation table offset?
//     int reloc_num;       // number of entries on the rleoc table
//     int symbol_num;      // total number of symbols
//     int refsymbol_num;   // number of reference symbols
//     int archive_vers;    // idk for sure sometimes 001B
//     int unk1;            //
//     int unk2;            //
//     int *general_points; // 0x20 = pointer to the "general points"
//     int *reloc_table;    // pointer to relocation table in memory
//     int *symbols1;       // pointer to symbol pointers and name offsets
//     int *refsymmbols;    // pointer to reference symbol info in memory
//     int *symbols2;       // pointer to symbol list in memory
//     int *file_start;     // pointer to the header of the dat
// };

struct HSD_ArchiveHeader
{
    u32 file_size; /* 0x00 */
    u32 data_size; /* 0x04 */
    u32 nb_reloc;  /* 0x08 */
    u32 nb_public; /* 0x0C */
    u32 nb_extern; /* 0x10 */
    u8 version[4]; /* 0x14 */
    u32 pad[2];    /* 0x18 */
};

struct HSD_ArchiveRelocationInfo
{
    u32 offset;
};

struct HSD_ArchivePublicInfo
{
    u32 offset; /* 0x00 */
    u32 symbol; /* 0x04 */
};

struct HSD_ArchiveExternInfo
{
    u32 offset; /* 0x00 */
    u32 symbol; /* 0x04 */
};

struct HSD_Archive
{
    struct HSD_ArchiveHeader header;              /* 0x00 */
    u8 *data;                                     /* 0x20 */
    struct HSD_ArchiveRelocationInfo *reloc_info; /* 0x24 */
    struct HSD_ArchivePublicInfo *public_info;    /* 0x28 */
    struct HSD_ArchiveExternInfo *extern_info;    /* 0x2C */
    char *symbols;                                /* 0x30 */
    struct HSD_Archive *next;                     /* 0x34 */
    char *name;                                   /* 0x38 */
    u32 flags;                                    /* 0x3C */
    void *top_ptr;                                /* 0x40 */
};

typedef struct HSD_PadData {
    PADStatus stat[4];
} HSD_PadData;

typedef struct PadLibData {
    u8 qnum;
    u8 qread;
    u8 qwrite;
    u8 qcount;
    u8 qtype;
    HSD_PadData *queue;
    s32 repeat_start;
    s32 repeat_interval;
    u8 adc_type;
    s8 adc_th;
    f32 adc_angle;
    u8 clamp_stickType;
    u8 clamp_stickShift;
    s8 clamp_stickMax;
    s8 clamp_stickMin;
    u8 clamp_analogLRShift;
    u8 clamp_analogLRMax;
    u8 clamp_analogLRMin;
    u8 clamp_analogABShift;
    u8 clamp_analogABMax;
    u8 clamp_analogABMin;
    s8 scale_stick;
    u8 scale_analogLR;
    u8 scale_analogAB;
    u8 cross_dir;
    u8 reset_switch_status;
    u8 reset_switch;
    void *rumble_info;
} PadLibData;

/*** Static Variables ***/
static HSD_IDTable *stc_hsd_default_table = (void *)0x804C23EC;
static HSD_VI *stc_HSD_VI = (void *)0x8046b0f0;
static HSD_Update *stc_hsd_update = (void *)0x80479d58;
static int **stc_rng_seed = (void *)0x804D5F94;
static HSD_Pads *stc_engine_pads = (HSD_Pads *)0x804c21cc;
static HSD_Pads *stc_master_pads = (HSD_Pads *)0x804c1fac;
static u64 *stc_pause_plink_whitelists = (void *)0x803da888; // array of u64 bitfields defining which gobj p_links should run for the corresponding PauseKind
static PadLibData *stc_padlibdata = (void *)0x804c1f78;
static PadLibData *stc_default_padlibdata = (void *)0x80406dac;
static GXPixelFmt *stc_hsd_pixelfmt = (void *)0x804d76c8;

/*** Functions ***/

HSD_Archive *Archive_LoadFile(char *filename);
HSD_Archive *Archive_LoadInitReturnSymbol(char *filename, void *ptr, ...);                // input each symbol name pointer sequentially and terminate with 0;
void Archive_GetSections(HSD_Archive *archive, void *symbol_out, char *symbol_name, ...); // input each symbol name sequentially and terminate with 0;
void *Archive_GetPublicAddress(HSD_Archive *archive, char *symbol);
void Archive_Init(HSD_Archive *archive, void *file_data, int size);
void Archive_Free(HSD_Archive *archive);
char *Archive_GetExtern(HSD_Archive *archive, int index);                   // gets name of the nth symbol in the dat file
void Archive_LocateExtern(HSD_Archive *archive, char *symbols, void *addr); // relocates pointers to symbols
HSD_Archive *File_GetPreloadedFile(char *filename);
void Archive_LoadSync(char *filename, void *alloc, int *out_size);
char *Archive_AppendExtension(char *filename);
int HSD_Randi(int max);
float HSD_Randf();
void *HSD_MemAlloc(int size);
void HSD_Free(void *ptr);
void *HSD_ObjAlloc(HSD_ObjAllocData *obj_def);
void HSD_ObjFree(HSD_ObjAllocData *obj_def, void *obj);
void HSD_ClassDestroy(void *hsd_class);
void HSD_ImageDescCopyFromEFB(_HSD_ImageDesc *image_desc, int left, int top, int clear_efb); // must be called from a cobj callback!
void HSD_StartRender(int unk);
void HSD_SetSpeed(u64 speed);
void HSD_SetSpeedEasy(float mult);
void HSD_StateInvalidate(int flags);
void HSD_StateInitTev();
void HSD_StateInitDirect(GXVtxFmt vtxfmt, int render_flags);
void HSD_StateSetZMode(GXBool compare_enable, GXCompare func, GXBool update_enable);
void HSD_StateSetNumChans(u8 nChans);
void HSD_SetupChannel(void *unk);
void HSD_StateSetColorUpdate(GXBool update_enable); // This function enables or disables color-buffer updates when rendering into the Embedded Frame Buffer (EFB)
void HSD_StateSetAlphaUpdate(GXBool update_enable); // This function enables or disables alpha-buffer updates when rendering into the Embedded Frame Buffer (EFB)
void HSD_StateSetDstAlpha(GXBool enable, u8 alpha);
void HSD_StateSetAlphaCompare(GXCompare func0, u8 reference0,
                              GXAlphaOperation operation,
                              GXCompare func1, u8 reference1);
void HSD_StateSetBlendMode(GXBlendMode type, GXBlendFactor src_factor,
                           GXBlendFactor dst_factor, GXLogicOp op);
void HSD_StateSetCullMode(GXCullMode cull_mode);
void HSD_StateSetDither();
void HSD_StateSetLineWidth(u8 width, int tex_offsets);
void HSD_StateSetNumTevStages(u8 stages);
void HSD_StateSetNumTexGens(u8 nTexGens);
void HSD_StateSetPointSize();
void HSD_StateSetZCompLoc(GXBool enable);
void HSD_ClearVtxDesc();
void HSD_VICopyXFBASync(int unk);
int HSD_VIGetDrawDoneWaitingFlag();
int HSD_VIGetXFBDrawEnable();
void HSD_VICopyEFB2XFBPtr(void *, int, int);
int HSD_GXProject(COBJ *cobj, Vec3 *in, Vec3 *out, int unk);
void HSD_UpdateDiscAndCardStatus();
void HSD_PadFlushQueue(int);
void HSD_PadRenewStatus();
void HSD_PadRenewMasterStatus();
void HSD_PadRenewCopyStatus();
void HSD_PadRenewUpdateStruct();
void HSD_PadRumbleInterpret();
void HSD_VIPostRetraceCallback(int unk);
void GX_AllocImageData(_HSD_ImageDesc *image_desc, int width, int height, int fmt, int size); // image data buffer is stored to the image_desc
void GXTexModeSync();
void GXPixModeSync();
void GXInvalidateTexAll();
u64 Pad_GetDown(int pad);
u64 Pad_GetRapidHeld(int pad);
u64 Pad_GetHeld(int pad);
void Pad_Rumble(int pad, int unk, int strength, int duration); // make unk = 0
void Pad_RumbleStopAll();
void HSD_DumpHeapStat();                        // 80015df8
void HSD_DumpClassStat(int r3, int r4, int r5); // 80382854
void HSD_ObjDumpStat();                         // 803755f8
HSD_ObjAllocData *HSD_IDGetAllocData();
HSD_ObjAllocData *HSD_AObjGetAllocData();
HSD_ObjAllocData *HSD_FObjGetAllocData();
void HSD_IDInsertToTable(HSD_IDTable *id_table, u32 id, void *data);
void HSD_IDRemoveByIDFromTable(HSD_IDTable *id_table, u32 id);
void *HSD_IDGetDataFromTable(HSD_IDTable *id_table, u32 id, u8 *success);
#endif
