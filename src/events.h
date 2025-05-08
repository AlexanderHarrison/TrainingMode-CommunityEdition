#include "../MexTK/mex.h"

#define TM_VERSSHORT "TM-CE v1.2 d5"
#define TM_VERSLONG "TM Community Edition v1.2 d5"
#define TM_DEBUG 0 // 0 = release (no logging), 1 = OSReport logs, 2 = onscreen logs
#define EVENT_DATASIZE 512
#define TM_FUNC -(50 * 4)
#define MENU_MAXOPTION 9
#define MENU_POPMAXOPTION 5
#define countof(A) (sizeof(A)/sizeof(*A))

#define TMLOG(...) DevelopText_AddString(event_vars->db_console_text, __VA_ARGS__)

// disable all logs in release mode
#if TM_DEBUG == 0
#define TMLOG (void)sizeof
//#define assert (void)sizeof
#endif

// use OSReport for all logs
#if TM_DEBUG == 1
#define OSReport OSReport
#endif

// use TMLog for all logs
#if TM_DEBUG == 2
#define OSReport TMLOG
#endif

#define SHORTCUT_BUTTONS (HSD_BUTTON_A | HSD_BUTTON_B | HSD_BUTTON_X | HSD_TRIGGER_Z)

// Custom File Structs
typedef struct evMenu
{
    JOBJDesc *menu;
    JOBJDesc *popup;
    JOBJDesc *scroll;
    JOBJDesc *check;
    JOBJDesc *arrow;
    JOBJDesc *playback;
    JOBJDesc *message;
    COBJDesc *hud_cobjdesc;
    JOBJ *tip_jobj;
    void **tip_jointanim; // pointer to array
} evMenu;

// Structure Definitions
typedef struct EventMenu EventMenu;
typedef struct EventMatchData //r8
{
    unsigned int timer : 2;
    unsigned int matchType : 3;
    unsigned int isDisableMusic : 1;
    unsigned int hideGo : 1;
    unsigned int hideReady : 1;
    unsigned int isCreateHUD : 1;
    unsigned int isDisablePause : 1;
    unsigned int timerRunOnPause : 1;    // 0x01
    unsigned int isHidePauseHUD : 1;     // 0x02
    unsigned int isShowLRAStart : 1;     // 0x04
    unsigned int isCheckForLRAStart : 1; // 0x08
    unsigned int isShowZRetry : 1;       // 0x10
    unsigned int isCheckForZRetry : 1;   // 0x20
    unsigned int isShowAnalogStick : 1;  // 0x40
    unsigned int isShowScore : 1;        // 0x80
    unsigned int isRunStockLogic : 1;    // 0x20
    unsigned int isDisableHit : 1;       // 0x20
    unsigned int useKOCounter : 1;
    s8 playerKind;                    // -1 = use selected fighter
    s8 cpuKind;                       // -1 = no CPU
    s16 stage;                        // -1 = use selected stage
    unsigned int timerSeconds : 32;   // 0xFFFFFFFF
    unsigned int timerSubSeconds : 8; // 0xFF
} EventMatchData;
typedef struct EventDesc
{
    char *eventName;
    char *eventDescription;
    char *eventFile;
    char *eventCSSFile;
    u8 isSelectStage : 1;
    u8 use_savestates : 1;  // enables dpad left and right savestates
    u8 disable_hazards : 1; // removes stage hazards
    u8 force_sopo : 1;
    u8 CSSType;
    u8 scoreType;
    u8 callbackPriority;
    EventMatchData *matchData;
    int defaultOSD;
} EventDesc;
typedef struct EventPage
{
    char *name;
    int eventNum;
    EventDesc **events;
} EventPage;
typedef struct EventOption
{
    u8 kind;                                        // the type of option this is; string, integers, etc
    u8 disable;                                     // boolean for disabling the option
    s16 value_min;                                  // minimum value
    u16 value_num;                                  // number of values
    s16 val;                                        // value of this option
    EventMenu *menu;                                // pointer to the menu that pressing A opens
    char *name;                                     // pointer to the name of this option
    char *desc;                                     // pointer to the description string for this option
    void **values;                                  // pointer to an array of strings
    void (*OnChange)(GOBJ *menu_gobj, int value);   // function that runs when option is changed
    void (*OnSelect)(GOBJ *menu_gobj);              // function that runs when option is selected
} EventOption;
typedef struct Shortcut {
    int buttons_mask;
    EventOption *option;
} Shortcut;
typedef struct ShortcutList {
    int count;
    Shortcut *list;
} ShortcutList;
struct EventMenu
{
    char *name;                    // name of this menu
    u8 option_num;                 // number of options this menu contains
    u8 scroll;                     //
    u8 state;                      // bool used to know if this menu is focused
    u8 cursor;                     // index of the option currently selected
    EventOption *options;          // pointer to all of this menu's options
    EventMenu *prev;               // pointer to previous menu, used at runtime
    int (*menu_think)(GOBJ *menu); // function that runs every frame.
    ShortcutList *shortcuts;       // pointer to shortcuts when shortcut mode is entered on this menu
};
typedef enum MenuMode {
    MenuMode_Normal,
    MenuMode_Paused,
    MenuMode_Shortcut,
    MenuMode_ShortcutWaitForRelease,
} MenuMode;
typedef struct MenuData
{
    EventDesc *event_desc;
    EventMenu *currMenu;
    u16 canvas_menu;
    u16 canvas_popup;
    u8 mode;
    u8 controller_index; // index of the controller who paused
    Text *text_name;
    Text *text_value;
    Text *text_popup;
    Text *text_title;
    Text *text_desc;
    u16 popup_cursor;
    u16 popup_scroll;
    GOBJ *popup;
    evMenu *menu_assets;
    JOBJ *row_joints[MENU_MAXOPTION][2]; // pointers to row jobjs
    JOBJ *highlight_menu;                // pointer to the highlight jobj
    JOBJ *highlight_popup;               // pointer to the highlight jobj
    JOBJ *scroll_top;
    JOBJ *scroll_bot;
    GOBJ *custom_gobj;                               // onSelect gobj
    int (*custom_gobj_think)(GOBJ *custom_gobj);     // per frame function. Returns bool indicating if the program should check to unpause
    void *(*custom_gobj_destroy)(GOBJ *custom_gobj); // on destroy function
} MenuData;

typedef struct FtSaveStateData
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
    struct fighter_var fighter_var;    // 0x222c
    struct state_var state_var;        // 0x2340
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
} FtSaveStateData;

typedef struct FtSaveState
{
    FtSaveStateData data[2];
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
} FtSaveState;

typedef struct Savestate
{
    int is_exist;
    int frame;
    u8 event_data[EVENT_DATASIZE];
    FtSaveState ft_state[6];
} Savestate;

typedef struct evFunction
{
    void (*Event_Init)(GOBJ *event);
    void (*Event_Update)();
    void (*Event_Think)(GOBJ *event);
    EventMenu **menu_start;
} evFunction;

typedef struct EventVars
{
    EventDesc *event_desc;                                                                   // event information
    evMenu *menu_assets;                                                                     // menu assets
    GOBJ *event_gobj;                                                                        // event gobj
    GOBJ *menu_gobj;                                                                         // event menu gobj
    int game_timer;                                                                          // amount of game frames passed
    u8 hide_menu;                                                                            // enable this to hide the base menu. used for custom menus.
    int (*Savestate_Save)(Savestate *savestate);                                             // function pointer to save state
    int (*Savestate_Load)(Savestate *savestate, int is_mirrored);                            // function pointer to load state
    GOBJ *(*Message_Display)(int msg_kind, int queue_num, int msg_color, char *format, ...); // function pointer to display message
    int *(*Tip_Display)(int lifetime, char *fmt, ...);
    void (*Tip_Destroy)();      // function pointer to destroy tip
    Savestate *savestate;       // points to the events main savestate

    // To allow minor savestates during mirrored playback, 
    // we need to record if the minor savestate was saved duting mirroring.
    // Otherwise, the savestate will be loaded as mirrored AGAIN.
    int savestate_saved_while_mirrored;
    int loaded_mirrored;

    evFunction evFunction;      // event specific functions
    HSD_Archive *event_archive; // event archive header
    DevText *db_console_text;
} EventVars;

// Function prototypes
EventDesc *GetEventDesc(int page, int event);
void EventInit(int page, int eventID, MatchInit *matchData);
void EventLoad();
GOBJ *EventMenu_Init(EventDesc *event_desc, EventMenu *start_menu);
void EventMenu_Think(GOBJ *eventMenu, int pass);
void EventMenu_COBJThink(GOBJ *gobj);
void EventMenu_Draw(GOBJ *eventMenu);
int Text_AddSubtextManual(Text *text, char *string, int posx, int posy, int scalex, int scaley);
EventMenu *EventMenu_GetCurrentMenu(GOBJ *gobj);
int Savestate_Save(Savestate *savestate);
int Savestate_Load(Savestate *savestate, int is_mirrored);
void Update_Savestates();
int GOBJToID(GOBJ *gobj);
int FtDataToID(FighterData *fighter_data);
int BoneToID(FighterData *fighter_data, JOBJ *bone);
GOBJ *IDToGOBJ(int id);
FighterData *IDToFtData(int id);
JOBJ *IDToBone(FighterData *fighter_data, int id);
void EventUpdate();
void Event_IncTimer(GOBJ *gobj);
void Test_Think(GOBJ *gobj);
void Hazards_Disable();
static EventDesc *static_eventInfo;
static EventVars stc_event_vars;
static int *eventDataBackup;

static EventVars **event_vars_ptr = 0x803d7054; //R13 + (-0x4730)
static EventVars *event_vars;

// EventOption kind definitions
enum option_kind {
    OPTKIND_MENU,
    OPTKIND_STRING,
    OPTKIND_INT,
    OPTKIND_FLOAT,
    OPTKIND_FUNC,
};

// EventMenu state definitions
enum event_menu_state {
    EMSTATE_FOCUS,
    EMSTATE_OPENSUB,
    EMSTATE_OPENPOP,
    EMSTATE_WAIT, // pauses menu logic, used for when a custom window is being shown
};

// GX Link args
#define GXLINK_MENUMODEL 12
#define GXPRI_MENUMODEL 80
#define GXLINK_MENUTEXT 12
#define GXPRI_MENUTEXT GXPRI_MENUMODEL + 1
// popup menu
#define GXPRI_POPUPMODEL GXPRI_MENUTEXT + 1
#define GXLINK_POPUPMODEL 12
#define GXPRI_POPUPTEXT GXPRI_POPUPMODEL + 1
#define GXLINK_POPUPTEXT 12
// cobj
#define MENUCAM_COBJGXLINK (1 << GXLINK_MENUMODEL) | (1 << GXLINK_MENUTEXT) | (1 << GXLINK_POPUPMODEL) | (1 << GXLINK_POPUPTEXT)
#define MENUCAM_GXPRI 9

// menu model
#define OPT_SCALE 1
#define OPT_X 0 //0.5
#define OPT_Y -1
#define OPT_Z 0
#define OPT_WIDTH 55 / OPT_SCALE
#define OPT_HEIGHT 40 / OPT_SCALE
// menu text object
#define MENU_CANVASSCALE 0.05
#define MENU_TEXTSCALE 1
#define MENU_TEXTZ 0
// menu title
#define MENU_TITLEXPOS -430
#define MENU_TITLEYPOS -366
#define MENU_TITLESCALE 2.3
#define MENU_TITLEASPECT 870
// menu description
#define MENU_DESCXPOS -21.5
#define MENU_DESCYPOS 12
#define MENU_DESCSCALE 1
// menu option name
#define MENU_OPTIONNAMEXPOS -430
#define MENU_OPTIONNAMEYPOS -230
#define MENU_NAMEASPECT 440
// menu option value
#define MENU_OPTIONVALXPOS 250
#define MENU_OPTIONVALYPOS -230
#define MENU_TEXTYOFFSET 50
#define MENU_VALASPECT 280
// menu highlight
#define MENUHIGHLIGHT_SCALE 1 // OPT_SCALE
#define MENUHIGHLIGHT_HEIGHT ROWBOX_HEIGHT
#define MENUHIGHLIGHT_WIDTH (OPT_WIDTH * 0.785)
#define MENUHIGHLIGHT_X OPT_X
#define MENUHIGHLIGHT_Y 10.8 //10.3
#define MENUHIGHLIGHT_Z 0.01
#define MENUHIGHLIGHT_YOFFSET ROWBOX_YOFFSET
#define MENUHIGHLIGHT_COLOR \
    {                       \
        255, 211, 0, 255    \
    }
// menu scroll
#define MENUSCROLL_SCALE 2                         // OPT_SCALE
#define MENUSCROLL_SCALEY 1.105 * MENUSCROLL_SCALE // OPT_SCALE
#define MENUSCROLL_X 22.5
#define MENUSCROLL_Y 12
#define MENUSCROLL_Z 0.01
#define MENUSCROLL_PEROPTION 1
#define MENUSCROLL_MINLENGTH -1
#define MENUSCROLL_MAXLENGTH -10
#define MENUSCROLL_COLOR \
    {                    \
        255, 211, 0, 255 \
    }

// row jobj
#define ROWBOX_HEIGHT 2.3
#define ROWBOX_WIDTH 18
#define ROWBOX_X 12.5 //13
#define ROWBOX_Y 10.8 //10.3
#define ROWBOX_Z 0
#define ROWBOX_YOFFSET -2.5
#define ROWBOX_COLOR       \
    {                      \
        104, 105, 129, 100 \
    }
// arrow jobj
#define TICKBOX_SCALE 1.8
#define TICKBOX_X 11.7
#define TICKBOX_Y 11.7

// popup model
#define POPUP_WIDTH ROWBOX_WIDTH
#define POPUP_HEIGHT 19
#define POPUP_SCALE 1
#define POPUP_X 12.5
#define POPUP_Y 8.3
#define POPUP_Z 0.01
#define POPUP_YOFFSET -2.5
// popup text object
#define POPUP_CANVASSCALE 0.05
#define POPUP_TEXTSCALE 1
#define POPUP_TEXTZ 0.01
// popup text
#define POPUP_OPTIONVALXPOS 250
#define POPUP_OPTIONVALYPOS -280
#define POPUP_TEXTYOFFSET 50
// popup highlight
#define POPUPHIGHLIGHT_HEIGHT ROWBOX_HEIGHT
#define POPUPHIGHLIGHT_WIDTH (POPUP_WIDTH * 0.785)
#define POPUPHIGHLIGHT_X 0
#define POPUPHIGHLIGHT_Y 5
#define POPUPHIGHLIGHT_Z 1
#define POPUPHIGHLIGHT_YOFFSET ROWBOX_YOFFSET
#define POPUPHIGHLIGHT_COLOR \
    {                        \
        255, 211, 0, 255     \
    }

// Message
void Message_Init();
GOBJ *Message_Display(int msg_kind, int queue_num, int msg_color, char *format, ...);
void Message_Manager(GOBJ *mngr_gobj);
void Message_Destroy(GOBJ **msg_queue, int msg_num);
void Message_Add(GOBJ *msg_gobj, int queue_num);
void Message_CObjThink(GOBJ *gobj);
float BezierBlend(float t);

#define MSGQUEUE_NUM 7
#define MSGQUEUE_SIZE 8
#define MSGQUEUE_GENERAL 6
enum MsgState
{
    MSGSTATE_WAIT,
    MSGSTATE_SHIFT,
    MSGSTATE_DELETE,
};
enum MsgArea
{
    MSGKIND_P1,
    MSGKIND_P2,
    MSGKIND_P3,
    MSGKIND_P4,
    MSGKIND_P5,
    MSGKIND_P6,
    MSGKIND_GENERAL,
};
typedef struct MsgData
{
    Text *text;      // text pointer
    int kind;        // the type of message this is
    int state;       // unused atm
    int prev_index;  // used to animate the messages position during shifts
    int orig_index;  // used to tell if the message moved throughout the course of the frame
    int anim_timer;  // used to track animation frame
    int lifetime;    // amount of frames after spawning to kill this message
    int alive_timer; // amount of frames this message has been alive for
} MsgData;
typedef struct MsgMngrData
{
    COBJ *cobj;
    int state;
    int canvas;
    GOBJ *msg_queue[MSGQUEUE_NUM][MSGQUEUE_SIZE]; // array 7 is for miscellaneous messages, not related to a player
} MsgMngrData;
static GOBJ *stc_msgmgr;

static Vec2 stc_msg_queue_offsets_vertical[] = {
    {0, 5.15f},
    {0, 5.15f},
    {0, 5.15f},
    {0, 5.15f},
    {0, 5.15f},
    {0, 5.15f},
    {0, -5.15f}
};
static Vec3 stc_msg_queue_pos_sides[] = {
    {-22.f, -13.f, 0},
    {-7.f, -13.f, 0},
    {7.f, -13.f, 0},
    {22.f, -13.f, 0},
    {0, 0, 0},
    {0, 0, 0},
    {0, -5.15f}
};

static Vec2 stc_msg_queue_offsets_horizontal[] = {
    {12.5f, 0},
    {12.5f, 0},
    {12.5f, 0},
    {12.5f, 0},
    {12.5f, 0},
    {12.5f, 0},
    {-12.5f, 0},
};
static Vec3 stc_msg_queue_pos_top[] = {
    {-22.f, 20.f, 0},
    {-22.f, 14.f, 0},
    {-22.f, 8.f, 0},
    {-22.f, 2.f, 0},
    {0, 0, 0},
    {0, 0, 0},
    {22.f, 20.f, 0}
};

enum MsgColors
{
    MSGCOLOR_WHITE,
    MSGCOLOR_GREEN,
    MSGCOLOR_RED,
    MSGCOLOR_YELLOW
};
static GXColor stc_msg_colors[] = {
    {255, 255, 255, 255},
    {141, 255, 110, 255},
    {255, 162, 186, 255},
    {255, 240, 0, 255},
};

#define MSGTIMER_SHIFT 6
#define MSGTIMER_DELETE 6
#define MSG_LIFETIME (2 * 60)
#define MSG_LINEMAX 3  // lines per message
#define MSG_CHARMAX 32 // characters per line
#define MSG_HUDYOFFSET 8
#define MSGJOINT_SCALE 3
#define MSGJOINT_X 0
#define MSGJOINT_Y 0
#define MSGJOINT_Z 0
#define MSGTEXT_BASESCALE 1.4
#define MSGTEXT_BASEWIDTH (330 / MSGTEXT_BASESCALE)
#define MSGTEXT_BASEX 0
#define MSGTEXT_BASEY -1
#define MSGTEXT_YOFFSET 30

// GX stuff
#define MSG_GXLINK 13
#define MSG_GXPRI 80
#define MSGTEXT_GXPRI MSG_GXPRI + 1
#define MSG_COBJLGXLINKS (1 << MSG_GXLINK)
#define MSG_COBJLGXPRI 8

typedef struct TipMgr
{
    GOBJ *gobj;   // tip gobj
    Text *text;   // tip text object
    int state;    // state this tip is in. 0 = in, 1 = wait, 2 = out
    int lifetime; // tips time spent onscreen
} TipMgr;

int Tip_Display(int lifetime, char *fmt, ...);
void Tip_Destroy(); // 0 = immediately destroy, 1 = force exit
void Tip_Think(GOBJ *gobj);

#define TIP_TXTJOINT 2
