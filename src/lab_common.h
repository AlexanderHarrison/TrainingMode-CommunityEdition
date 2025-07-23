#include "../MexTK/mex.h"
#include "events.h"

// unfortunately there are static in this header that only used in labCSS or lab
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"

// Labbing event
// Custom TDI definitions
#define TDI_HITNUM 10
#define TDI_DISPNUM 4
// menu model
#define TDIMENU_SCALE 1
#define TDIMENU_X 0
#define TDIMENU_Y 0
#define TDIMENU_Z 0
#define TDIMENU_WIDTH 55 / TDIMENU_SCALE
#define TDIMENU_HEIGHT 40 / TDIMENU_SCALE
// menu text object
#define TDITEXT_CANVASSCALE 0.05
#define TDITEXT_TEXTSCALE 1
#define TDITEXT_TEXTZ 0

// recording
#define REC_VERS 1

//Recording Version History:
//v0 = 3.0 Alpha 1-3.
//v1 = 3.0 Alpha 4. 
//  First version with importing UI.
//  Added filename and match settings to metadata, menu settings and adjusted the playerblock and grab struct for the fighters.
//v2 = not released

#define GXPRI_RECJOINT 80
#define REC_GXLINK 18
#define GXPRI_RECTEXT GXPRI_RECJOINT + 1
// cobj
#define RECCAM_COBJGXLINK (1 << REC_GXLINK)
#define RECCAM_GXPRI 8
// params
#define REC_LENGTH (1 * 60 * 60)
#define REC_SLOTS 6
#define REC_SEEKJOINT 7
#define REC_LEFTBOUNDJOINT 5
#define REC_RIGHTBOUNDJOINT 6
#define REC_LEFTTEXTJOINT 2
#define REC_RIGHTTEXTJOINT 3
#define REC_TEXTSCALE 0.04

// export
enum export_status
{
    EXSTAT_NONE,
    EXSTAT_REQSAVE,
    EXSTAT_SAVEWAIT,
    EXSTAT_DONE,
};
enum export_menuindex
{
    EXMENU_SELCARD,
    EXMENU_NAME,
    EXMENU_CONFIRM,
};
enum export_popup
{
    EXPOP_CONFIRM,
    EXPOP_SAVE,
};
#define EXP_MEMCARDAJOBJ 9
#define EXP_MEMCARDBJOBJ 11
#define EXP_TEXTBOXJOBJ 12
#define EXP_SCREENSHOTJOBJ 18

#define EXP_KEYBOARD_X 0
#define EXP_KEYBOARD_Y 3.5
#define EXP_KEYBOARD_SIZE 1

#define EXP_FILEDETAILS_X -8
#define EXP_FILEDETAILS_Y -11.7
#define EXP_FILEDETAILS_SIZE 0.7

#define EXP_FILENAME_X -7
#define EXP_FILENAME_Y -4.8
#define EXP_FILENAME_ASPECTX 560
#define EXP_FILENAME_ASPECTY 100
#define EXP_FILENAME_SIZE 1

#define EXP_SCREENSHOT_WIDTH (640)
#define EXP_SCREENSHOT_HEIGHT (480)

#define RESIZE_MULT (0.15)
#define RESIZE_WIDTH (EXP_SCREENSHOT_WIDTH * RESIZE_MULT)
#define RESIZE_HEIGHT (EXP_SCREENSHOT_HEIGHT * RESIZE_MULT)

// input display
typedef struct ButtonLookup
{
    u8 jobj;
    u8 dobj;
} ButtonLookup;
enum buttons_enum
{
    BTN_A,
    BTN_B,
    BTN_X,
    BTN_Y,
    BTN_START,
    BTN_DPADUP,
    BTN_DPADRIGHT,
    BTN_DPADLEFT,
    BTN_DPADDOWN,
    BTN_L,
    BTN_R,
    BTN_Z,
    BTN_NUM,
};
static ButtonLookup button_lookup[] = {
    {1, 0},  // A
    {2, 0},  // B
    {3, 0},  // X
    {4, 0},  // Y
    {5, 0},  // Start
    {6, 1},  // Dpad Up
    {6, 2},  // Dpad Right
    {6, 3},  // Dpad Left
    {6, 4},  // Dpad Down
    {11, 0}, // L
    {12, 0}, // R
    {13, 0}, // Z
};
static GXColor button_colors[] = {
    {50, 180, 50, 255},   // A
    {255, 50, 50, 255},   // B
    {127, 127, 127, 255}, // X
    {127, 127, 127, 255}, // Y
    {192, 192, 192, 255}, // Start
    {192, 192, 192, 255}, // Dpad Up
    {192, 192, 192, 255}, // Dpad Right
    {192, 192, 192, 255}, // Dpad Left
    {192, 192, 192, 255}, // Dpad Down
    {127, 127, 127, 255}, // L
    {127, 127, 127, 255}, // R
    {0, 0, 255, 255},     // Z
};
static int button_bits[] = {
    HSD_BUTTON_A,          // A
    HSD_BUTTON_B,          // B
    HSD_BUTTON_X,          // X
    HSD_BUTTON_Y,          // Y
    HSD_BUTTON_START,      // Start
    HSD_BUTTON_DPAD_UP,    // Dpad Up
    HSD_BUTTON_DPAD_RIGHT, // Dpad Right
    HSD_BUTTON_DPAD_LEFT,  // Dpad Left
    HSD_BUTTON_DPAD_DOWN,  // Dpad Down
    HSD_TRIGGER_L,         // L
    HSD_TRIGGER_R,         // R
    HSD_TRIGGER_Z,         // Z
 };// GX
#define INPUT_GXLINK 12
#define INPUT_GXPRI 80
// params
#define INPUT_SHELL_JOBJ 14
#define INPUT_SHELL_DOBJ 0
#define INPUT_COLOR_PRESSED \
    {                       \
        255, 255, 255, 255  \
    }

typedef struct Arch_ImportData
{
    JOBJDesc *import_button;
    JOBJDesc *import_menu;
    COBJDesc *import_cam;
    JOBJDesc *import_popup;
} Arch_ImportData;
typedef struct Arch_LabData
{
    JOBJDesc *stick;
    JOBJDesc *cstick;
    void *save_icon;
    void *save_banner;
    JOBJDesc *controller;
    JOBJDesc *export_menu;
    JOBJDesc *export_popup;
} Arch_LabData;
typedef struct LabData
{
    EventDesc *eventInfo;
    u8 cpu_state;
    u8 cpu_hitshield;
    u8 cpu_hitnum;
    u8 cpu_sdinum;
    u8 cpu_sdidir;

    // Delay timer for "Counter After Frames" Option
    u8 cpu_countertimer;

    // Delay timer for "Miss Tech Wait Chance" Option
    u8 cpu_miss_tech_wait_timer;

    // Aitch: We set this flag if the CPU performs a counter action on this frame.
    // Occasionally we need to determine if the CPU is countering (such as AUTORESTORE_COUNTER).
    // However, if the counter action immediately completes, the cpu state is never set to CPUSTATE_COUNTER.
    // And sometimes the CPU is in the CPUSTATE_COUNTER state but does not act.
    // This flag is only set if the CPU has counter acted this frame.
    u8 cpu_countering: 1;

    // Aitch: This flag is set per action and prevents air-ground transitions from interrupting the counter action.
    u8 cpu_countering_no_interrupt: 1;

    // Aitch: This counter needs to be manually set. We can't use the baked in ssbm timer, because
    // It'll mess with internal ssbm stuff probably. If the timer is not zero, then no tech.stuff
    // This is set when hit in tumble.
    int cpu_tech_lockout;

    // We need a place to store the current playback frame when the cpu is set to use a slot as a counter action.
    u16 counter_slot_frame;

    s16 cpu_lasthit;
    s16 cpu_lastshieldstun; // last move instance of the opponent in shield stun. used to tell how many times the shield was hit
    s8 cpu_hitkind;         // how the CPU was hit, damage or shield
    u8 cpu_hitshieldnum;    // times the CPUs shield was hit
    u8 cpu_isactionable;    // flag that indicates if a cpu has become actionable
    u8 cpu_groundstate;     // indicates if the player was touching ground upon being actionable
    s32 timer;
    u8 cpu_isthrown; // bool for if the cpu is being thrown
    GOBJ *rec_gobj;
    u8 hmn_controller;
    u8 cpu_controller;
} LabData;
typedef struct LabPersistentData {
    u8 peach_item_rng;      // 0x0
    u8 peach_fsmash_rng;    // 0x1
    u8 luigi_misfire_rng;   // 0x2
    u8 gnw_hammer_rng;      // 0x3
    u8 nana_throw_rng;      // 0x4
} LabPersistentData;
typedef struct InfoDisplayData
{
    JOBJ *menuModel;
    JOBJ *botLeftEdge;
    JOBJ *botRightEdge;
    Text *text;
} InfoDisplayData;
typedef struct DIDraw
{
    int num[2];        // number of vertices
    Vec2 *vertices[2]; // pointer to vertex to draw
    GXColor color;     // color of this vertex
} DIDraw;
typedef struct DIDrawCalculate
{
    Vec2 pos;       // position of vertices
    int envFlags;   // environment flags
    float ECBTopY;  // used for determining middle of body
    float ECBLeftY; // used for determining middle of body
    float kb_Y;     // used to determine ceiling KOs
} DIDrawCalculate;
typedef struct TDIData
{
    JOBJ *stick_curr[2];
    JOBJ *stick_prev[6][2];
    Text *text_curr;
} TDIData;
typedef struct RecInputs
{
    unsigned char btn_dpadup : 1;
    unsigned char btn_a : 1;
    unsigned char btn_b : 1;
    unsigned char btn_x : 1;
    unsigned char btn_y : 1;
    unsigned char btn_L : 1;
    unsigned char btn_R : 1;
    unsigned char btn_Z : 1;
    s8 stickX;
    s8 stickY;
    s8 substickX;
    s8 substickY;
    u8 trigger;
} RecInputs;
typedef struct RecInputData
{
    int start_frame; // the frame these inputs start on
    int num;
    RecInputs inputs[REC_LENGTH];
} RecInputData;
typedef struct RecData
{
    int hmn_rndm_slot;
    RecInputData *hmn_inputs[REC_SLOTS];
    RecInputData *hmn_rerecord_inputs;
    int cpu_rndm_slot;
    RecInputData *cpu_inputs[REC_SLOTS];
    RecInputData *cpu_rerecord_inputs;
    int restore_timer;
    JOBJ *seek_jobj;
    Text *text;
    float seek_left;
    float seek_right;
} RecData;

typedef struct RecordingSave
{
    MatchInit match_data; // this will point to a struct containing match info
    Savestate_v1 savestate;
    RecInputData hmn_inputs[REC_SLOTS];
    RecInputData cpu_inputs[REC_SLOTS];
} RecordingSave;

typedef struct Controller
{
    JOBJ *jobj;
    Vec2 ltrig_origin;
    Vec2 rtrig_origin;
} Controller;

typedef struct ExportData
{
    u16 menu_index;
    u16 menu_state;
    JOBJ *memcard_jobj[2];
    JOBJ *screenshot_jobj;
    JOBJ *textbox_jobj;
    RGB565 *scaled_image;
    Text *text_title;
    Text *text_desc;
    Text *text_misc;
    int slot;
    u8 is_inserted[2];
    int free_blocks[2];
    int free_files[2];
    Text *text_keyboard;
    Text *text_filename;
    Text *text_filedetails;
    u8 key_cursor[2];
    u8 filename_cursor;
    u8 caps_lock;
    char *filename_buffer;
    GOBJ *confirm_gobj;
    Text *confirm_text;
    u8 confirm_cursor;
    u8 confirm_state;
    int hmn_id;
    int cpu_id;
    int stage_id;
} ExportData;
typedef struct ExportMenuSettings
{
    u8 hmn_mode;
    u8 hmn_slot;
    u8 cpu_mode;
    u8 cpu_slot;
    u8 loop_inputs;
    u8 auto_restore;
} ExportMenuSettings;
typedef struct ExportHeader
{
    struct rec_metadata // metadata
    {
        u16 version;
        u16 image_width;
        u16 image_height;
        u16 image_fmt;
        u8 hmn;
        u8 hmn_costume;
        u8 cpu;
        u8 cpu_costume;
        u16 stage_external; // instance of the stage
        u16 stage_internal; //  unique stage
        u8 month;
        u8 day;
        u16 year;
        u8 hour;
        u8 minute;
        u8 second;
        char filename[32];
    } metadata;
    struct // lookup
    {
        int ofst_screenshot;
        int ofst_recording;
        int ofst_menusettings;
        // to-do, add menu data offset
    } lookup;
} ExportHeader;

// TODO move these to lab.h

void Event_Init(GOBJ *gobj);
void Event_Update();
void Event_Think(GOBJ *event);

void Button_Think(GOBJ *button_gobj);

void Lab_CustomOSDsThink(void);
void Lab_RemoveCustomOSD(GOBJ *menu_gobj);
void Lab_AddCustomOSD(GOBJ *menu_gobj);
void Lab_ChangeOSDs(GOBJ *menu_gobj, int value);
void Lab_ChangeStadiumTransformation(GOBJ *menu_gobj, int value);
void Lab_ChangeInputDisplay(GOBJ *menu_gobj, int value);
void Lab_ChangeOverlays(GOBJ *menu_gobj, int value);
void Lab_StartMoveCPU(GOBJ *menu_gobj);
void Lab_FinishMoveCPU(GOBJ *menu_gobj);
void Lab_FreezeCPU(GOBJ *menu_gobj);
void Lab_ChangeTauntEnabled(GOBJ *menu_gobj, int value);
void Lab_ChangePlayerPercent(GOBJ *menu_gobj, int value);
void Lab_ChangePlayerLockPercent(GOBJ *menu_gobj, int value);
void Lab_ChangeCharacterRng_Turnip(GOBJ *menu_gobj, int value);
void Lab_ChangeCharacterRng_PeachFSmash(GOBJ *menu_gobj, int value);
void Lab_ChangeCharacterRng_Hammer(GOBJ *menu_gobj, int value);
void Lab_ChangeCharacterRng_Misfire(GOBJ *menu_gobj, int value);
void Lab_ChangeCharacterRng_NanaThrow(GOBJ *menu_gobj, int value);
void Lab_ChangeFrameAdvance(GOBJ *menu_gobj, int value);
void Lab_ChangeFrameAdvanceButton(GOBJ *menu_gobj, int value);
void Lab_ChangeCPUPercent(GOBJ *menu_gobj, int value);
void Lab_ChangeCPULockPercent(GOBJ *menu_gobj, int value);
void Lab_ChangeTech(GOBJ *menu_gobj, int value);
void Lab_ChangeGetup(GOBJ *menu_gobj, int value);
void Lab_ChangeTechInPlaceChance(GOBJ *menu_gobj, int value);
void Lab_ChangeTechAwayChance(GOBJ *menu_gobj, int value);
void Lab_ChangeTechTowardChance(GOBJ *menu_gobj, int value);
void Lab_ChangeMissTechChance(GOBJ *menu_gobj, int value);
void Lab_ChangeStandChance(GOBJ *menu_gobj, int value);
void Lab_ChangeRollAwayChance(GOBJ *menu_gobj, int value);
void Lab_ChangeRollTowardChance(GOBJ *menu_gobj, int value);
void Lab_ChangeGetupAttackChance(GOBJ *menu_gobj, int value);
void Lab_ChangeCPUIntang(GOBJ *menu_gobj, int value);
void Lab_ChangeModelDisplay(GOBJ *menu_gobj, int value);
void Lab_ChangeHitDisplay(GOBJ *menu_gobj, int value);
void Lab_ChangeEnvCollDisplay(GOBJ *menu_gobj, int value);
void Lab_ChangeItemGrabDisplay(GOBJ *menu_gobj, int value);
void Lab_ChangeCamMode(GOBJ *menu_gobj, int value);
void Lab_ChangeInfoPresetHMN(GOBJ *menu_gobj, int preset_id);
void Lab_ChangeInfoPresetCPU(GOBJ *menu_gobj, int preset_id);
void Lab_ChangeHUD(GOBJ *menu_gobj, int value);
void Lab_SelectCustomTDI(GOBJ *menu_gobj);
void Lab_ChangeSlot1ChanceHMN (GOBJ *menu_gobj, int _new_val);
void Lab_ChangeSlot2ChanceHMN (GOBJ *menu_gobj, int _new_val);
void Lab_ChangeSlot3ChanceHMN (GOBJ *menu_gobj, int _new_val);
void Lab_ChangeSlot4ChanceHMN (GOBJ *menu_gobj, int _new_val);
void Lab_ChangeSlot5ChanceHMN (GOBJ *menu_gobj, int _new_val);
void Lab_ChangeSlot6ChanceHMN (GOBJ *menu_gobj, int _new_val);
void Lab_ChangeSlot1ChanceCPU (GOBJ *menu_gobj, int _new_val);
void Lab_ChangeSlot2ChanceCPU (GOBJ *menu_gobj, int _new_val);
void Lab_ChangeSlot3ChanceCPU (GOBJ *menu_gobj, int _new_val);
void Lab_ChangeSlot4ChanceCPU (GOBJ *menu_gobj, int _new_val);
void Lab_ChangeSlot5ChanceCPU (GOBJ *menu_gobj, int _new_val);
void Lab_ChangeSlot6ChanceCPU (GOBJ *menu_gobj, int _new_val);
void DIDraw_GX();
int Record_GetEndFrame(void);
int Record_GetCurrFrame(void);
int Record_GetSlot(int ply);
bool Record_CPU_IsRecording(void);
bool Record_HMN_IsRecording(void);
bool Record_HMN_IsPlayback(void);
bool Record_CPU_IsPlayback(void);
bool Record_HMN_IsRandomSlot(void);
bool Record_CPU_IsRandomSlot(void);
void Record_CopySlot(GOBJ *menu_gobj);
void Record_DeleteSlot(GOBJ *menu_gobj);
void Record_ChangeHMNSlot(GOBJ *menu_gobj, int value);
void Record_ChangeCPUSlot(GOBJ *menu_gobj, int value);
void Record_ChangeHMNMode(GOBJ *menu_gobj, int value);
void Record_ChangeCPUMode(GOBJ *menu_gobj, int value);
void Record_ChangeMirroredPlayback(GOBJ *menu_gobj, int value);
void Record_ChangeSlot(GOBJ *menu_gobj, int value);
void Record_MemcardSave(GOBJ *menu_gobj);
void Record_MemcardLoad(int slot, int file_no);
void Record_InitState(GOBJ *menu_gobj);
void Record_ResaveState(GOBJ *menu_gobj);
void Record_PruneState(GOBJ *menu_gobj);
void Record_DeleteState(GOBJ *menu_gobj);
void Record_RestoreState(GOBJ *menu_gobj);
void Record_CObjThink(GOBJ *gobj);
void Record_GX(GOBJ *gobj, int pass);
void Record_Think(GOBJ *rec_gobj);
void Record_Update(int ply, RecInputData *inputs, RecInputData *rerecord_inputs, int rec_mode);
void Record_SetInputs(GOBJ *fighter, RecInputs *inputs, bool mirror);
int Record_RearrangeButtons(RecInputs *inputs);
void Record_LoadSavestate(Savestate_v1 *savestate);
int Record_MenuThink(GOBJ *menu_gobj);
int Record_GetRandomSlot(RecInputData **input_data, EventOption slot_menu[]);
int Record_GOBJToID(GOBJ *gobj);
int Record_FtDataToID(FighterData *fighter_data);
int Record_BoneToID(FighterData *fighter_data, JOBJ *bone);
GOBJ *Record_IDToGOBJ(int id);
FighterData *Record_IDToFtData(int id);
JOBJ *Record_IDToBone(FighterData *fighter_data, int id);
void Snap_CObjThink(GOBJ *gobj);
void Record_StartExport(GOBJ *menu_gobj);
void Record_OnSuccessfulSave(int deleteRecordings);
void Export_Init(GOBJ *menu_gobj);
int Export_Think(GOBJ *export_gobj);
void Export_Destroy(GOBJ *export_gobj);
void Export_SelCardInit(GOBJ *export_gobj);
int Export_SelCardThink(GOBJ *export_gobj);
int Export_Compress(u8 *dest, u8 *source, u32 size);
void Export_EnterNameUpdateKeyboard(GOBJ *export_gobj);
void Export_ConfirmExit(GOBJ *export_gobj);
int Export_Process(GOBJ *export_gobj);
void Export_ConfirmInit(GOBJ *export_gobj);
void Export_EnterNameExit(GOBJ *export_gobj);
int Export_EnterNameThink(GOBJ *export_gobj);
void Export_EnterNameInit(GOBJ *export_gobj);
void Export_SelCardExit(GOBJ *export_gobj);
int Export_SelCardThink(GOBJ *export_gobj);
void Export_SelCardInit(GOBJ *export_gobj);
void Export_Destroy(GOBJ *export_gobj);
int Export_Think(GOBJ *export_gobj);
void Export_ConfirmInit(GOBJ *export_gobj);
int Export_ConfirmThink(GOBJ *export_gobj);
void Lab_Exit(GOBJ *menu);
void InfoDisplay_Update(GOBJ *menu_gobj, EventOption menu[], GOBJ *fighter, GOBJ *below);

// info display
#define GXPRI_INFDISP GXPRI_MENUMODEL - 2
#define GXLINK_INFDISP 12
#define GXPRI_INFDISPTEXT GXPRI_INFDISP + 1
#define GXLINK_INFDISPTEXT 12

// info display jobj
#define INFDISP_WIDTH 6
#define INFDISP_SCALE 4
#define INFDISP_X -26.5
#define INFDISP_Y 21.5
#define INFDISP_Z 0.01
#define INFDISP_YOFFSET -2.5
#define INFDISP_BOTY -0.5
#define INFDISP_BOTYOFFSET -0.30
// info display text
#define INFDISPTEXT_SCALE 0.04
#define INFDISPTEXT_X 1.0
#define INFDISPTEXT_Y 1.0
#define INFDISPTEXT_YOFFSET 30

// DI Draw Constants
#define DI_MaxColl 50

static char *stage_names[] = {
    "",
    "",
    "Princess Peach's Castle",
    "Rainbow Cruise",
    "Kongo Jungle",
    "Jungle Japes",
    "Great Bay",
    "Hyrule Temple",
    "Brinstar",
    "Brinstar Depths",
    "Yoshi's Story",
    "Yoshi's Island",
    "Fountain of Dreams",
    "Green Greens",
    "Corneria",
    "Venom",
    "Pokemon Stadium",
    "Poke Floats",
    "Mute City",
    "Big Blue",
    "Onett",
    "Fourside",
    "Icicle Mountain",
    "",
    "Mushroom Kingdom",
    "Mushroom Kingdom II",
    "",
    "Flat Zone",
    "Dream Land",
    "Yoshi's Island (64)",
    "Kongo Jungle (64)",
    "",
    "",
    "",
    "",
    "",
    "Battlefield",
    "Final Destination",
};

// CSS Import
s8 *onload_fileno = R13_OFFSET(-0x4670);
s8 *onload_slot = R13_OFFSET(-0x466F);
#define IMPORT_FILESPERPAGE 10
enum ImportMenuStates
{
    IMP_SELCARD,
    IMP_SELFILE,
    IMP_CONFIRM,
};
enum ImportConfirmKind
{
    CFRM_LOAD,
    CFRM_OLD,
    CFRM_NEW,
    CFRM_NONE,
    CFRM_DEL,
    CFRM_ERR,
};
typedef struct FileInfo
{
    char *file_name; // pointer to file name array
    int file_size;    // number of files on card
    int file_no;      // index of this file on the card
} FileInfo;
typedef struct ImportData
{
    GOBJ *menu_gobj;
    u16 canvas;
    u8 menu_state;
    u8 cursor;
    u8 memcard_inserted[2];     // memcard inserted bools
    u16 memcard_slot;           // selected slot
    JOBJ *memcard_jobj[2];
    JOBJ *screenshot_jobj;
    JOBJ *scroll_jobj;
    JOBJ *scroll_top;
    JOBJ *scroll_bot;
    Text *title_text;
    Text *desc_text;
    Text *option_text;
    Text *filename_text;
    Text *fileinfo_text;
    int file_num;         // number of displayed files on card
    int hidden_file_num;  // number of hidden files on card
    FileInfo *file_info;  // pointer to file info array
    ExportHeader *header; // pointer to header array for the files on the current page
    u8 page;              // file page
    struct
    {
        GOBJ *gobj; // confirm gobj
        u16 canvas;
        u8 kind; // which kind of confirm dialog this is
        u8 cursor;
        Text *text;
    } confirm;
    struct
    {
        _HSD_ImageDesc *orig_image;           // pointer to jobj's original image desc
        RGB565 *image;                        // pointer to 32 byte aligned image allocation (one being displayed)
        int loaded_num;                       // number of completed snaps loaded
        u8 load_inprogress;                   // bool for if a file is being loaded
        u8 file_loading;                      // page local index of the file being loaded
        void *file_data[IMPORT_FILESPERPAGE]; // pointer to each files data
        u8 is_loaded[IMPORT_FILESPERPAGE];    // bools for which snap has been loaded
    } snap;
} ImportData;
void Cam_Button_Create();
void Cam_Button_Think(GOBJ *button_gobj);
void Hazards_Button_Create();
void Hazards_Button_Think(GOBJ *button_gobj);
void Menu_Confirm_Init(GOBJ *menu_gobj, int kind);
void Menu_Confirm_Think(GOBJ *menu_gobj);
void Menu_Confirm_Exit(GOBJ *menu_gobj);
GOBJ *Menu_Create();
void Menu_Think(GOBJ *menu_gobj);
#define MENUCAM_GXLINK 5
#define SIS_ID 0

#pragma GCC diagnostic pop
