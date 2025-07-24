#ifndef MEX_H_STAGE
#define MEX_H_STAGE

#include "color.h"
#include "structs.h"
#include "datatypes.h"
#include "hsd.h"
#include "obj.h"

// MapDesc Flags
#define map_isCObj 0x20000000
#define map_isBG 0x40000000
#define map_isUnk 0x80000000

typedef enum GrInternal
{
    GRKIND_DUMMY,
    GRKIND_TEST,
    GRKIND_CASTLE,
    GRKIND_RCRUISE,
    GRKIND_KONGO,
    GRKIND_GARDEN,
    GRKIND_GREATBAY,
    GRKIND_SHRINE,
    GRKIND_ZEBES,
    GRKIND_KRAID,
    GRKIND_STORY,
    GRKIND_YOSTER,
    GRKIND_IZUMI,
    GRKIND_GREENS,
    GRKIND_CORNERIA,
    GRKIND_VENOM,
    GRKIND_PSTAD,
    GRKIND_PURA,
    GRKIND_MUTECITY,
    GRKIND_BIGBLUE,
    GRKIND_ONETT,
    GRKIND_FOURSIDE,
    GRKIND_ICEMT,
    GRKIND_ICETOP,
    GRKIND_MK1,
    GRKIND_MK2,
    GRKIND_AKANEIA,
    GRKIND_FLATZONE,
    GRKIND_OLDPU,
    GRKIND_OLDSTORY,
    GRKIND_OLDKONGO,
    GRKIND_ADVKRAID,
    GRKIND_ADVSHRINE,
    GRKIND_ADVZR,
    GRKIND_ADVBR,
    GRKIND_ADVTE,
    GRKIND_BATTLE,
    GRKIND_FD,
    // target test stages in between this
    GRKIND_ALLSTARHEAL = 66,
    GRKIND_HOMERUN,
    GRKIND_TROPHY1,
    GRKIND_TROPHY2,
    GRKIND_TROPHY3,
} GrInternal;
typedef enum GrExternal
{
    GRKINDEXT_DUMMY,
    GRKINDEXT_TEST,
    GRKINDEXT_IZUMI,
    GRKINDEXT_PSTAD,
    GRKINDEXT_CASTLE,
    GRKINDEXT_KONGO,
    GRKINDEXT_ZEBES,
    GRKINDEXT_CORNERIA,
    GRKINDEXT_STORY,
    GRKINDEXT_ONETT,
    GRKINDEXT_MUTECITY,
    GRKINDEXT_RCRUISE,
    GRKINDEXT_GARDEN,
    GRKINDEXT_GREATBAY,
    GRKINDEXT_SHRINE,
    GRKINDEXT_KRAID,
    GRKINDEXT_YOSTER,
    GRKINDEXT_GREENS,
    GRKINDEXT_FOURSIDE,
    GRKINDEXT_MK1,
    GRKINDEXT_MK2,
    GRKINDEXT_AKANEIA,
    GRKINDEXT_VENOM,
    GRKINDEXT_PURA,
    GRKINDEXT_BIGBLUE,
    GRKINDEXT_ICEMT,
    GRKINDEXT_ICETOP,
    GRKINDEXT_FLATZONE,
    GRKINDEXT_OLDPU,
    GRKINDEXT_OLDSTORY,
    GRKINDEXT_OLDKONGO,
    GRKINDEXT_BATTLE,
    GRKINDEXT_FD,
} GrExternal;

typedef enum MapRenderKind
{
    MAP_RENDERKIND_FG,      // foreground, rendered third
    MAP_RENDERKIND_FGTRANS, // foreground with transparency, rendered second
    MAP_RENDERKIND_BG,      // background, rendered first
    MAP_RENDERKIND_HIGHPRI, // rendered above everything (fighters effects stage etc)
    MAP_RENDERKIND_NUM,
} MapRenderKind;

/*** Structs ***/

struct MapDesc
{
    void (*onCreation)(GOBJ *map);
    int (*onUnk)(GOBJ *map);
    void (*onFrame)(GOBJ *map);
    void (*onDeletion)(GOBJ *map);
    unsigned char is_lobj : 1; //  0x80
    unsigned char is_fog : 1;  //  0x40
    unsigned char is_cobj : 1; //  0x20
};

struct MapData
{
    int x0;                            // 0x0
    GOBJ *gobj;                        // 0x4
    void (*post_anim_cb)(GOBJ *map);   // 0x8, this cb is fired after the stage animation is updated
    int xC;                            // 0xC
    unsigned char flagx80 : 1;         // 0x10, 0x80
    unsigned char flagx40 : 1;         // 0x10, 0x40
    unsigned char is_fog : 1;          // 0x10, 0x20, checked @ 801c5e80 and 801c5f10
    unsigned char flagx10 : 1;         // 0x10, 0x10
    unsigned char flagx8 : 1;          // 0x10, 0x08
    unsigned char is_check_shadow : 1; // 0x10, 0x04, will run the "shadow check" per fighter to see if a fighter's shadow should be drawn
    unsigned char flagx2 : 1;          // 0x10, 0x02
    unsigned char flagx1 : 1;          // 0x10, 0x01
    unsigned char render_kind : 3;     // 0x11, 0xE0, compares this to the current match render kind and only renders if they match
    unsigned char flag2x10 : 1;        // 0x11, 0x10
    unsigned char flag2x08 : 1;        // 0x11, 0x08
    unsigned char flag2x04 : 1;        // 0x11, 0x04, will run the "shadow check" per fighter to see if a fighter's shadow should be drawn
    unsigned char flag2x02 : 1;        // 0x11, 0x02
    unsigned char flag2x01 : 1;        // 0x11, 0x01
    int index;                         // 0x14, map_gobj index
    GOBJ *camera_gobj;                 // 0x18
    void *OnDestroyCB;                 // 0x1c
    int live_sfx[8];                   // 0x20, sfx ID's currently playing
    ColorOverlay color;                // 0x40
    int xc0;                           // 0xc0
    // pretty sure map_var actually starts here? rofl
    int xc4; // 0xc4
    int xc8; // 0xc8
    int xcc; // 0xcc
    int xd0; // 0xd0
    int xd4; // 0xd4
    int xd8; // 0xd8
    int xdc; // 0xdc
    struct
    {
        int mapVar0; // 0xe0
        int mapVar1; // 0xe4
        int mapVar2; // 0xe8
        int mapVar3; // 0xec
        int mapVar4; // 0xf0
        int mapVar5; // 0xf4
        int mapVar6; // 0xf8
        int mapVar7; // 0xfc
        int x100;    // 0x100
        int x104;    // 0x104
        int x108;    // 0x108
        int x10c;    // 0x10c
        int x110;    // 0x110
        int x114;    // 0x114
        int x118;    // 0x118
        int x11c;    // 0x11c
        int x120;    // 0x120
        int x124;    // 0x124
        int x128;    // 0x128
        int x12c;    // 0x12c
        int x130;    // 0x130
        int x134;    // 0x134
        int x138;    // 0x138
        int x13c;    // 0x13c
        int x140;    // 0x140
        int x144;    // 0x144
        int x148;    // 0x148
        int x14c;    // 0x14c
        int x150;    // 0x150
        int x154;    // 0x154
        int x158;    // 0x158
        int x15c;    // 0x15c
        int x160;    // 0x160
        int x164;    // 0x164
        int x168;    // 0x168
        int x16c;    // 0x16c
        int x170;    // 0x170
        int x174;    // 0x174
        int x178;    // 0x178
        int x17c;    // 0x17c
        int x180;    // 0x180
        int x184;    // 0x184
        int x188;    // 0x188
        int x18c;    // 0x18c
        int x190;    // 0x190
        int x194;    // 0x194
        int x198;    // 0x198
        int x19c;    // 0x19c
        int x1a0;    // 0x1a0
        int x1a4;    // 0x1a4
        int x1a8;    // 0x1a8
        int x1ac;    // 0x1ac
        int x1b0;    // 0x1b0
        int x1b4;    // 0x1b4
        int x1b8;    // 0x1b8
        int x1bc;    // 0x1bc
        int x1c0;    // 0x1c0
        int x1c4;    // 0x1c4
        int x1c8;    // 0x1c8
        int x1cc;    // 0x1cc
        int x1d0;    // 0x1d0
        int x1d4;    // 0x1d4
        int x1d8;    // 0x1d8
        int x1dc;    // 0x1dc
        int x1e0;    // 0x1e0
        int x1e4;    // 0x1e4
        int x1e8;    // 0x1e8
        int x1ec;    // 0x1ec
        int x1f0;    // 0x1f0
        int x1f4;    // 0x1f4
        int x1f8;    // 0x1f8
        int x1fc;    // 0x1fc
        int x200;    // 0x200
    } map_var;
};

struct StageOnGO
{
    StageOnGO *next;
    GOBJ *map_gobj;
    void *cb;
};

struct grGroundParam
{
    float scale;                // 0x0
    u8 x4;                      // 0x4
    u8 shadow_alpha;            // 0x5
    u8 x6;                      // 0x6
    u8 x7;                      // 0x7
    u16 fov;                    // 0x8
    int cam_distance_min;       // 0xc
    int cam_distance_max;       // 0x10
    int tilt_scale;             // 0x14
    float y_rotation;           // 0x18
    float x_rotation;           // 0x1c
    float fixedness;            // 0x20
    float bubble_mult;          // 0x24
    float cam_smoothness;       // 0x28
    u16 x2c;                    // 0x2c
    u16 x2e;                    // 0x2e
    int pause_min_z;            // 0x30
    int pause_default_z;        // 0x34
    int pause_max_z;            // 0x38
    float pause_tilt_max_up;    // 0x3c
    float pause_tilt_max_down;  // 0x40
    float pause_tilt_max_left;  // 0x44
    float pause_tilt_max_right; // 0x48
    float x4c;                  // 0x4c
    float cam_fixed_x;          // 0x50
    float cam_fixed_y;          // 0x54
    float cam_fixed_z;          // 0x58
    float cam_fixed_fov;        // 0x5c
    float cam_fixed_angle_y;    // 0x60
    float cam_fixed_angle_x;    // 0x64
    u16 x68;                    // 0x68
    u16 item_rates[35];         // 0x6a
    void *bgm_data;             // 0xb0
    int bgm_num;                // 0xb4
    int xb8;                    // 0xb8
    int xbc;                    // 0xbc
    int xc0;                    // 0xc0
    int xc4;                    // 0xc4
    int xc8;                    // 0xc8
    int xcc;                    // 0xcc
    int xd0;                    // 0xd0
    int xd4;                    // 0xd4
    int xd8;                    // 0xd8
};

struct Stage
{
    float cambound_left;                                                  // 0x0
    float cambound_right;                                                 // 0x4
    float cambound_top;                                                   // 0x8
    float cambound_bottom;                                                // 0xc
    Vec2 cambound_offset;                                                 // 0x10, offset of the camera bounds. begins checking for crowd gasps below this Y component
    float fov_d;                                                          // 0x18
    float fov_u;                                                          // 0x1c
    float fov_r;                                                          // 0x20, actually horizontal rotation?
    float fov_l;                                                          // 0x24
    float x28;                                                            // 0x28
    float x2c;                                                            // 0x2c
    float x30;                                                            // 0x30
    float x34;                                                            // 0x34, camera distance min
    float x38;                                                            // 0x38
    float x3c;                                                            // 0x3c
    float x40;                                                            // 0x40
    float x44;                                                            // 0x44
    float x48;                                                            // 0x48
    float x4c;                                                            // 0x4c
    float x50;                                                            // 0x50
    float x54;                                                            // 0x54
    float x58;                                                            // 0x58
    Vec3 fixed_cam_pos;                                                   // 0x5c
    float x68;                                                            // 0x68
    float x6c;                                                            // 0x6c
    float x70;                                                            // 0x70
    float blastzoneLeft;                                                  // 0x74
    float blastzoneRight;                                                 // 0x78
    float blastzoneTop;                                                   // 0x7c
    float blastzoneBottom;                                                // 0x80
    u8 x84_80 : 1;                                                        // 0x84
    u8 x84_40 : 1;                                                        // 0x84
    u8 x84_20 : 1;                                                        // 0x84
    u8 x84_10 : 1;                                                        // 0x84
    u8 x84_08 : 1;                                                        // 0x84
    u8 x84_04 : 1;                                                        // 0x84
    u8 x84_02 : 1;                                                        // 0x84
    u8 x84_01 : 1;                                                        // 0x84
    u8 x85;                                                               // 0x85
    u8 x86;                                                               // 0x86
    u8 x87_80 : 1;                                                        // 0x87
    u8 is_end_temple : 1;                                                 // 0x87, 0x40
    u8 is_end_targets : 1;                                                // 0x87, 0x20
    u8 is_end_mush : 1;                                                   // 0x87, 0x10
    u8 x87_08 : 1;                                                        // 0x87
    u8 x87_04 : 1;                                                        // 0x87
    u8 x87_02 : 1;                                                        // 0x87, 0x02
    u8 x87_01 : 1;                                                        // 0x87
    GrInternal kind;                                                      // 0x88
    u8 flags2x80 : 1;                                                     // 0x8c
    u8 flags2x40 : 1;                                                     // 0x8c
    u8 flags2x20 : 1;                                                     // 0x8c
    u8 flags2x10 : 1;                                                     // 0x8c
    u8 flags2x08 : 1;                                                     // 0x8c
    u8 flags2x04 : 1;                                                     // 0x8c
    u8 end_check_mush : 1;                                                // 0x8c, 0x02
    u8 end_check_temple : 1;                                              // 0x8c, 0x01
    int (*OnEnterEndGame1Check)(Vec3 *f_pos, int genpoint_index);         // 0x90
    int (*OnEnterEndGame2Check)(Vec3 *f_pos, int genpoint_index);         // 0x94
    int hpsID;                                                            // 0x98
    int x9c;                                                              // 0x9c
    int xa0;                                                              // 0xa0
    int xa4;                                                              // 0xa4
    int xa8;                                                              // 0xa8
    int xac;                                                              // 0xac
    int xb0;                                                              // 0xb0
    int xb4;                                                              // 0xb4
    int xb8;                                                              // 0xb8
    int xbc;                                                              // 0xbc
    int xc0;                                                              // 0xc0
    int xc4;                                                              // 0xc4
    int xc8;                                                              // 0xc8
    int xcc;                                                              // 0xcc
    int xd0;                                                              // 0xd0
    int xd4;                                                              // 0xd4
    int xd8;                                                              // 0xd8
    int xdc;                                                              // 0xdc
    int xe0;                                                              // 0xe0
    int xe4;                                                              // 0xe4
    int xe8;                                                              // 0xe8
    int xec;                                                              // 0xec
    int xf0;                                                              // 0xf0
    int xf4;                                                              // 0xf4
    int xf8;                                                              // 0xf8
    int xfc;                                                              // 0xfc
    int x100;                                                             // 0x100
    int x104;                                                             // 0x104
    int x108;                                                             // 0x108
    int x10c;                                                             // 0x10c
    int x110;                                                             // 0x110
    int x114;                                                             // 0x114
    int x118;                                                             // 0x118
    int x11c;                                                             // 0x11c
    int x120;                                                             // 0x120
    int x124;                                                             // 0x124
    int x128;                                                             // 0x128
    int x12c;                                                             // 0x12c
    int x130;                                                             // 0x130
    int x134;                                                             // 0x134
    int x138;                                                             // 0x138
    int x13c;                                                             // 0x13c
    int x140;                                                             // 0x140
    int x144;                                                             // 0x144
    int x148;                                                             // 0x148
    int x14c;                                                             // 0x14c
    int x150;                                                             // 0x150
    int x154;                                                             // 0x154
    int x158;                                                             // 0x158
    int x15c;                                                             // 0x15c
    int x160;                                                             // 0x160
    int x164;                                                             // 0x164
    int x168;                                                             // 0x168
    int x16c;                                                             // 0x16c
    int x170;                                                             // 0x170
    int x174;                                                             // 0x174
    int x178;                                                             // 0x178
    void (*OnShadowRender)(Vec3 *fighter_pos, int unk, JOBJ *stage_jobj); // 0x17c
    GOBJ *map_gobjs[64];
    JOBJ *general_points[256];    // 0x280
    int x680;                     // 0x680
    int x684;                     // 0x684
    int x688;                     // 0x688
    int x68c;                     // 0x68c
    int x690;                     // 0x690
    COBJ *cobj[4];                // 0x694, indexed @ 801c2bd4
    StageOnGO *on_go;             // 0x6a4
    MapItemDesc **itemdata;       // 0x6a8
    CollDataStage *coll_data;     // 0x6ac
    grGroundParam *grGroundParam; // 0x6b0
    int *ALDYakuAll;              // 0x6b4
    int *map_ptcl;                // 0x6b8
    int *map_texg;                // 0x6bc
    void *yakumono_param;         // 0x6c0
    int *map_plit;                // 0x6c4
    int *x6c8;                    // 0x6c8
    void *quake_model_set;        // 0x6cc
    s16 x6d0;                     // 0x6d0
    s16 targets_hit;              // 0x6d2
    s16 targets_left;             // 0x6d4
    int x6d8;                     // 0x6d8
    int x6dc;                     // 0x6dc
    int x6e0;                     // 0x6e0
    int x6e4;                     // 0x6e4
    int x6e8;                     // 0x6e8
    int x6ec;                     // 0x6ec
    int x6f0;                     // 0x6f0
    int x6f4;                     // 0x6f4
    int x6f8;                     // 0x6f8
    int x6fc;                     // 0x6fc
    int x700;                     // 0x700
    int x704;                     // 0x704
    int x708;                     // 0x708
    float endgame1_boundwidth;    // 0x70c, used for mush king
    float endgame1_boundheight;   // 0x710, used for mush king
    int endgame1_genpoint;        // 0x714, general point that ended the game, used for mush king
    float endgame2_boundwidth;    // 0x718, used for temple
    float endgame2_boundheight;   // 0x71c, used for temple
    int endgame2_genpoint;        // 0x720, general point that ended the gamem, used for temple
    int x724;                     // 0x724
    int x728;                     // 0x728
    struct
    {
        GOBJ *gobj; // 0x72c, points to the map gobj
        Vec3 pos;   // 0x730, current position of hazard
        float x73c; // 0x73c, unk
    } catch;
    int x740; // 0x740, unk, used by onett cars only? 801c5784
};

struct GeneralPoints
{
    u16 jobj_index;
    u16 kind;
};

struct GeneralPointsInfo
{
    JOBJDesc *jobj_desc;
    GeneralPoints *general_point;
    int num;
};

struct MapHead
{
    GeneralPointsInfo *general_points_info; // 0x0
    int general_points_num;                 // 0x4
    MapGObjDesc *map_gobj_desc;             // 0x8, pointer to array of map_gobj_desc's
    int map_gobj_desc_num;                  // 0xC
    void *splines;                          // 0x10
    int splines_num;                        // 0x14
    void *lights;                           // 0x18
    int lights_num;                         // 0x1C
    void *splines_desc;                     // 0x20
    int splines_desc_num;                   // 0x24
    MOBJ **mobj;                            // 0x28
    int mobj_num;                           // 0x2c
};

struct MapCollLink
{
    s16 coll_group;
    s16 unk;
    s16 jobj_index;
};

struct MapGObjDesc
{
    JOBJSet jobjset;         // 0x0
    COBJDesc *cobj;          // 0x10
    void *x14;               // 0x14
    void *lobj;              // 0x18
    void *fog_desc;          // 0x1c
    MapCollLink *coll_links; // 0x20
    int coll_links_num;      // 0x24
    u8 *anim_behave;         // 0x28, points to an array of u8 that will override some of the animations behvaior (like enabling loop). executes the code @ 801c82a8
    void *coll_links2;       // 0x2c
    int coll_links2_num;     // 0x30
};

struct StageFile
{
    HSD_Archive *archive; //
    MapHead *map_head;    //
    int xc;               // is_secondary? transformations use 1 for this
};

struct GrDesc
{
    int internal_id;
    MapDesc *map_desc;
    char *filename;
    void (*onInit)();
    void *x10;
    void (*onLoad)();
    void (*onGo)();
    void *x1c;
    void (*LineDamageCheck)(int line_id);
    void *x24;
    int x28;
    void *x2c;
    int x30;
};

struct GrExtLookup
{
    int internal_id;
    int x4;
    int x8;
};

struct LineHazardDesc
{
    int x0;
    int dmg;
    int angle;
    int kb_growth;
    int x10;
    int kb;
    int element;
    int x1c;
    int sfx;
};

struct LineRange
{
    struct
    {
        float top;
        float bottom;
        float left;
        float right;
    } unk;
    struct
    {
        float top;
        float bottom;
        float left;
        float right;
    } ground;
};

struct MapItemDesc
{
    int index;
    ItemDesc *desc;
};

static Stage *stc_stage = (void *)0x8049e6c8;
static int *ftchkdevice_windnum = R13_OFFSET(-0x5128);
static int *ftchkdevice_grabnum = R13_OFFSET(-0x512C);
static int *ftchkdevice_dmgnum = R13_OFFSET(-0x5130);
static int *stc_gr_ext_cur = (void *)0x804d49e8;
static GrExtLookup *stc_gr_lookup_cur = (void *)0x804d49ec;
static LineRange *stc_line_range = (void *)0x80458868;
static GOBJ **stc_stage_hud_gobj = (void *)0x804d6d80; // points to a gobj that gets rendered to the hud camera

/*** Functions ***/
int Stage_GetRandomExternalID();
StageFile *Stage_GetStageFiles();                 // returns an array of StageFiles
StageFile *Stage_GetStageFile(int mapgobj_index); // returns the StageFile the ID belongs to
void Stage_AddFtChkDevice(GOBJ *map, int hazard_kind, void *check);
void Stage_SetChkDevicePos(float y_pos);
void Stage_GetChkDevicePos(float *y_pos, float *y_delta);
float Stage_GetScale();
int *Stage_GetYakumonoParam();
void Stage_SetMapJOBJAnim(GOBJ *map, int jobj_index, int flags, int anim_id, float start_frame, float rate);
void Stage_MapStateChange(GOBJ *map, int map_gobjID, int anim_id);
int Stage_CheckAnimEnd(GOBJ *map, int jobj_index, int flags);  // 0x1 = unk aobj, 0x2 = material aobj, 0x4 = unk aobj
int Stage_CheckAnimEnd2(GOBJ *map, int jobj_index, int flags); // 0x1 = unk aobj, 0x2 = material aobj, 0x4 = unk aobj
void Stage_PlaySFX(MapData *map, int live_index, int sfx_id);
int Stage_CheckSFX(MapData *map, int live_index);
GOBJ *Stage_CreateMapGObj(int mapgobjID);
GOBJ *Stage_CreateMapGObjDefineIndex(JOBJ *jobjdesc, int map_index);
void Stage_DestroyMapGObj(GOBJ *map_gobj);
void *GXLink_Stage(GOBJ *gobj, int pass);
GOBJ *Stage_GetMapGObj(int mapgobjID);
JOBJ *Stage_GetMapGObjJObj(GOBJ *mapgobj, int jointIndex);
int Stage_GetLinesGroup(int line);
int Stage_GetLinesUnk(int line);
int Stage_GetLinesDirection(int line);
void Stage_SetGroundCallback(int group, void *userdata, void (*OnStand)(MapData *mp, int group_index, CollData *coll_data, int weight, int time));
void Stage_ClearGroundCallback(int group);
void Stage_SetCeilingCallback(int group, void *userdata, void *callback);
void Stage_InitMovingColl(JOBJ *mapjoint, int mapgobjID);
void Stage_UpdateMovingColl(GOBJ *mapgobj);
void Stage_GetSpawnPosition(int spawn_id, Vec3 *pos);
Particle *Stage_SpawnEffectPos(int gfxID, int efFileID, Vec3 *pos);
Particle *Stage_SpawnEffectJointPos(int gfxID, int efFileID, JOBJ *pos);
Particle *Stage_SpawnEffectJointPos2(int gfxID, int efFileID, JOBJ *pos);
GOBJ *Zako_Create(int item_id, Vec3 *pos, JOBJ *jobj, Vec3 *velocity, int isMovingItem);
GOBJ *Stage_CreateMapItem(MapData *map_data, int takeDamageSFXKind, int state, JOBJ *joint, Vec3 *pos, int unk_bool, void *onGiveDamage, void *onTakeDamage); // this function creates an item of id 0xA0, its a generic ID used across multiple stages. its mainly used for giving a joint a hurtbox/hitbox and an onTakeDamage callback.
int Stage_CheckForNearbyFighters(Vec3 *pos, float radius);
ptclGen *Stage_CreatePtclGen(int ptcl_index, int bank_index, Vec3 *pos);
float Stage_GetBlastzoneRight();
float Stage_GetBlastzoneLeft();
float Stage_GetBlastzoneTop();
float Stage_GetBlastzoneBottom();
float Stage_GetCameraRight();
float Stage_GetCameraLeft();
float Stage_GetCameraTop();
float Stage_GetCameraBottom();
int Stage_GetGeneralPoint(int index, Vec3 *pos);
void Stage_EnableLineGroup(int index);
void Stage_DisableLineGroup(int index);
void Stage_AutoLinkLineGroups();
void Stage_LinkLineGroups(int group1, int group2);
void Stage_InitLines(CollDataStage *coll_data);
void Stage_InitCatchHazard(GOBJ *map, int unk, void *check_cb);
void Stage_InitMoveHazard(GOBJ *map, int unk, int (*check_cb)(GOBJ *m, GOBJ *f, Vec3 *queued_velocity)); // returns is_apply_velocity
void Stage_InitDamageHazard(GOBJ *map, int unk, void *check_cb);
void Stage_InitLineHazardDescUnk(void *unk, LineHazardDesc *hazard_desc); // 0x80008d30
void Stage_InitColAnim(JOBJ *map_jobj);
void Stage_ApplyColAnim(GOBJ *map, ColAnimDesc *colanim);
void Stage_DisableColAnim(GOBJ *map);
void Stages_MovingCollisionPointsUpdate(int moving_collision_idx);
int Stage_GetExternalID();
int Stage_ExternalToInternal(int ext_id);
void Stage_GetLeftOfLineCoordinates(int ledge_id, Vec3 *pos_out);
void Stage_GetRightOfLineCoordinates(int ledge_id, Vec3 *pos_out);
#endif

