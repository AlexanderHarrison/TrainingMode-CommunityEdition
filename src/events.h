#ifndef EVENTS_H
#define EVENTS_H

#include "../MexTK/mex.h"
#include "menu.h"
#include "savestate.h"
#include <stdint.h>

#define TM_VERSSHORT "TM-CE v1.3.1"
#define TM_VERSLONG "TM Community Edition v1.3.1"
#define TM_DEBUG 0 // 0 = release (no logging), 1 = OSReport logs, 2 = onscreen logs
#define EVENT_DATASIZE 512
#define TM_FUNC -(50 * 4)

#define ANALOG_TRIGGER_THRESHOLD 43

// disable all logs in release mode
#if TM_DEBUG == 0
#define TMLOG(...) (void)0
#else
#define TMLOG(...) do { \
    DevelopText_AddString(event_vars->db_console_text, __VA_ARGS__); \
    OSReport(__VA_ARGS__); \
} while (0)
#endif

typedef struct EventMatchData //r8
{
    unsigned int timer : 2;
    unsigned int matchType : 3;
    unsigned int hideGo : 1;
    unsigned int hideReady : 1;
    unsigned int isCreateHUD : 1;       // Display Stocks/Percents/Timer
    unsigned int timerRunOnPause : 1;
    unsigned int isCheckForZRetry : 1;
    unsigned int isShowScore : 1;
    unsigned int isRunStockLogic : 1;
    unsigned int isDisableHit : 1;
    unsigned int useKOCounter : 1;
    unsigned int timerSeconds : 32;   // 0xFFFFFFFF
} EventMatchData;
enum CSSID {
    CSSID_DOCTOR_MARIO   = 1 << 0,
    CSSID_MARIO          = 1 << 1,
    CSSID_LUIGI          = 1 << 2,
    CSSID_BOWSER         = 1 << 3,
    CSSID_PEACH          = 1 << 4,
    CSSID_YOSHI          = 1 << 5,
    CSSID_DONKEY_KONG    = 1 << 6,
    CSSID_CAPTAIN_FALCON = 1 << 7,
    CSSID_GANONDORF      = 1 << 8,
    CSSID_FALCO          = 1 << 9,
    CSSID_FOX            = 1 << 10,
    CSSID_NESS           = 1 << 11,
    CSSID_ICE_CLIMBERS   = 1 << 12,
    CSSID_KIRBY          = 1 << 13,
    CSSID_SAMUS          = 1 << 14,
    CSSID_ZELDA          = 1 << 15,
    CSSID_LINK           = 1 << 16,
    CSSID_YOUNG_LINK     = 1 << 17,
    CSSID_PICHU          = 1 << 18,
    CSSID_PIKACHU        = 1 << 19,
    CSSID_JIGGLYPUFF     = 1 << 20,
    CSSID_MEWTWO         = 1 << 21,
    CSSID_GAME_AND_WATCH = 1 << 22,
    CSSID_MARTH          = 1 << 23,
    CSSID_ROY            = 1 << 24
};
// This order must match the EventJumpTable in Globals.s
enum JumpTableIndex {
    JUMP_AMSAHTECH,
    JUMP_ATTACKONSHIELD,
    JUMP_COMBO,
    JUMP_EGGS,
    JUMP_ESCAPESHIEK,
    JUMP_GRABMASH,
    JUMP_LEDGESTALL,
    JUMP_LEDGETECH,
    JUMP_LEDGETECHCOUNTER,
    JUMP_MULTISHINE,
    JUMP_REACTION,
    JUMP_REVERSAL,
    JUMP_SDITRAINING,
    JUMP_SHIELDDROP,
    JUMP_SIDEBSWEET,
    JUMP_SLIDEOFF,
    JUMP_WAVESHINESDI
};
typedef struct AllowedCharacters
{
    // whitelist bitfields of CSSID_* characters
    int hmn;
    int cpu;
} AllowedCharacters;
typedef struct EventDesc
{
    char *eventName;
    char *eventDescription;
    char *eventFile;
    int jumpTableIndex;
    char *eventCSSFile;
    u8 disable_hazards : 1; // removes stage hazards
    u8 force_sopo : 1;
    u8 CSSType;
    AllowedCharacters allowed_characters;
    s8 playerKind;                    // -1 = use selected fighter
    s8 cpuKind;                       // -1 = no CPU
    s16 stage;                        // -1 = use selected stage
    u8 scoreType;
    u8 callbackPriority;
    EventMatchData *matchData;
} EventDesc;
typedef struct EventPage
{
    char *name;
    int eventNum;
    EventDesc **events;
} EventPage;


typedef struct evFunction
{
    void (*Event_Init)(GOBJ *event);
    void (*Event_Update)(void);
    void (*Event_Think)(GOBJ *event);
    EventMenu **menu_start;
} evFunction;

typedef struct EventVars
{
    EventDesc *event_desc;                                                                   // event information
    evMenu *menu_assets;                                                                     // menu assets
    GOBJ *event_gobj;                                                                        // event gobj
    GOBJ *menu_gobj;                                                                         // event menu gobj
    void *persistent_data;                                                                   // persistent data to be accessed from both C and ASM
    int game_timer;                                                                          // amount of game frames passed
    int (*Savestate_Save_v1)(Savestate_v1 *savestate, int flags);                                  // function pointer to save state
    int (*Savestate_Load_v1)(Savestate_v1 *savestate, int flags);                                  // function pointer to load state
    GOBJ *(*Message_Display)(int msg_kind, int queue_num, int msg_color, char *format, ...); // function pointer to display message
    int *(*Tip_Display)(int lifetime, char *fmt, ...);
    void (*Tip_Destroy)(void);      // function pointer to destroy tip
    Savestate_v1 *savestate;       // points to the events main savestate

    // To allow minor savestates during mirrored playback, 
    // we need to record if the minor savestate was saved duting mirroring.
    // Otherwise, the savestate will be loaded as mirrored AGAIN.
    int savestate_saved_while_mirrored;
    int loaded_mirrored;

    evFunction evFunction;      // event specific functions
    HSD_Archive *event_archive; // event archive header
    DevText *db_console_text;
} EventVars;
#define event_vars_ptr_loc ((EventVars**)0x803d7054)
#define event_vars (*event_vars_ptr_loc)

// Function prototypes
EventDesc *GetEventDesc(int page, int event);
void EventInit(int page, int eventID, MatchInit *matchData);
void EventLoad(void);
int Text_AddSubtextManual(Text *text, char *string, int posx, int posy, int scalex, int scaley);
int GOBJToID(GOBJ *gobj);
int FtDataToID(FighterData *fighter_data);
int BoneToID(FighterData *fighter_data, JOBJ *bone);
GOBJ *IDToGOBJ(int id);
FighterData *IDToFtData(int id);
JOBJ *IDToBone(FighterData *fighter_data, int id);
void EventUpdate(void);
void Event_IncTimer(GOBJ *gobj);
void Test_Think(GOBJ *gobj);
void Hazards_Disable(void);

// GX Link args
#define GXLINK_MENUMODEL 12
#define GXPRI_MENUMODEL 80
#define GXLINK_MENUTEXT 12
#define GXPRI_MENUTEXT GXPRI_MENUMODEL + 1

// Message
void Message_Init(void);
GOBJ *Message_Display(int msg_kind, int queue_num, int msg_color, char *format, ...);
void Message_Manager(GOBJ *mngr_gobj);
void Message_Destroy(GOBJ **msg_queue, int msg_num);
void Message_Add(GOBJ *msg_gobj, int queue_num);
void Message_CObjThink(GOBJ *gobj);

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

enum MsgColors
{
    MSGCOLOR_WHITE,
    MSGCOLOR_GREEN,
    MSGCOLOR_RED,
    MSGCOLOR_YELLOW
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
void Tip_Destroy(void); // 0 = immediately destroy, 1 = force exit
void Tip_Think(GOBJ *gobj);

#define TIP_TXTJOINT 2

#endif
