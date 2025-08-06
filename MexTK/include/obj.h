#ifndef MEX_H_OBJ
#define MEX_H_OBJ

#include "structs.h"
#include "datatypes.h"
#include "gx.h"

// JObj Flags
#define JOBJ_SKELETON (1 << 0)             // 0x00000001
#define JOBJ_SKELETON_ROOT (1 << 1)        // 0x00000002
#define JOBJ_ENVELOPE_MODEL (1 << 2)       // 0x00000004
#define JOBJ_CLASSICAL_SCALING (1 << 3)    // 0x00000008
#define JOBJ_HIDDEN (1 << 4)               // 0x00000010
#define JOBJ_PTCL (1 << 5)                 // 0x00000020
#define JOBJ_MTX_DIRTY (1 << 6)            // 0x00000040
#define JOBJ_LIGHTING (1 << 7)             // 0x00000080
#define JOBJ_TEXGEN (1 << 8)               // 0x00000100
#define JOBJ_BILLBOARD (1 << 9)            // 0x00000200
#define JOBJ_VBILLBOARD (2 << 9)           // 0x00000400
#define JOBJ_HBILLBOARD (3 << 9)           // 0x00000600
#define JOBJ_RBILLBOARD (4 << 9)           // 0x00000800
#define JOBJ_INSTANCE (1 << 12)            // 0x00001000
#define JOBJ_PBILLBOARD (1 << 13)          // 0x00002000
#define JOBJ_SPLINE (1 << 14)              // 0x00004000
#define JOBJ_FLIP_IK (1 << 15)             // 0x00008000
#define JOBJ_SPECULAR (1 << 16)            // 0x00010000
#define JOBJ_USE_QUATERNION (1 << 17)      // 0x00020000
#define JOBJ_OPA (1 << 18)                 // 0x00040000 only rendered with gx pass 3
#define JOBJ_XLU (1 << 19)                 // 0x00080000
#define JOBJ_TEXEDGE (1 << 20)             // 0x00100000
#define JOBJ_NULL (0 << 21)                // 0x00000000
#define JOBJ_JOINT1 (1 << 21)              // 0x00100000
#define JOBJ_JOINT2 (2 << 21)              // 0x00200000
#define JOBJ_EFFECTOR (3 << 21)            // 0x00300000
#define JOBJ_USER_DEFINED_MTX (1 << 23)    // 0x00800000
#define JOBJ_MTX_INDEPEND_PARENT (1 << 24) // 0x01000000
#define JOBJ_MTS_INDEPEND_SRT (1 << 25)    // 0x02000000
#define JOBJ_GENERALFLAG (1 << 26)         // 0x04000000
#define JOBJ_GENERALFLAG2 (1 << 27)        // 0x08000000
#define JOBJ_ROOT_OPA (1 << 28)            // 0x10000000 only rendered with gx pass 3
#define JOBJ_ROOT_XLU (1 << 29)            // 0x20000000
#define JOBJ_ROOT_TEXEDGE (1 << 30)        // 0x40000000
#define JOBJ_31 (1 << 31)                  // 0x80000000

// MObj Flags
#define HSD_A_M_AMBIENT_R 1
#define HSD_A_M_AMBIENT_G 2
#define HSD_A_M_AMBIENT_B 3
#define HSD_A_M_DIFFUSE_R 4
#define HSD_A_M_DIFFUSE_G 5
#define HSD_A_M_DIFFUSE_B 6
#define HSD_A_M_SPECULAR_R 7
#define HSD_A_M_SPECULAR_G 8
#define HSD_A_M_SPECULAR_B 9
#define HSD_A_M_ALPHA 10
#define HSD_A_M_PE_REF0 11
#define HSD_A_M_PE_REF1 12
#define HSD_A_M_PE_DSTALPHA 13
#define RENDER_DIFFUSE_SHIFT 0
#define RENDER_DIFFUSE_BITS (3 << RENDER_DIFFUSE_SHIFT)
#define RENDER_DIFFUSE_MAT0 (0 << RENDER_DIFFUSE_SHIFT)
#define RENDER_DIFFUSE_MAT (1 << RENDER_DIFFUSE_SHIFT)
#define RENDER_DIFFUSE_VTX (2 << RENDER_DIFFUSE_SHIFT)
#define RENDER_DIFFUSE_BOTH (3 << RENDER_DIFFUSE_SHIFT)
#define RENDER_CONSTANT (1 << 0) // 0x00000001
#define RENDER_VERTEX (1 << 1)   // 0x00000002
#define RENDER_DIFFUSE (1 << 2)  // 0x00000004
#define RENDER_SPECULAR (1 << 3) // 0x00000008
#define CHANNEL_FIELD (RENDER_CONSTANT | RENDER_VERTEX | RENDER_DIFFUSE | RENDER_SPECULAR)
#define RENDER_TEX0 (1 << 4)  // 0x00000010
#define RENDER_TEX1 (1 << 5)  // 0x00000020
#define RENDER_TEX2 (1 << 6)  // 0x00000040
#define RENDER_TEX3 (1 << 7)  // 0x00000080
#define RENDER_TEX4 (1 << 8)  // 0x00000100
#define RENDER_TEX5 (1 << 9)  // 0x00000200
#define RENDER_TEX6 (1 << 10) // 0x00000400
#define RENDER_TEX7 (1 << 11) // 0x00000800
#define RENDER_TEXTURES (RENDER_TEX0 | RENDER_TEX1 | RENDER_TEX2 | RENDER_TEX3 | RENDER_TEX4 | RENDER_TEX5 | RENDER_TEX6 | RENDER_TEX7)
#define RENDER_TOON (1 << 12)                         // 0x00001000
#define RENDER_ALPHA_SHIFT 13                         //
#define RENDER_ALPHA_BITS (3 << RENDER_ALPHA_SHIFT)   // 0x00006000
#define RENDER_ALPHA_COMPAT (0 << RENDER_ALPHA_SHIFT) //
#define RENDER_ALPHA_MAT (1 << RENDER_ALPHA_SHIFT)    // 0x00002000
#define RENDER_ALPHA_VTX (2 << RENDER_ALPHA_SHIFT)    // 0x00004000
#define RENDER_ALPHA_BOTH (3 << RENDER_ALPHA_SHIFT)   // 0x00006000
#define RENDER_SHADOW (1 << 26)                       // 0x04000000
#define RENDER_ZMODE_ALWAYS (1 << 27)                 // 0x08000000
#define RENDER_NO_ZUPDATE (1 << 29)                   // 0x20000000
#define RENDER_XLU (1 << 30)                          // 0x40000000

// DOBJ flags
#define DOBJ_HIDDEN (1 << 0)           // 0x00000001
#define DOBJ_RENDER_ORDER_UNK (1 << 2) // 0x00000004

// POBJ flags
#define POBJ_ANIM (1 << 3)
#define POBJ_SKIN (0 << 12)
#define POBJ_SHAPEANIM (1 << 12)
#define POBJ_ENVELOPE (2 << 12)
#define POBJ_CULLFRONT (1 << 14)
#define POBJ_CULLBACK (1 << 15)

// AOBJ flags
#define AOBJ_REWINDED (1 << 26)   // 0x04000000
#define AOBJ_FIRST_PLAY (1 << 27) // 0x08000000
#define AOBJ_NO_UPDATE (1 << 28)  // 0x10000000
#define AOBJ_LOOP (1 << 29)       // 0x20000000
#define AOBJ_NO_ANIM (1 << 30)    // 0x40000000

// LOBJ flags
#define LOBJ_AMBIENT (0 << 0)
#define LOBJ_INFINITE (1 << 0)
#define LOBJ_POINT (2 << 0)
#define LOBJ_SPOT (3 << 0)
#define LOBJ_DIFFUSE (1 << 2)
#define LOBJ_SPECULAR (1 << 3)
#define LOBJ_ALPHA (1 << 4)
#define LOBJ_HIDDEN (1 << 5)
#define LOBJ_RAW_PARAM (1 << 6)
#define LOBJ_DIFF_DIRTY (1 << 7)
#define LOBJ_SPEC_DIRTY (1 << 8)

// COBJ flags
#define COBJ_UP_VECTOR_UNK (1 << 0) // 0x00000001, related to initing the up vector on cobj load (8036a440)
#define COBJ_MTX_DIRTY (1 << 1)     // 0x00000002, updates the view mtx when this flag is lowered during COBJSetCurrent @ 80368564
#define COBJ_40000000 (1 << 30)     // 0x40000000, is checked during CObjMtxIsDirty (8036959c)
#define COBJ_80000000 (1 << 31)     // 0x80000000, is raised when the COBJSetCurrent returns

#define PROJ_PERSPECTIVE 1
#define PROJ_FRUSTRUM 2
#define PROJ_ORTHO 3

// Anim flags (used for JOBJ_XByFlags)
#define JOBJ_ANIM 0x1
#define MOBJ_ANIM 0x4
#define TOBJ_ANIM 0x10
#define ALL_ANIM 0x7FF

// Macro
#define JOBJ_PauseOnFrame(jobj, child_index, flags, frame)                    \
    {                                                                         \
        JOBJ *this_jobj;                                                      \
        if (child_index != 0)                                                 \
            JOBJ_GetChild(jobj, &this_jobj, child_index, -1);                 \
        else                                                                  \
            this_jobj = jobj;                                                 \
        JOBJ_ForEachAnim(this_jobj, 6, flags, AOBJ_ReqAnim, 1, (float)frame); \
        JOBJ_AnimAll(this_jobj);                                              \
        JOBJ_ForEachAnim(this_jobj, 6, flags, AOBJ_StopAnim, 6, 0, 0);        \
    }
#define JOBJ_PlayOnFrame(jobj, child_index, flags, frame)                     \
    {                                                                         \
        JOBJ *this_jobj;                                                      \
        if (child_index != 0)                                                 \
            JOBJ_GetChild(jobj, &this_jobj, child_index, -1);                 \
        else                                                                  \
            this_jobj = jobj;                                                 \
        JOBJ_ForEachAnim(this_jobj, 6, flags, AOBJ_ReqAnim, 1, (float)frame); \
        JOBJ_AnimAll(this_jobj);                                              \
    }
#define JOBJ_GetChildPosition(jobj, child_index, pos)         \
    {                                                         \
        JOBJ *this_jobj;                                      \
        if (child_index != 0)                                 \
            JOBJ_GetChild(jobj, &this_jobj, child_index, -1); \
        else                                                  \
            this_jobj = jobj;                                 \
        JOBJ_GetWorldPosition(this_jobj, 0, pos);             \
    }

typedef enum ForEachAnimFlag //  (used for JOBJ_ForEachAnim)
{
    AOBJFLAG_JOBJ = 0x1,
    AOBJFLAG_MOBJ = 0x80,
    AOBJFLAG_TOBJ = 0x400,
    AOBJFLAG_ALL = 0x7FF,
} ForEachAnimFlag;

typedef enum HSD_ObjKind
{
    HSD_OBJKIND_NONE = 0,
    HSD_OBJKIND_COBJ,
    HSD_OBJKIND_LOBJ,
    HSD_OBJKIND_JOBJ,
    HSD_OBJKIND_FOG
} HSD_ObjKind;

/*** Structs ***/

struct HSD_Obj
{
    HSD_ClassInfo *parent;    // 0x0
    s16 ref_count;            // 0x4
    s16 ref_count_individual; // 0x6
};

struct GOBJ
{
    short entity_class;      // 0x0
    char p_link;             // 0x2, used to classify similar gobjs together.
    char gx_link;            // 0x3. 0-63 are gx. 64+ are reserved for camera objects
    char p_priority;         // 0x4
    char gx_pri;             // 0x5
    char obj_kind;           // 0x6
    char data_kind;          // 0x7
    GOBJ *next;              // 0x8
    GOBJ *previous;          // 0xC
    GOBJ *nextOrdered;       // 0x10
    GOBJ *previousOrdered;   // 0x14
    GOBJProc *proc;          // 0x18
    void (*gx_cb)(GOBJ* gobj, int code);         // 0x1C
    u64 cobj_links;          // 0x20. this is used to know which gobjs to render
    void *hsd_object;        // 0x28
    void *userdata;          // 0x2C
    int destructor_function; // 0x30
    int unk_linked_list;     // 0x34
};

struct GOBJProc
{
    GOBJ *parent;
    GOBJProc *next;
    GOBJProc *prev;
    char s_link;     // 0xC
    char x0d_80 : 1; // 0xD
    char x0d_40 : 1;
    char update_idx : 2;
    char x0d_08 : 1;
    char x0d_04 : 1;
    char x0d_02 : 1;
    char x0d_01 : 1;
    GOBJ *parentGOBJ;       // 0x10
    void (*cb)(GOBJ *gobj); // function callback
};

struct GXList
{
    // pointed to @ -0x3e80(r13)
    GOBJ *gx_render[63]; // pointer to 63 gobjs
    GOBJ *gx_camera;     // pointer to the highest priority cobj gobj. they are linked together via the next member.
};

struct TOBJ
{
    HSD_Obj parent;
    TOBJ *next;
    u32 id;                           // GXTexMapID
    u32 src;                          // GXTexGenSrc 0x10
    u32 mtxid;                        // 0x14
    Vec4 rotate;                      // 0x18
    Vec3 scale;                       // 0x28
    Vec3 translate;                   // 0x34
    u32 wrap_s;                       // 0x40 GXTexWrapMode
    u32 wrap_t;                       // 0x44 GXTexWrapMode
    u8 repeat_s;                      // 0x48
    u8 repeat_t;                      // 0x49
    u16 anim_id;                      // 0x4A
    u32 flags;                        // 0x4C
    f32 blending;                     // 0x50
    u32 magFilt;                      // 0x54 GXTexFilter
    struct _HSD_ImageDesc *imagedesc; // 0x58
    struct _HSD_Tlut *tlut;           // 0x5C
    struct _HSD_TexLODDesc *lod;      // 0x60
    AOBJ *aobj;                       // 0x64
    struct _HSD_ImageDesc **imagetbl; // 0x68
    struct _HSD_Tlut **tluttbl;       // 0x6C
    int tlut_no;                      // 0x70
    Mtx mtx;                          // 0x74
    u32 coord;                        // GXTexCoordID
    struct _HSD_TObjTev *tev;
};

typedef struct _HSD_AObjDesc
{
    u32 flags;                      // 0x00
    f32 end_frame;                  // 0x04
    struct _HSD_FObjDesc *fobjdesc; // 0x08
    u32 obj_id;                     // 0x0C
} HSD_AObjDesc;

struct AOBJ
{
    u32 flags;                // 0x0
    f32 curr_frame;           // 0x4
    f32 rewind_frame;         // 0x8
    f32 end_frame;            // 0xC
    f32 framerate;            // 0x10
    struct _HSD_FObj *fobj;   // 0x14
    struct _HSD_Obj *hsd_obj; // 0x18, points to spline data
};

struct MOBJ
{
    int *parent;
    u32 rendermode;
    TOBJ *tobj;             // 0x8
    HSD_Material *mat;      // 0xC
    struct _HSD_PEDesc *pe; // 0x10
    AOBJ *aobj;             // 0x14
    /*
    struct _HSD_TObj *ambient_tobj;
    struct _HSD_TObj *specular_tobj;
    */
    struct _HSD_TExpTevDesc *tevdesc;
    union _HSD_TExp *texp;

    struct _HSD_TObj *tobj_toon;
    struct _HSD_TObj *tobj_gradation;
    struct _HSD_TObj *tobj_backlight;
    f32 z_offset;
};

struct JOBJDesc
{
    char *class_name;       // 0x00
    u32 flags;              // 0x04
    struct JOBJDesc *child; // 0x08
    struct JOBJDesc *next;  // 0x0C
    union
    {
        struct _HSD_DObjDesc *dobjdesc;
        struct _HSD_Spline *spline;
        struct _HSD_SList *ptcl;
    } u;                            // 0x10
    Vec3 rotation;                  // 0x14 - 0x1C
    Vec3 scale;                     // 0x20 - 0x28
    Vec3 position;                  // 0x2C - 0x34
    Mtx mtx;                        // 0x38
    struct _HSD_RObjDesc *robjdesc; // 0x3C
};

struct MatAnimDesc
{
    MatAnimDesc *next;
    HSD_AObjDesc *material_aobj;
    void *texture_anim;
    int is_render_anim;
};

struct MatAnimJointDesc
{
    MatAnimJointDesc *child;
    MatAnimJointDesc *next;
    MatAnimDesc *matanim;
};

struct AnimJointDesc
{
    AnimJointDesc *child;
    AnimJointDesc *next;
    void *aobj;
    int flags;
    int flags2;
};

struct WOBJDesc
{
    char *class_name;
    Vec3 pos;                       // 0x04
    struct _HSD_RObjDesc *robjdesc; // 0x10
    WOBJDesc *next;
};

struct COBJDesc
{
    char *class_name;        // 0x00
    u16 flags;               // 0x04
    u16 projection_type;     // 0x06
    u16 viewport_left;       // 0x08
    u16 viewport_right;      // 0x0A
    u16 viewport_top;        // 0x0C
    u16 viewport_bottom;     // 0x0E
    u32 scissor_lr;          // 0x10
    u32 scissor_tb;          // 0x14
    WOBJDesc *eye_desc;      // 0x18
    WOBJDesc *interest_desc; // 0x1C
    f32 roll;                // 0x20
    Vec3 *vector;            // 0x24
    f32 near;                // 0x28
    f32 far;                 // 0x2C
    union
    {
        struct
        {
            f32 fov;
            f32 aspect;
        } perspective;

        struct
        {
            f32 top;
            f32 bottom;
            f32 left;
            f32 right;
        } frustrum;

        struct
        {
            f32 top;
            f32 bottom;
            f32 left;
            f32 right;
        } ortho;
    } projection_param;
};

struct HSD_VtxDescList
{
    GXAttribute attr;             // 0x0
    GXAttributeType attr_type;    // 0x4
    GXComponentContents comp_cnt; // 0x8
    GXComponentType comp_type;    // 0xc
    u8 frac;                      // 0x10
    u16 stride;                   // 0x12
    void *vertex;                 // 0x14
};

struct POBJDesc
{
    char *class_name;
    struct POBJDesc *next;
    struct _HSD_VtxDescList *verts;
    u16 flags;
    u16 n_display;
    u8 *display;
    union
    {
        JOBJDesc *joint;
        void *shape_set;
        void **envelope_p;
    } u;
};
struct POBJ
{
    int parent;                    // 0x0
    POBJ *next;                    // 0x4
    struct HSD_VtxDescList *verts; // 0x8
    u16 flags;                     // 0xC
    u16 n_display;                 // 0xE
    u8 *display;                   // 0x10, u8 primitive, u8 vtxcnt, u16* indices
    union                          // 0x14
    {
        JOBJ *jobj;
        void *shape_set;
        void *envelope_list;
    } u;
    AOBJ *aobj; // 0x18
};

struct DOBJ
{
    int parent;
    DOBJ *next; // 0x04
    MOBJ *mobj; // 0x08
    POBJ *pobj; // 0x0C
    AOBJ *aobj; // 0x10
    u32 flags;  // 0x14
    u32 unk;
};

struct JOBJ
{
    HSD_Obj object; // 0x0
    JOBJ *sibling;  // 0x08 (this is actually called next)
    JOBJ *parent;   // 0x0C (this is actually called prev)
    JOBJ *child;    // 0x10
    int flags;      // 0x14
    DOBJ *dobj;     // 0x18
    //  union {        // 0x18
    //     void* ptcl;
    //     DOBJ* dobj;
    //     HSD_Spline* spline;
    // } u;
    Vec4 rot;       // 0x1C 0x20 0x24 0x28
    Vec3 scale;     // 0x2C
    Vec3 trans;     // 0x38
    Mtx rotMtx;     // 0x44
    Vec3 *VEC;      // 0x74
    Mtx *MTX;       // 0x78
    AOBJ *aobj;     // 0x7C
    int *RObj;      // 0x80
    JOBJDesc *desc; // 0x84
};

struct WOBJ
{
    HSD_Obj parent;
    u32 flags;  // 0x08
    Vec3 pos;   // 0xC
    AOBJ *aobj; // 0x18
    void *robj; // 0x1C
};

struct COBJ
{
    HSD_Obj parent;      // 0x0
    u32 flags;           // 0x08
    f32 viewport_left;   // 0x0C
    f32 viewport_right;  // 0x10
    f32 viewport_top;    // 0x14
    f32 viewport_bottom; // 0x18
    u16 scissor_left;    // 0x1C
    u16 scissor_right;   // 0x1E
    u16 scissor_top;     // 0x20
    u16 scissor_bottom;  // 0x22
    WOBJ *eye;           // 0x24
    WOBJ *interest;      // 0x28
    union
    {
        f32 roll; // 0x28
        Vec3 up;  // 0x28 - 0x34
    } u;
    f32 near; // 0x38
    f32 far;  // 0x3C
    union
    {
        struct
        {
            f32 fov;
            f32 aspect;
        } perspective;

        struct
        {
            f32 top;
            f32 bottom;
            f32 left;
            f32 right;
        } frustrum;

        struct
        {
            f32 top;
            f32 bottom;
            f32 left;
            f32 right;
        } ortho;
    } projection_param;
    u8 projection_type; // 0x50
    Mtx view_mtx;       // 0x54
    AOBJ *aobj;         // 0x84
    Mtx *proj_mtx;      // 0x88
};

struct _HSD_ImageDesc
{
    void *img_ptr;
    u16 width;
    u16 height;
    u32 format;
    u32 mipmap;
    f32 minLOD;
    f32 maxLOD;
};

struct _HSD_Tlut
{
    void *tlut_ptr;
    u32 format;
    u32 gxtlut;
    u16 colorcount;
    u16 x0E;
};

struct _HSD_TObjTev
{
    u8 color_op;
    u8 alpha_op;
    u8 color_bias;
    u8 alpha_bias;

    u8 color_scale;
    u8 alpha_scale;
    u8 color_clamp;
    u8 alpha_clamp;

    u8 color_a;
    u8 color_b;
    u8 color_c;
    u8 color_d;

    u8 alpha_a;
    u8 alpha_b;
    u8 alpha_c;
    u8 alpha_d;

    GXColor constant;
    GXColor tev0;
    GXColor tev1;
    uint flags;
};

struct _HSD_LightPoint
{
    f32 cutoff;
    u8 point_func;
    f32 ref_br;
    f32 ref_dist;
    u8 dist_func;
};

struct _HSD_LightPointDesc
{
    f32 cutoff;
    u8 point_func;
    f32 ref_br;
    f32 ref_dist;
    u8 dist_func;
};

struct _HSD_LightSpot
{
    f32 cutoff;
    u8 spot_func;
    f32 ref_br;
    f32 ref_dist;
    u8 dist_func;
};

struct _HSD_LightSpotDesc
{
    f32 cutoff;
    u8 spot_func;
    f32 ref_br;
    f32 ref_dist;
    u8 dist_func;
};

struct _HSD_LightAttn
{
    f32 a0;
    f32 a1;
    f32 a2;
    f32 k0;
    f32 k1;
    f32 k2;
};

struct LObjDesc
{
    char *class_name;               // 0x00
    LObjDesc *next;                 // 0x04
    u16 flags;                      // 0x08
    u16 attnflags;                  // 0x0A
    GXColor color;                  // 0x0C
    struct _HSD_WObjDesc *position; // 0x10
    struct _HSD_WObjDesc *interest; // 0x14
    union
    {
        void *p;
        f32 *shininess;
        void *point;
        void *spot;
        void *attn;
    } u;
};
struct LightAnim
{
    LightAnim *next;
    struct _HSD_AObjDesc *aobjdesc;
    struct _HSD_WObjAnim *position_anim;
    struct _HSD_WObjAnim *interest_anim;
};
struct LightGroup
{
    LObjDesc *lobj_desc;
    LightAnim *anim;
};

struct LOBJ
{
    HSD_Obj parent;    // 0x00
    u16 flags;         // 0x08
    u16 priority;      // 0x0A
    struct LOBJ *next; // 0x0C
    GXColor color;     // 0x10
    GXColor hw_color;  // 0x14
    WOBJ *position;    // 0x18
    WOBJ *interest;    // 0x1C
    union
    {
        _HSD_LightPoint point;
        _HSD_LightSpot spot;
        _HSD_LightAttn attn;
    } u;
    f32 shininess;
    Vec3 lvec;
    AOBJ *aobj; // 0x48
    u32 id;     // 0x4c, GXLightID
    // GXLightObj lightobj;      //0x50
    u32 spec_id; // 0x90 GXLightID
    // GXLightObj spec_lightobj; //0x94
};

struct HSD_Fog
{
    HSD_Obj parent;
    u8 type;           // 0x08
    HSD_Fog *fog_adj;  // 0x0C
    f32 start;         // 0x10
    f32 end;           // 0x14
    GXColor color;     // 0x18
    struct AOBJ *aobj; // 0x1C
};

struct HSD_FogDesc
{
    u8 type;                    // 0x00
    HSD_FogDesc *fog_adj;       // 0x04
    f32 start;                  // 0x08
    f32 end;                    // 0x0C
    GXColor color;              // 0x10
    struct _HSD_AObjDesc *aobj; // 0x14
};

struct JOBJSet
{
    JOBJDesc *jobj;
    void **animjoint;
    void **matanimjoint;
    void **shapeaninjoint;
};

struct HSD_SObjDesc
{
    JOBJSet **jobjset;
    COBJDesc **cobjdesc;
    LightGroup *lights;
    HSD_FogDesc **fog;
};

/*** Static Variables ***/
// static GOBJ ***stc_gobj_gx_lookup = R13_OFFSET(-0x3E7C);
static GOBJ ***stc_gobj_lookup = R13_OFFSET(-0x3E74);    //
static u8 *stc_gobj_proc_num = (void *)0x804ce382;           // number of elements in the below array
static GOBJProc ***stc_gobjproc_lookup = (void *)0x804D7840; // array of gobj procs ptrs
static GOBJProc **stc_gobjproc_cur = (void *)0x804d7838;     // current gobj proc being processed
static u32 *stc_gobjproc_updateidx_cur = (void *)0x804d783c; // update index of the current gobj proc being processed. this is compared to
static u8 *objkind_sobj = R13_OFFSET(-0x3D40);
static u8 *objkind_cobj = R13_OFFSET(-0x3E55);
static u8 *objkind_lobj = R13_OFFSET(-0x3E56);
static u8 *objkind_jobj = R13_OFFSET(-0x3E57);
static u8 *objkind_fog = R13_OFFSET(-0x3E58);

/*** Functions ***/
int JOBJ_GetWorldPosition(JOBJ *source, Vec3 *add, Vec3 *dest);
void JOBJ_SetMtxDirtySub(JOBJ *jobj);
void JOBJ_SetupMtxSub(JOBJ *jobj);
void JOBJ_MakeMatrix(JOBJ *jobj);
JOBJ *JOBJ_LoadDummy();
JOBJ *JOBJ_LoadJoint(JOBJDesc *joint);
void JOBJ_RemoveAll(JOBJ *joint);
void JOBJ_Remove(JOBJ *joint);
void JOBJ_GetChild(JOBJ *joint, JOBJ **ptr, int index, ...);
void JOBJ_AddChild(JOBJ *parent, JOBJ *child);
void JOBJ_AddNext(JOBJ *parent, JOBJ *child);
float JOBJ_GetCurrentMatAnimFrame(JOBJ *joint);
void JOBJ_SetFlags(JOBJ *joint, int flags);
void JOBJ_SetFlagsAll(JOBJ *joint, int flags);
void JOBJ_ClearFlags(JOBJ *joint, int flags);
void JOBJ_ClearFlagsAll(JOBJ *joint, int flags);
void JOBJ_BillBoard(JOBJ *joint, Mtx *m, Mtx *mx);
void JOBJ_ForEachAnim(JOBJ *joint, int unk, ForEachAnimFlag flags, void *cb, int argkind, ...); // argkind specifies how to pop args off the va_list
void JOBJ_Anim(JOBJ *joint);
void JOBJ_AnimAll(JOBJ *joint);
void JOBJ_AddAnimAll(JOBJ *joint, void *animjoint, void *matanimjoint, void *shapeanimjoint);
void JOBJ_RemoveAnimAll(JOBJ *joint);
void JOBJ_ReqAnim(JOBJ *joint, float frame);
void JOBJ_ReqAnimByFlags(JOBJ *joint, int flags, float frame);
void JOBJ_ReqAnimAll(JOBJ *joint, float unk);
void JOBJ_ReqAnimAllByFlags(JOBJ *joint, int flags, float frame);
float JOBJ_GetJointAnimCurrFrame(JOBJ *joint);
float JOBJ_GetJointAnimFrameTotal(JOBJ *joint);
float JOBJ_GetJointAnimNextFrame(JOBJ *joint);
void JOBJ_SetAllMOBJFlags(JOBJ *joint, int flags);
void JOBJ_SetFlagAllMOBJ(JOBJ *joint, int flags); // enables this flag for all mobjs
int JOBJ_CheckAObjEnd(JOBJ *joint);
void JOBJ_CompileTEVAllMOBJ(JOBJ *joint);
void JObj_DispAll(JOBJ *joint, Mtx *vmtx, int rendermode, int mobj_flags);
void JOBJ_AttachPosition(JOBJ *to_attach, JOBJ *attach_to);
void JOBJ_AttachPositionRotation(JOBJ *to_attach, JOBJ *attach_to);
GOBJ *JOBJ_LoadSet(int is_hidden, JOBJSet *set, int anim_id, float frame, int p_link, int gx_link, int is_add_anim, void *cb); // 8019035c
void JOBJ_AddSetAnim(JOBJ *jobj, JOBJSet *set, int anim_id);                                                                   // 8016895c
void JOBJ_Detach(JOBJ *to_attach);
void JOBJ_ResetFromDesc(JOBJ *, JOBJDesc *);
void JOBJ_RemoveAnimByFlags(JOBJ *, int);
void AOBJ_ReqAnim(int *aobj, float unk);
void AOBJ_StopAnim(AOBJ *aobj);
void AOBJ_SetRate(AOBJ *aobj, float rate);
void AOBJ_SetFlags(AOBJ *aobj, int flags);
void AOBJ_ClearFlags(AOBJ *aobj, int flags);
void DOBJ_SetFlags(DOBJ *dobj, int flags);
void DOBJ_ClearFlags(DOBJ *dobj, int flags);
void DOBJ_AddAnimAll(DOBJ *dobj, void *matanim, void *textureanim);
void TOBJ_AddAnim(TOBJ *tobj, void *textureanim);
COBJ *COBJ_Alloc();
COBJ *COBJ_LoadDesc(COBJDesc *cobj);
COBJ *COBJ_LoadDescSetScissor(COBJDesc *cobj);
void COBJ_Init(COBJ *cobj, COBJDesc *cobj_desc); // re-initializes a live cobj using its descriptor
void CObjThink_Common(GOBJ *gobj);
int CObj_SetCurrent(COBJ *cobj);
void CObj_SetEraseColor(int r, int g, int b, int a);
void CObj_EraseScreen(COBJ *cobj, GXBool color_update_enable, GXBool alpha_update_enable, GXBool depth_update_enable);
void CObj_UpdateAll();
void CObj_RenderGXLinks(GOBJ *gobj, int render_mode);
void CObj_EndCurrent();
void CObj_SetOrtho(COBJ *cobj, float top, float bottom, float left, float right);
void CObj_SetViewport(COBJ *cobj, float left, float right, float top, float bottom);
void CObj_SetScissor(COBJ *cobj, u16 top, u16 bottom, u16 left, u16 right);
void CObj_SetEyePosition(COBJ *cobj, Vec3 *eye_pos);
void COBJ_GetEyePosition(COBJ *cobj, Vec3 *eye_pos);
void CObj_SetInterest(COBJ *cobj, Vec3 *pos);
void CObj_SetRoll(COBJ *cobj, float roll);
void CObj_Release(COBJ *cobj);
void CObj_Destroy(COBJ *cobj);
COBJ *COBJ_GetCurrent(void);
void COBJ_GetEyeVector(COBJ *cobj, Vec3 *eye_vec);
void COBJ_GetInterest(COBJ *cobj, Vec3 *interest);
float COBJ_GetEyeDistance(COBJ *cobj);
void COBJ_GetViewingMtx(COBJ *cobj, Mtx *out);
Mtx *COBJ_SetupViewingMtx(COBJ *cobj);
GOBJ *GObj_Create(int entity_class, int p_link, int p_priority);
void GObj_Destroy(GOBJ *gobj);
void GObj_AddGXLink(GOBJ *gobj, void *cb, int gx_link, int gx_pri);
void GObj_DestroyGXLink(GOBJ *gobj);
void GObj_GXReorder(GOBJ *gobj, int unk);
GOBJProc *GObj_AddProc(GOBJ *gobj, void *callback, int priority);
void GObj_RemoveProc(GOBJ *gobj);
void GObj_AddObject(GOBJ *gobj, u8 obj_kind, void *object);
void GObj_FreeObject(GOBJ *gobj);
void GObj_AddUserData(GOBJ *gobj, int userDataKind, void *destructor, void *userData);
void GOBJ_InitCamera(GOBJ *gobj, void *cb, int gx_pri);
void GObj_Anim(GOBJ *gobj);
void *GObj_AddRenderObject(GOBJ *gobj, int width, int height);
void GObj_ProcUnk(GOBJ *gobj);
void GObj_DestroyByPLink(int p_link);                           // destroys all gobjs with p_link X
void GObj_DestroyByPLinkRange(int p_link_low, int p_link_high); // destroys all gobjs of p_link_low -> p_link_high
void GObj_UpdateAll();
void GXLink_Common(GOBJ *gobj, int pass);
int GX_LookupRenderPass(int pass);
void GXLink_LObj(GOBJ *gobj, int pass);
void GXLink_Fog(GOBJ *gobj, int pass);
LOBJ *LObj_LoadDesc(void *lobjdesc);
LOBJ *LObj_CreateAll(void **lobjdesc);
int LObj_GetPosition(LOBJ *lobj, Vec3 *pos);
void LObj_SetPosition(LOBJ *lobj, Vec3 *pos);
int LObj_GetInterest(LOBJ *lobj, Vec3 *pos);
void LObj_SetInterest(LOBJ *lobj, Vec3 *pos);
void LObj_ReqAnimAll(LOBJ *lobj, float frame);
void LObj_AnimAll(LOBJ *lobj);
void LObj_DeleteCurrentAll(int unk);
void LObj_RemoveAll(LOBJ *lobj);
HSD_Fog *Fog_LoadDesc(HSD_FogDesc *fogdesc);
void Fog_Set(HSD_Fog *fog);
void Fog_Release(HSD_Fog *fog);
DOBJ *JOBJ_GetDObj(JOBJ *jobj);
void *MOBJ_SetAlpha(DOBJ *dobj, float alpha);
void MOBJ_SetToonTextureImage(_HSD_ImageDesc *);
void MOBJ_ReqAnim(MOBJ *, float frame);
void MObj_Anim(MOBJ *);
void GObj_CopyGXPri(GOBJ *target, GOBJ *source);
#endif
