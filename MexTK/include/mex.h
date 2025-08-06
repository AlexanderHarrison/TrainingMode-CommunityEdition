#ifndef MEX_H_MEX
#define MEX_H_MEX

#include "structs.h"
#include "datatypes.h"
#include "obj.h"

#define DB_FLAG 0

enum MEX_GETDATA
{
    MXDT_FTINTNUM,
    MXDT_FTEXTNUM,
    MXDT_FTICONNUM,
    MXDT_FTICONDATA,
    MXDT_GRINTNUM,
    MXDT_GREXTNUM,
    MXDT_GRICONNUM,
    MXDT_GRICONDATA,
    MXDT_FTCOSTUMEARCHIVE,
    MXDT_GRDESC,         // gets GrDesc struct
    MXDT_GREXTLOOKUP,    // gets first stage external ID from internal ID
    MXDT_GRNAME,         // returns stage name char*
    MXDT_FTNAME,         // returns fighter name char*, indexed by external ID
    MXDT_FTDAT,          // returns pointer to fighter file struct, indexed by internal ID
    MXDT_FTKINDDESC,     // returns pointer to ftkind desc array, indexed by external ID
    MXDT_FTEMBLEMLOOKUP, // returns pointer to ftkind desc array, indexed by external ID
    MXDT_MEXDATA,        //
    MXDT_PRELOADHEAP,    // returns pointer to PreloadHeap
    MXPT_FILE,           // returns pointer to Archive data for MxPt if it exists
};

typedef enum SSMKind
{
    SSMKIND_CHAR,
    SSMKIND_STAGE,
} SSMKind;

/*** Structs ***/

struct Stc_icns
{
    u16 reserved_num;
    u16 stride;
    MatAnimJointDesc *matanimjoint;
    int custom_stock_num;
    void *custom_stock_descs;
};

struct MEXPlaylistEntry
{
    u16 bgm;
    u16 chance;
};

struct MEXPlaylist
{
    int entry_count;
    MEXPlaylistEntry *entries;
};

struct MexCostumeDesc // this is the symbol in the file
{

    void *costume_vis_lookup; // 0x0
    void *costume_mat_lookup; // 0x4
    int accessory_num;        // 0x8
    struct MexCostumeDescAccessory
    {
        JOBJDesc *joint_desc;
        int bone_index;
        int dynamic_num;
        DynamicsDesc **dynamic_desc;
        int ftparts_num;
        FtPartsVis *ftparts;
        int dynamic_hit_num;
        DynamicsHitDesc **dynamic_hit_desc;
        AnimJointDesc *animjointdesc;
        MatAnimJointDesc *matanimjointdesc;
        FtScript *script;
    } **accessories;
};

typedef struct MexCostumeRuntime
{
    JOBJDesc *joint_desc;
    MatAnimJointDesc *matanim_desc;
    int x08;
    int x0C;
    int x10;
    HSD_Archive *archive;
} MexCostumeRuntime;

typedef struct MexCostumeRuntimeDesc
{
    MexCostumeRuntime *runtimes;
    int count;
} MexCostumeRuntimeDesc;

typedef struct MEXInstructionTable
{
    u32 cmd : 8;          // 0x0
    u32 code_offset : 24; // 0x01
    uint reloc_offset;    // 0x4
} MEXInstructionTable;

typedef struct MEXFunctionTable
{
    int index;        // 0x0
    uint code_offset; // 0x4
} MEXFunctionTable;

typedef struct MEXDebugSymbol
{
    uint code_offset; // 0x0
    uint code_length; // 0x4
    char *symbol;     // 0x8
} MEXDebugSymbol;

typedef struct MEXFunctionLookup
{
    int num;
    void *function[30];
} MEXFunctionLookup;

typedef struct MEXFunction
{
    u8 *code;                                     // 0x0
    MEXInstructionTable *instruction_reloc_table; // 0x4
    int instruction_reloc_table_num;              // 0x8
    MEXFunctionTable *func_reloc_table;           // 0xC
    int func_reloc_table_num;                     // 0x10
    int code_size;                                // 0x14
    int debug_symbol_num;                         // 0x18
    MEXDebugSymbol *debug_symbol;                 // 0x1c
} MEXFunction;

typedef struct MexMetaData
{
    u8 v_major;
    u8 v_minor;
    short flags;
    int internal_id_count;
    int external_id_count;
    int css_icon_count;
    int internal_stage_count;
    int external_stage_count;
    int sss_icon_count;
    int ssm_count;
    int bgm_count;
    int effect_count;
    int bootup_scene;
    int last_major;
    int last_minor;
    int trophy_count;
    int trophy_sd_offset;
} MexMetaData;

typedef struct MnSlMapIcon
{
    JOBJ *runtime_joint;
    int flags;
    byte icon_state;
    byte preview_id;
    byte random_select_stage_id;

    Vec2 cursor_size;
    Vec2 outline_size;
    int external_id;
} MnSlMapIcon;

typedef struct MexMenuData
{
    int *params;
    MnSlChrIcon *css_icons;
    MnSlMapIcon *sss_icons;
    void *sss_bitfields;
} MexMenuData;

typedef struct MexMusicTable
{
    char **filenames;
    MEXPlaylistEntry *menu_playlist;
    int menu_playlist_count;
    char **labels;

} MexMusicTable;

typedef struct MexStageSound
{
    char ssmid;
    char reverb;
    char x03;
} MexStageSound;

typedef struct MexStageTable
{
    GrExtLookup *grkind_ext_desc; // indexed by GrKindExt (external ID)
    MexStageSound *sound_table;
    void *collision_table;
    void *item_table;
    char **name_table;
    MEXPlaylist *playlist_table;
} MexStageTable;

typedef struct MexSceneData
{
    MajorSceneDesc *major_desc_arr;
    MinorSceneDesc *minor_desc_arr;
} MexSceneData;

typedef struct MexData
{
    MexMetaData *metadata;
    MexMenuData *menu;
    struct
    {                                      //
        char **names;                      // 0x0,
        struct                             //
        {                                  //
            char *name;                    //
            char *symbol;                  //
        } *pl_file;                        // 0x4
        u8 *insignia_idx;                  // 0x8
        FtKindDesc *ft_kind_desc;          // 0xC, array of these
        struct                             //
        {                                  //
            u8 num;                        //
            u8 red_idx;                    //
            u8 blue_idx;                   //
            u8 green_idx;                  //
        } *costume_info;                   // 0x10, array of these
        struct                             //
        {                                  //
            struct                         //
            {                              //
                char *file_name;           //
                char *joint_symbol;        //
                char *matanim_symbol;      //
                int visibility_lookup_idx; //
            } *costume_symbol_table;       // array of these per costume
        } *costume_file;                   // 0x14, array of these per character
        // theres more im just lazy
        void *ftdemo_filenames;
        void *anim_filenames;
        void *anim_num;
        u8 *effect_index;
    } *fighter; // indexed by ft_kind
    void *fighter_function;
    void *ssm;
    MexMusicTable *music;
    struct
    {
        struct
        {
            char *dat_name;
            char *symbol_name;
            void *particle_data_runtime; // this pointer is shifed back by (effect_idx_start * 4)!
        } *files;
    } *effect;
    void *item;
    void *kirby;
    void *kirby_function;
    MexStageTable *stage;
    GrDesc **stage_desc; // indexed by GrKind (internal ID)
    void *scene;
    void *misc;
} MexData;

static MEXFunctionLookup **stc_mexfunction_lookup = (void *)0x804dfad8;

/*** Functions ***/
HSD_Archive *MEX_LoadRelArchive(char *file, void *functions, char *symbol);
void MEX_RelocRelArchive(void *xFunction);
void *calloc(int size);
void bp();

// see https://smashboards.com/threads/primitive-drawing-module.454232/

enum PRIM_Shape {
    PRIM_SHAPE_QUADS,
    PRIM_SHAPE_TRIANGLES = 2,
    PRIM_SHAPE_TRIANGLE_STRIP,
    PRIM_SHAPE_TRIANGLE_FAN,
    PRIM_SHAPE_LINES,
    PRIM_SHAPE_LINE_STRIP,
    PRIM_SHAPE_POINTS,
};
enum PRIM_Blend_Type {
    PRIM_BLEND_NONE,
    PRIM_BLEND_BLEND,
    PRIM_BLEND_LOGIC,
    PRIM_BLEND_SUBTRACT,
};
enum PRIM_Logic {
    PRIM_LOGIC_CLEAR,
    PRIM_LOGIC_AND,
    PRIM_LOGIC_REVAND,
    PRIM_LOGIC_COPY,
    PRIM_LOGIC_INVAND,
    PRIM_LOGIC_NOOP,
    PRIM_LOGIC_XOR,
    PRIM_LOGIC_OR,
    PRIM_LOGIC_EQUIV,
    PRIM_LOGIC_INV,
    PRIM_LOGIC_REVOR,
    PRIM_LOGIC_INVCOPY,
    PRIM_LOGIC_INVOR,
    PRIM_LOGIC_NAND,
    PRIM_LOGIC_SET,
};
enum PRIM_Source {
    PRIM_SOURCE_0,
    PRIM_SOURCE_1,
    PRIM_SOURCE_SRC_COLOR,
    PRIM_SOURCE_SRC_INV_COLOR,
    PRIM_SOURCE_SRC_ALPHA,
    PRIM_SOURCE_SRC_INV_ALPHA,
    PRIM_SOURCE_DST_ALPHA,
    PRIM_SOURCE_DST_INV_ALPHA,
};

typedef struct PRIM_DrawMode {
    u32 cull_back : 1;
    u32 cull_front : 1;
    u32 unk1 : 4;
    u32 line_width : 10;
    u32 unk2 : 2;
    u32 z_compare_timing : 1;
    u32 z_compare_enable : 1;
    u32 z_update_enable : 1;
    u32 z_logic_gt : 1;
    u32 z_logic_eq : 1;
    u32 z_logic_lt : 1;
    u32 shape : 8;
} PRIM_DrawMode;
typedef struct PRIM_BlendMode {
    u32 unk1 : 18;
    u32 blend_type : 2;
    u32 unk2 : 1;
    u32 blend_src : 3;
    u32 unk3 : 1;
    u32 blend_dst : 3;
    u32 blend_logic : 4;
} PRIM_BlendMode;

void PRIM_NEW_INTERNAL(int vert_count, u32 draw_mode, u32 blend_mode);
static inline void PRIM_NEW(int vert_count, PRIM_DrawMode draw_mode, PRIM_BlendMode blend_mode)
{
    union {
        PRIM_DrawMode bitflags;
        u32 integer;
    } dm;
    union {
        PRIM_BlendMode bitflags;
        u32 integer;
    } bm;
    
    dm.bitflags = draw_mode;
    bm.bitflags = blend_mode;
    
    PRIM_NEW_INTERNAL(vert_count, dm.integer, bm.integer);
}
void PRIM_CLOSE();

static void MEX_InitRELDAT(HSD_Archive *archive, char *symbol_name, void **return_func_array)
{
    MEXFunction *mex_function = Archive_GetPublicAddress(archive, symbol_name);

    // reloc instructions
    MEX_RelocRelArchive(mex_function);

    // reloc function pointers
    for (int i = 0; i < mex_function->func_reloc_table_num; i++)
    {
        MEXFunctionTable *this_func = &mex_function->func_reloc_table[i];
        return_func_array[i] = (void *)&mex_function->code[this_func->code_offset];
    }
}

#endif
