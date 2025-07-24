#ifndef MEX_H_COLLISION
#define MEX_H_COLLISION

#include "structs.h"
#include "datatypes.h"
#include "obj.h"

// ECB Flags
#define ECB_GROUND 0x18000
#define ECB_CEIL 0x6000
#define ECB_WALLLEFT 0xfc0
#define ECB_WALLRIGHT 0x3f

// Line Directions
#define LINE_GROUND 1
#define LINE_CEIL 2
#define LINE_WALLRIGHT 4
#define LINE_WALLLEFT 8

/*** Enums ***/
enum LineDirection
{
    LINEDIR_GROUND = 1 << 0,
    LINEDIR_CEIL = 1 << 1,
    LINEDIR_LEFTWALL = 1 << 2,
    LINEDIR_RIGHTWALL = 1 << 3,
};

/*** Structs ***/

struct ECBSize
{
    float topY;
    float botY;
    Vec2 left;
    Vec2 right;
};

struct CollData
{
    GOBJ *gobj;                  // 0x0
    Vec3 topN_Curr;              // 0x4
    Vec3 topN_CurrCorrect;       // 0x10
    Vec3 topN_Prev;              // 0x1c
    Vec3 topN_Proj;              // 0x28
    int flags1;                  // 0x34
    int coll_test;               // 0x38, is the ID of the most recent collision test for this object
    int ignore_line;             // 0x3c, ignores this line ID during collision
    int ledge_left;              // 0x40, ledge ID in contact with
    int ledge_right;             // 0x44, ledge ID in contact with
    int ignore_group;            // 0x48  ignores this line group during collision
    int check_group;             // 0x4c  checks only this line group during collision
    float weight;                // 0x50
    float cliffgrab_width;       // 0x54
    float cliffgrab_y_offset;    // 0x58
    float cliffgrab_height;      // 0x5c
    int x60;                     // 0x60
    int x64;                     // 0x64
    int x68;                     // 0x68
    int x6c;                     // 0x6c
    int x70;                     // 0x70
    int x74;                     // 0x74
    int x78;                     // 0x78
    int x7c;                     // 0x7c
    int x80;                     // 0x80
    Vec2 ecbCurr_top;            // 0x84
    Vec2 ecbCurr_bot;            // 0x8C
    Vec2 ecbCurr_right;          // 0x94
    Vec2 ecbCurr_left;           // 0x9C
    Vec2 ecbCurrCorrect_top;     // 0xA4
    Vec2 ecbCurrCorrect_bot;     // 0xAC
    Vec2 ecbCurrCorrect_right;   // 0xB4
    Vec2 ecbCurrCorrect_left;    // 0xBC
    Vec2 ecbPrev_top;            // 0xC4
    Vec2 ecbPrev_bot;            // 0xCC
    Vec2 ecbPrev_right;          // 0xD4
    Vec2 ecbPrev_left;           // 0xDC
    Vec2 ecbProj_top;            // 0xE4
    Vec2 ecbProj_bot;            // 0xEC
    Vec2 ecbProj_right;          // 0xF4
    Vec2 ecbProj_left;           // 0xFC
    int is_use_joints;           // 0x104, bool that signals to use the jobjs below for ecb edge positions
    JOBJ *joint_1;               // 0x108
    JOBJ *joint_2;               // 0x10c
    JOBJ *joint_3;               // 0x110
    JOBJ *joint_4;               // 0x114
    JOBJ *joint_5;               // 0x118
    JOBJ *joint_6;               // 0x11c
    JOBJ *joint_7;               // 0x120
    int x124;                    // 0x124
    int x128;                    // 0x128
    int x12c;                    // 0x12c
    int flags;                   // 0x130, for fighters 0x10 is ecb lock enabled
    int envFlags;                // 0x134
    int envFlags_prev;           // 0x138
    int x13c;                    // 0x13c
    Vec2 coll_pos;               // 0x140, only updates for ceiling and ground?
    int x148;                    // 0x148
    int ground_index;            // 0x14c, ground
    u8 ground_info;              // 0x150
    u8 ground_unk;               // 0x151
    u8 ground_type;              // 0x152, platform/ledgegrab
    u8 ground_mat;               // 0x153, grass/ice etc
    Vec3 ground_slope;           // 0x154
    int rightwall_index;         // 0x160
    u8 rightwall_info;           // 0x164
    u8 rightwall_unk;            // 0x165
    u8 rightwall_type;           // 0x166, platform/ledgegrab
    u8 rightwall_mat;            // 0x167, grass/ice etc
    Vec3 rightwall_slope;        // 0x168
    int leftwall_index;          // 0x174
    u8 leftwall_info;            // 0x178
    u8 leftwall_unk;             // 0x179
    u8 leftwall_type;            // 0x17A, platform/ledgegrab
    u8 leftwall_mat;             // 0x17B, grass/ice etc
    Vec3 leftwall_slope;         // 0x17C
    int ceil_index;              // 0x188
    u8 ceil_info;                // 0x18C
    u8 ceil_unk;                 // 0x18D
    u8 ceil_type;                // 0x18E, platform/ledgegrab
    u8 ceil_mat;                 // 0x18F, grass/ice etc
    Vec3 ceil_slope;             // 0x190
    union                        // 0x19c,
    {                            //
        int ecb_bot_lock_frames; // fighters use this. frames until ecb bottom offset is no longer locked
    } u;                         //
};

struct CollGroupDesc // exists in stage file
{
    u16 floor_start; // 0x0
    u16 floor_num;
    u16 ceil_start; // 0x4
    u16 ceil_num;
    u16 rwall_start; // 0x8
    u16 rwall_num;
    u16 lwall_start; // 0xC
    u16 lwall_num;
    u16 dyn_start; // 0x10
    u16 dyn_num;
    Vec2 area_min;  // 0x14
    Vec2 area_max;  // 0x1C
    u16 vert_start; // 0x24
    u16 vert_num;   // 0x26
};

struct CollGroup // runtime struct
{
    CollGroup *next;      // 0x0
    CollGroupDesc *desc;  // 0x4
    u16 x8 : 15;          // 0x8, unk
    u16 is_enabled : 1;   // 0x8, bit 0x01
    u16 xa;               // unk, padding maybe
    u16 ray_id;           // 0xC, id of the last raycast
    u16 xe;               // 0xE, flags
    Vec2 area_min;        // 0x10
    Vec2 area_max;        // 0x18
    JOBJ *jobj;           // 0x20
    void *cb_floor;       // 0x24
    void *map_data_floor; // 0x28
    void *cb_ceil;        // 0x2C
    void *map_data_ceil;  // 0x30
};

struct CollLineDesc // exists in stage file
{
    s16 vert_prev;          // 0x0
    s16 vert_next;          // 0x2
    s16 line_prev;          // 0x4
    s16 line_next;          // 0x6
    s16 line_prev_altgroup; // 0x8
    s16 line_next_altgroup; // 0xA
    u8 xc;
    u8 xd_1 : 1;     // 0xD, 0x80
    u8 xd_2 : 1;     // 0xD, 0x40
    u8 xd_3 : 1;     // 0xD, 0x20
    u8 disabled : 1; // 0xD, 0x10
    u8 is_left : 1;  // 0xD, 0x08
    u8 is_right : 1; // 0xD, 0x04
    u8 is_ceil : 1;  // 0xD, 0x02
    u8 is_floor : 1; // 0xD, 0x01
    u8 xe_1 : 1;     // 0xE, 0x80
    u8 xe_2 : 1;     // 0xE, 0x40
    u8 xe_3 : 1;     // 0xE, 0x20
    u8 xe_4 : 1;     // 0xE, 0x10
    u8 xe_5 : 1;     // 0xE, 0x08
    u8 is_drop : 1;  // 0xE, 0x04
    u8 is_ledge : 1; // 0xE, 0x02
    u8 is_unk : 1;   // 0xE, 0x01
    u8 material;     // 0xF,
};

struct CollLine // runtime struct
{
    CollLineDesc *desc; // 0x0, (previously info)
    u8 x4;              // 0x4
    u8 x5_x80 : 1;      // 0x5, 0x80
    u8 x5_x40 : 1;      // 0x5, 0x40
    u8 x5_x20 : 1;      // 0x5, 0x20
    u8 x5_x10 : 1;      // 0x5, 0x10
    u8 x5_x08 : 1;      // 0x5, 0x08
    u8 x5_x04 : 1;      // 0x5, 0x04
    u8 x5_x02 : 1;      // 0x5, 0x02
    u8 is_enabled : 1;  // 0x5, 0x01
    u8 x6;              // 0x6
    u8 x7 : 4;          // 0x7
    u8 is_rwall : 1;    // 0x7, 0x8
    u8 is_lwall : 1;    // 0x7, 0x4
    u8 is_ceil : 1;     // 0x7, 0x2
    u8 is_floor : 1;    // 0x7, 0x1
};

struct CollVert
{
    Vec2 pos_orig;
    Vec2 pos_curr;
    Vec2 pos_prev;
};

struct CollLineUnk
{
    int x0;
    s16 x4;
    s16 x6;
    Vec3 left;  // 0x8
    Vec3 right; // 0x14
};

struct CollDataStage
{
    Vec2 *verts;           // 0x0, array of vec2's
    int vert_num;          // 0x4
    CollLineDesc *lines;   // 0x8, array of line descs
    int line_num;          // 0xC
    u16 floor_start;       // 0x10
    u16 floor_num;         // 0x12
    u16 ceil_start;        // 0x14
    u16 ceil_num;          // 0x16
    u16 rwall_start;       // 0x18
    u16 rwall_num;         // 0x1a
    u16 lwall_start;       // 0x1c
    u16 lwall_num;         // 0x1e
    u16 dyn_start;         // 0x20
    u16 dyn_num;           // 0x22
    CollGroupDesc *groups; // 0x24
    int group_num;         // 0x28
};

struct CollLineConnection // runtime struct, is created @ 8005a7cc, static array of these for each line direction @ 80458e88
{
    CollLineConnection *next; // 0x0
    int x4;                   // 0x4
    Vec3 start_pos;           // 0x8
    Vec3 end_pos;             // 0x14
    int x20;                  // 0x20
    int x24;                  // 0x24
};

/*** Functions ***/
void Shield_CreateBubble(GOBJ *ft, ShieldDesc *desc, void *(on_hit)(GOBJ *fighter));
void Shield_HitShield(GOBJ *ft);
void GuardReflectInitIDK(GOBJ *ft);
void GuardOnInitIDK(GOBJ *ft);
void Animation_GuardAgain(GOBJ *ft);
void Coll_CopyPosToECBs(CollData *coll_data, Vec3 *pos);
void Coll_ECBCurrToPrev(CollData *coll_data);
void Coll_InitECB(CollData *coll_data);
void Coll_SetECBScale(CollData *coll_data, float scale1, float scale2, float scale3, float scale4);
int Coll_CheckLedge(CollData *coll_data);
float Coll_GetCurrGroundsFrictionMult(CollData *);
int ECB_CollGround_PassLedge(CollData *ecb, ECBSize *bones); // returns is touching ground bool
int ECB_CollGround3(CollData *ecb);
void ECB_CollAir(CollData *ecb, ECBSize *bones);
int ECB_CollAir2(CollData *ecb);
int ECB_CollAir3(CollData *ecb);
int ECB_CollAirCheckLedge(CollData *ecb);
int ECB_CollGround(CollData *ecb);
int ECB_StoreLedgeCheckDirection(CollData *ecb, int ledge_check_dir);
int GrColl_SearchLedgeLeft(CollData *coll_data, int *return_ledge_index);
int GrColl_SearchLedgeRight(CollData *coll_data, int *return_ledge_index);
void GrColl_GetGroundLineEndLeft(int floor_index, Vec3 *pos);                                                                                                                                    // returns the leftmost grounded coordinate of the inputted line index within its group                                                                                                                                 // this functon will crawl along the entire line sequence and find the end of the ledge
void GrColl_GetGroundLineEndRight(int floor_index, Vec3 *pos);                                                                                                                                   // returns the rightmost grounded coordinate of the inputted line index within its group
void GrColl_GetGroundLineEndLeft_AllGroups(int floor_index, Vec3 *pos);                                                                                                                          // returns the leftmost grounded coordinate of the inputted line index regardless of the group
void GrColl_GetGroundLineEndRight_AllGroups(int floor_index, Vec3 *pos);                                                                                                                         // returns the rightmost grounded coordinate of the inputted line index regardless of the group
int GrColl_RaycastGround(Vec3 *coll_pos, int *line_index, int *line_kind, Vec3 *unk1, int unk2, int unk3, int unk4, void *cb, float fromX, float fromY, float toX, float toY, float unk5); // make unk5
int GrColl_RaycastUnk(Vec3 *coll_pos, int *line_index, int *line_kind, Vec3 *direction, void *cb, void *unk2, float from_x, float from_y, float to_x, float to_y);                                   // unk = 0, unk2 = -1;
int GrColl_RaycastAll(Vec3 *coll_pos, int *line_index, int *line_kind, Vec3 *direction, void *cb, void *unk2, float from_x, float from_y, float to_x, float to_y);
int GrColl_CrawlGround(int line_index, Vec3 *pos, int *return_line, Vec3 *return_pos, int *return_flags, Vec3 *return_slope, float x_offset, float y_offset);                                    // returns bool for if position on line series exists
int GrColl_GetPosDifference(int line_index, Vec3 *pos, Vec3 *return_pos);
int GrColl_GetLineInfo(int line_index, Vec3 *r4, void *r5, int *flags, Vec3 *return_slope);
void GrColl_GetLineSlope(int line_index, Vec3 *return_slope);
int GrColl_CheckIfLineEnabled(int line_index);

static int *stc_colltest = R13_OFFSET(-0x51F4);
static CollGroup **stc_firstcollgroup = R13_OFFSET(-0x51DC);
static CollGroup **stc_collgroup = R13_OFFSET(-0x51E0);
static CollLine **stc_collline = R13_OFFSET(-0x51E4);
static CollVert **stc_collvert = R13_OFFSET(-0x51E8);
static CollDataStage **stc_colldata = R13_OFFSET(-0x51EC);
static CollLineConnection **stc_first_line_connect = (void *)0x80458e88; // array of 9

#endif
