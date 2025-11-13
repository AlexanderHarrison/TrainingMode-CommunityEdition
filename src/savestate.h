#ifndef SAVESTATE_V1_H
#define SAVESTATE_V1_H

#include "../MexTK/mex.h"

#define EVENT_DATASIZE 512

// VERSION 2 #####################################################

// TODO get rid of 'is_exist' fields, store counts instead

typedef struct IDInfo {
    // for locating gobj
    char p_link;
    char list_idx;
    
    // for locating jobj (unused for FighterData and GOBJ)
    char jobj_idx;
} IDInfo;

typedef union ID {
    void *ptr;
    IDInfo id;
} ID;

typedef struct VarPtr {
    u8 valid : 1;
    u8 index : 7;
} VarPtr;

typedef struct FtSaveStateData_v2 {
    int is_exist;
    int state_id;
    float facing_direction;
    float state_frame;
    float state_rate;
    float state_blend;
    struct phys phys;
    struct input input;
    CollData coll_data;
    int ecb_bot_lock_frames;
    int ledge_cooldown;
    ColorOverlay color[3];
    CmSubject camera_subject;
    ftHit hitbox[4];
    ftHit throw_hitbox[2];
    ftHit thrown_hitbox;
    struct flags flags;
    union fighter_var fighter_var;
    union state_var state_var;
    struct ftcmd_var ftcmd_var;
    struct dmg dmg;
    struct grab grab;
    CPU cpu;
    struct wall wall;
    float jab2_timer;
    struct item item;
    struct jump jump;
    struct smash smash;
    struct hurt hurt;
    struct shield shield;
    struct shield_bubble shield_bubble;
    struct reflect_bubble reflect_bubble;
    struct absorb_bubble absorb_bubble;
    struct reflect_hit reflect_hit;
    struct absorb_hit absorb_hit;
    struct cb cb;

    VarPtr fighter_var_ptrs[8];
    VarPtr state_var_ptrs[8];
} FtSaveStateData_v2;

typedef struct FtSaveState_v2 {
    FtSaveStateData_v2 data[2];
    Playerblock playerblock;
    int stale_queue[11];
} FtSaveState_v2;

// enum StageSaveStateTag_v2 {
//     STSV2_StadiumTransformation = 1,
// };

// typedef struct StageSaveState_v2 {
//     u16 tag;
//     union {
//     };
// } StageSaveState_v2;

typedef struct GOBJSaveState_v2 {
    short entity_class;
    char p_link;
    char gx_link;
    char p_priority;
    char gx_pri;
    char obj_kind;
    char data_kind;
    void (*gx_cb)(GOBJ*, int);
    u64 cobj_links;
    void *destructor_function;

    void *proc[12];
    char proc_s_link[12];
    char proc_flags[12];
} GOBJSaveState_v2;

typedef struct JOBJSaveState_v2 {
    Vec4 rot;
    Vec3 scale;
    Vec3 trans;
} JOBJSaveState_v2;

typedef struct ItemSaveState_v2 {
    int is_exist;
    
    GOBJSaveState_v2 gobj;
    JOBJSaveState_v2 jobj;

    JOBJ *attached;
    JOBJ *attached_to;

    VarPtr var_ptrs[16]; // ItemData var GOBJ ptrs
    ItemData data;
} ItemSaveState_v2;

typedef struct ItemLinkSaveState_v2 {
    int is_exist;
    GOBJSaveState_v2 gobj;
    JOBJSaveState_v2 jobj;
    GOBJ *parent_fighter;
    GOBJ *parent_item;
    int parent_item_attributes_jobj_desc_idx;
    ItemLinkData data;
} ItemLinkSaveState_v2;

typedef struct Savestate_v2
{
    int is_exist;
    int frame;
    u8 event_data[EVENT_DATASIZE];
    FtSaveState_v2 ft_state[6];
    ItemSaveState_v2 item_state[8];
    ItemLinkSaveState_v2 item_link_state[32];
    
    // StageSaveState_v2 *stage_state;
} Savestate_v2;

// VERSION 1 #####################################################
    
typedef struct FtSaveStateData_v1
{
    int is_exist;
    int state_id;
    float facing_direction;
    float state_frame;
    float state_rate;
    float state_blend;
    // Legacy - no longer used now that we update the animation when restoring state.
    Vec4 XRotN_rot; // XRotN
    struct phys phys;
    ColorOverlay color[3];
    struct input input;
    CollData coll_data;
    int ecb_bot_lock_frames;
    struct
    {
        void *alloc;            // 0x0
        CmSubject *next;        // 0x4
        int kind;               // 0x8 1 = disabled, 2 = only focus if close to center
        u8 flags_x80 : 1;       // 0xC 0x80
        u8 is_disable : 1;      // 0xC 0x40
        Vec3 cam_pos;           // 0x10
        Vec3 bone_pos;          // 0x1c
        float direction;        // 0x28
        float boundleft_curr;   // 0x2C
        float boundright_curr;  // 0x30
        float boundtop_curr;    // 0x34
        float boundbottom_curr; // 0x38
        float x3c;              // 0x3c
        float boundleft_proj;   // 0x40
        float boundright_proj;  // 0x44
        float boundtop_proj;    // 0x48
        float boundbottom_proj; // 0x4c
        // the savestate CmSubject struct is missing one field compared
        // to the CmSubject struct defined in MexTK
    } camera_subject;
    ftHit hitbox[4];
    ftHit throw_hitbox[2];
    ftHit thrown_hitbox;
    struct flags flags;                // 0x2210
    union fighter_var fighter_var;    // 0x222c
    union state_var state_var;        // 0x2340
    struct ftcmd_var ftcmd_var;        // 0x2200
    struct
    {
        int behavior;                  // 0x182c
        float percent;                 // 0x1830
        int x1834;                     // 0x1834
        float percent_temp;            // 0x1838
        int applied;                   // 0x183c
        int x1840;                     // 0x1840
        FtDmgLog hit_log;              // 0x1844, info regarding the last solid hit
        FtDmgLog tip_log;              // 0x1870, info regarding the last phantom hit
        float tip_hitlag;              // 0x189c, hitlag is stored here during phantom hits @ 8006d774
        float tip_force_applied;       // 0x18a0, used to check if a tip happened this frame
        float kb_mag;                  // 0x18a4  kb magnitude
        int x18a8;                     // 0x18a8
        int time_since_hit;            // 0x18ac   in frames
        float armor_unk;               // 0x18b0, is prioritized over the armor below
        float armor;                   // 0x18b4, used by yoshi double jump
        Vec2 vibrate_offset;           // 0x18b8
        int x18c0;                     // 0x18c0
        int source_ply;                // 0x18c4   damage source ply number
        int x18c8;                     // 0x18c8
        int x18cc;                     // 0x18cc
        int x18d0;                     // 0x18d0
        int x18d4;                     // 0x18d4
        int x18d8;                     // 0x18d8
        int x18dc;                     // 0x18dc
        int x18e0;                     // 0x18e0
        int x18e4;                     // 0x18e4
        int x18e8;                     // 0x18e8
        u16 atk_instance_hurtby;       // 0x18ec. Last Attack Instance This Player Was Hit by,
        int x18f0;                     // 0x18f0
        int x18f4;                     // 0x18f4
        u8 vibrate_index;              // 0x18f8, which dmg vibration values to use
        u8 x18f9;                      // 0x18f9
        u16 vibrate_timer;             // 0x18fa
        u8 vibrate_index_cur;          // 0x18fc, index of the current offset
        u8 vibrate_offset_num;         // 0x18fd, number of different offsets for this dmg vibration index
        Vec2 ground_slope;             // 0x1900
        int x1908;                     // 0x1908
        void *random_sfx_table;        // 0x190c, contains a ptr to an sfx table when requesting a random sfx
        int x1910;                     // 0x1910
        int x1914;                     // 0x1914
        struct                         // aka clank
        {                              //
            int dmg_dealt;             // 0x1918,
            float dmg_based_rate_mult; // 0x191c, updated @ 80077aec (when hitting projectile). is equal to 3 + (damageDealt * 0.3)
            float dir;                 // 0x1920, direction the rebound/clank occured in
        } rebound;                     //
        int x1924;                     // 0x1924
        int x1928;                     // 0x1928
        int x192c;                     // 0x192c
        struct                         // 0x1930
        {                              //
            Vec3 pos_prev;             // 0x1930
            Vec3 pos_cur;              // 0x193c
        } footstool;                   //
        int x1948;                     // 0x1948
        int x194c;                     // 0x194c
        int x1950;                     // 0x1950
        float x1954;                   // 0x1954,
        float hitlag_env_frames;       // 0x1958, Environment Hitlag Counter (used for peachs castle switches)
        float hitlag_frames;           // 0x195c
        // the savestate dmg struct is missing two fields compared
        // to the dmg struct defined in MexTK
    } dmg;
    struct grab grab;                  // 0x1a4c
    struct jump jump;                  // 0x1968
    struct smash smash;                // 0x2114
    struct hurt hurt;                  // 0x1988
    struct shield shield;
    struct shield_bubble shield_bubble;   // 0x19c0
    struct reflect_bubble reflect_bubble; // 0x19e4
    struct absorb_bubble absorb_bubble;   // 0x1a08
    struct reflect_hit reflect_hit;       // 0x1a2c
    struct absorb_hit absorb_hit;         // 0x1a40
    struct cb cb;
} FtSaveStateData_v1;

typedef struct FtSaveState_v1
{
    FtSaveStateData_v1 data[2];
    struct
    {
        int state;           // 0x00 = not present, 0x02 = HMN, 0x03 = CPU
        int c_kind;          // external ID
        int p_kind;          // (0x00 is HMN, 0x01 is CPU, 0x02 is Demo, 0x03 n/a)
        u8 isTransformed[2]; // 0xC, 1 indicates the fighter that is active
        Vec3 tagPos;         // 0x10
        Vec3 spawnPos;       // 0x1C
        Vec3 respawnPos;     // 0x28
        int x34;
        int x38;
        int x3C;
        float initialFacing; // 0x40
        u8 costume;          // 0x44
        u8 color_accent;     // 0x45, will correspond to port color when ffa and team color when teams
        u8 tint;             // 0x46
        u8 team;             // 0x47
        u8 controller;       // 0x48
        u8 cpuLv;            // 0x49
        u8 cpuKind;          // 0x4a
        u8 handicap;         // 0x4b
        u8 x4c;              // 0x4c
        u8 kirby_copy;       // 0x4d, index of kirby copy ability
        u8 x4e;              // 0x4e
        u8 x4f;              // 0x4f
        float attack;        // 0x50
        float defense;       // 0x54
        float scale;         // 0x58
        u16 damage;          // 0x5c
        u16 initialDamage;   // 0x5e
        u16 stamina;         // 0x60
        int falls[2];        // 0x68
        int ko[6];           // 0x70
        int x88;
        u16 selfDestructs;
        u8 stocks;
        int coins_curr;
        int coins_total;
        int x98;
        int x9c;
        int stickSmashes[2];
        int tag;
        u8 xac_80 : 1;        // 0xac, 0x80
        u8 is_multispawn : 1; // 0xac, 0x40
        u8 xac_3f : 6;        // 0xac, 0x3f
        u8 xad;               // 0xad
        u8 xae;               // 0xae
        u8 xaf;               // 0xaf
        GOBJ *gobj[2];        // 0xb0
        // Omitting Playerblock fields recently added to MexTK
    } player_block;
    int stale_queue[11];
} FtSaveState_v1;

typedef struct Savestate_v1
{
    int is_exist;
    int frame;
    u8 event_data[EVENT_DATASIZE];
    FtSaveState_v1 ft_state[6];
} Savestate_v1;

enum savestate_flags {
    // save: perform initial safety checks 
    Savestate_Checks = 1 << 0,
    
    // save/load: don't play sfx and screen flash 
    Savestate_Silent = 1 << 1,
    
    // load: mirror savestate
    Savestate_Mirror = 1 << 2,
};

int Savestate_Save_v1(Savestate_v1 *savestate, int flags);
int Savestate_Load_v1(Savestate_v1 *savestate, int flags);
int Savestate_Save_v2(Savestate_v2 *savestate, int flags);
int Savestate_Load_v2(Savestate_v2 *savestate, int flags);

#endif
