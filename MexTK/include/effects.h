#ifndef MEX_H_EFFECTS
#define MEX_H_EFFECTS

#include "structs.h"
#include "datatypes.h"
#include "obj.h"

#define PTCL_LINKMAX 16

/*** Structs ***/

struct EffectModelDesc
{
    float frame_num;
    JOBJSet jobjset;
};

struct PtclDesc
{
    u16 x0;
    u16 x2;
    int effect_idx_start;
    int gen_num;
    struct
    {
        int x0;
        int x4;
        int flags; // 0x8
    } *gen[];
};

struct TexGDesc
{
    int num;
    struct
    {
        int img_num;
        GXTexFmt img_fmt;
        int x8;
        int width;
        int height;
        int x14;
        void *img_ptr[];
    } *data[];
};

struct EffectDataTable // exists in the dat
{
    PtclDesc *particle;
    TexGDesc *texg;
    EffectModelDesc model_desc[];
};

struct Effect
{
    GOBJ *child;
    GOBJ *gobj;
    GOBJ *parent;
    int xc;
    void *callback;
    int x14;
    int x18;
    int x1c;
    float x20;
    short lifetime;
    char x26;
    char x27;
    char x28; // if this is == 2, the effect is not updated
    char x29;
};

struct ptclGenCallback
{
    void (*cbSpawnParticle)(Particle *);   // x00
    void (*cbDestroyParticle)(Particle *); // x04
};

struct ptclGen // allocated at 8039d9c8
{
    struct ptclGen *next;       // 0x0
    int kind;                   // x4
    float random;               // x8
    float num_to_spawn;         // xc
    JOBJ *joint;                // x10
    u16 genlife;                // x14
    u16 type;                   // x16
    u8 ef_file;                 // x18
    u8 link_no;                 // x19, r3 for 8039f05c
    u8 tex_group;               // x1a
    u8 x1b;                     // x1b
    u16 instance;               // x1c, is equal to idnum value in the particles it creates
    u16 life;                   // x1e
    void *cmdList;              // x20, pointer to track data
    Vec3 pos;                   // x24
    Vec3 vel;                   // x30
    float gravity;              // x3c
    float friction;             // x40
    float size;                 // x44
    float radius;               // x48
    float angle;                // x4c
    int particle_num;           // x50
    GeneratorAppSRT *appsrt;    // x54, points to an SRT mtx used for transforming the generator while attached to a joint
    ptclGenCallback *callbacks; // x58
    void *x5C;                  // x5C
    Vec3 x60;                   // x60
    Vec3 x6c;                   // x6C
    Vec3 x78;                   // x78
    Vec3 x84;                   // x84
    short rect_flags;           // x90
};

struct GeneratorAppSRT // allocated at 803a42b0
{
    int x0;     // x0
    int x4;     // x4
    Vec3 pos;   // x8
    Vec4 rot;   // x14
    Vec3 scale; // x24
    int x30;    // x30
    int x34;    // x34
    int x38;    // x38
    int x3c;    // x3c
    int x40;    // x40
    int x44;    // x44
    int x48;    // x48
    int x4c;    // x4c
    int x50;    // x50
    int x54;    // x54
    int x58;    // x58
    int x5c;    // x5c
    int x60;    // x60
    int x64;    // x64
    int x68;    // x68
    int x6c;    // x6c
    int x70;    // x70
    int x74;    // x74
    int x78;    // x78
    int x7c;    // x7c
    int x80;    // x80
    int x84;    // x84
    int x88;    // x88
    int x8c;    // x8c
    int x90;    // x90
    int x94;    // x94
    int x98;    // x98
    int x9c;    // x9c
    int xa0;    // xa0
    u16 xa2;
};

// courtesy of psilupan: https://pastebin.com/raw/yQdjypW0
struct Particle // created at 80398c90. dont feel like labelling this, offsets are @ 80398de4
{
    struct Particle *next; // 0x0
    u32 kind;              // 0x4, actually are flags. 0x800 pauses it
    u8 bank;               // 0x8
    u8 texGroup;           // 0x9
    u8 poseNum;            // 0xa
    u8 palNum;             // 0xb
    u16 sizeCount;         // 0xc
    u16 primColCount;      // 0xe
    u16 envColCount;       // 0x10
    u8 primCol[4];         // 0x12
    u8 envCol[4];          // 0x16
    u16 cmdWait;           // 0x1a
    u8 loopCount;          // 0x1c
    u8 linkNo;             // 0x1d
    u16 idnum;             // 0x1e
    void *cmdList;         // 0x20
    u16 cmdPtr;            // 0x24
    u16 cmdMarkPtr;        // 0x26
    u16 cmdLoopPtr;        // 0x28
    u16 life;              // 0x2a
    Vec3 v;                // 0x2C
    float grav;            // 0x38
    float fric;            // 0x3C
    Vec3 pos;              // 0x40
    float size;            // 0x4C
    float rotate;          // 0x50
    u16 aCmpCount;         // 0x54
    u8 aCmpMode;           // 0x56
    u8 aCmpParam1;         // 0x57
    u8 aCmpParam2;         // 0x58
    u8 pJObjOfs;           // 0x59
    u16 matColCount;       // 0x5A
    u16 ambColCount;       // 0x5C
    u16 rotateCount;       // 0x5E
    float sizeTarget;      // 0x60
    float rotateTarget;    // 0x64
    float rotateAcc;       // 0x68
    u16 primColRemain;     // 0x6C
    u16 envColRemain;      // 0x6E
    GXColor primColTarget; // 0x70
    GXColor envColTarget;  // 0x74
    u16 matColRemain;      // 0x78
    u16 ambColRemain;      // 0x7A
    u16 aCmpRemain;        // 0x7C
    u8 aCmpParam1Target;   // 0x7D
    u8 aCmpParam2Target;   // 0x7E
    u8 matRGB;             // 0x7F
    u8 matA;               // 0x80
    u8 ambRGB;             // 0x81
    u8 ambA;               // 0x82
    // u8 matRGBTarget;         // 0x83
    // u8 matATarget;           // 0x84
    // u8 ambRGBTarget;         // 0x85
    // u8 ambATarget;           // 0x86
    float trail;             // 0x84
    ptclGen *gen;            // 0x88
    GeneratorAppSRT *appsrt; // 0x8C
    void *userdata;          // 0x90
    void *callback;          // 0x94
};

/*** Functions ***/

void Effect_LoadFile(int eff_file_idx);
Effect *Effect_SpawnSync(int gfx_id, ...);
void Effect_SpawnAsync(GOBJ *fighter, Effect *ptr, int type, int gfx_id, ...);
void Effect_SpawnAsyncLookup(GOBJ *gobj, int gfx_id, int bone, int unk, int destroy_on_leave, Vec3 *offset, Vec3 *range);
void Effect_SpawnItEffectLookup(GOBJ *gobj, int gfx_id, int bone, Vec3 *offset, Vec3 *scatter, int unk3);
void Effect_SpawnItEffect(GOBJ *gobj, int gfx_id);
void Effect_DestroyAll(GOBJ *fighter);
void Particle_DestroyAll(JOBJ *jobj);
void Effect_PauseAll(GOBJ *fighter);
void Effect_ResumeAll(GOBJ *fighter);
void Effect_CheckQueue(GOBJ *g, Effect **gfx);
void Particle_InitFile(void *ptcl, void *texg, int r5);
ptclGen *psCreateGeneratorID(int linkno, int bank_no, int ptcl_index);                 // 8039f05c
ptclGen *psCreateGeneratorIDJObj(int linkno, int bank_no, int ptcl_index, JOBJ *jobj); // 8039efac
void psInterpretParticles(u32 blacklist_link_nos);                                     // input is a bitfield, shifted left by 16 bits!
void psExecGenerator(u32 blacklist_link_nos);                                          // input is a bitfield, shifted left by 16 bits!
void psDispParticles(u32 whitelist_link_nos, int pass);                                // input is a bitfield
GeneratorAppSRT *psAddGeneratorAppSRT(ptclGen *ptcl_gen, int unk);
int psRemoveParticleAppSRT(Particle *ptcl);
void psDeletePntJObjwithParticle(Particle *ptcl);
ptclGen *psKillGenerator(ptclGen *gen, ptclGen *unk);
ptclGen *psKillGeneratorEZ(ptclGen *gen);
void psInitDataBanks(int bank_no, void *ptcl, void *texg, int r6, int r7);

static u16 *stc_ptclnum = R13_OFFSET(-0x3DBE);      // number of pctls alive
static Particle **stc_ptcl = (void *)0x804d0908;        // last created ptcl
static ptclGen **stc_ptclgen = R13_OFFSET(-0x3DA4); // last created gen
static ptclGen **stc_ptclgencurr = R13_OFFSET(-0x3DA8);
static u16 *stc_ptclgennum = R13_OFFSET(-0x3DC0);

#endif
