#ifndef MEX_H_CSS
#define MEX_H_CSS

#include "structs.h"
#include "datatypes.h"
#include "fighter.h"
#include "scene.h"

#define MENU_BUTTON_UP 0x1
#define MENU_BUTTON_DOWN 0x2
#define MENU_BUTTON_LEFT 0x4
#define MENU_BUTTON_RIGHT 0x8
#define MENU_BUTTON_A 0x10
#define MENU_BUTTON_B 0x20
#define MENU_BUTTON_L 0x40
#define MENU_BUTTON_R 0x80
#define MENU_BUTTON_START 0x100
#define MENU_BUTTON_X 0x400
#define MENU_BUTTON_Y 0x800

enum CSSKind
{
    SLCHRKIND_VS,              // regular vs
    SLCHRKIND_SNAPSHOT,        // camera mode
    SLCHRKIND_STAMINA,         //
    SLCHRKIND_SUDDEN,          //
    SLCHRKIND_GIANT,           //
    SLCHRKIND_TINY,            //
    SLCHRKIND_INVISIBLE,       //
    SLCHRKIND_FIXEDCAM,        //
    SLCHRKIND_SNGLBTN,         //
    SLCHRKIND_FAST,            //
    SLCHRKIND_SLOW,            //
    SLCHRKIND_CLASSIC,         //
    SLCHRKIND_ADV,             //
    SLCHRKIND_ALLSTAR,         //
    SLCHRKIND_EVENT,           //
    SLCHRKIND_TARGET,          //
    SLCHRKIND_HOMERUN,         //
    SLCHRKIND_MULTIMAN10,      //
    SLCHRKIND_MULTIMAN100,     //
    SLCHRKIND_MULTIMAN3MIN,    //
    SLCHRKIND_MULTIMAN15MIN,   //
    SLCHRKIND_MULTIMANENDLESS, //
    SLCHRKIND_MULTIMANCRUEL,   //
    SLCHRKIND_TRAINING,        //
};

enum CSSExitKind
{
    CSSEXIT_NONE,        // can be used to identify LRStart to go back to main menu
    CSSEXIT_SSS,         //
    CSSEXIT_MAINMENU,    //
    CSSEXIT_RULES,       //
    CSSEXIT_NAME,        // name entry
    CSSEXIT_SUBMENUOPEN, // in rules / name entry
};

/*** Structs ***/
struct CSSBackup
{
    u8 c_kind;  // 0x0
    u8 costume; // 0x1
    u8 nametag; // 0x2
    u8 event;   // 0x3
    u8 port;    // 0x4
    u8 x5;      // 0x5
    u8 x6;      // 0x6
    u8 x7;      // 0x7
    u8 x8;      // 0x8
};

struct MnSelectChrDataTable
{
    COBJDesc *cobj;
    void *lobj1;
    void *lobj2;
    void *fog;
    struct
    {
        JOBJSet bg;
        JOBJSet cursor;
        JOBJSet puck;
        JOBJSet vsmenu;
        JOBJSet start;
        JOBJSet camdoor;
        JOBJSet solomenu;
        JOBJSet solooption;
        JOBJSet cpudoor;
    } jobjset;
};

enum CSSCursorState
{
    SLCHRCUR_POINT,
    SLCHRCUR_HOLD,
    SLCHRCUR_OPEN,
    SLCHRCUR_HIDDEN,
};
struct CSSCursor
{
    GOBJ *gobj;         // 0x0
    u8 door_id;         // 0x4, actually door index
    u8 state;           // 0x5, 0x0 = Pointing / 0x1 = Holding Puck / 0x2 = Open Hand / 0x3 = Hidden/Unplugged
    u8 puck;            // 0x6, puck index being held
    u8 x7;              // 0x7,
    u16 is_over_option; // 0x8, is hovered over an option on the top of the CSS (rules, teams, back)
    u16 exit_timer;     // 0xa, frames held B
    Vec2 pos;           // 0xc-0x10
};

enum CSSObjectKind
{
    SLCHROBJKIND_P1PUCK,
    SLCHROBJKIND_P2PUCK,
    SLCHROBJKIND_P3PUCK,
    SLCHROBJKIND_P4PUCK,
    SLCHROBJKIND_P1CPU,
    SLCHROBJKIND_P2CPU,
    SLCHROBJKIND_P3CPU,
    SLCHROBJKIND_P4CPU,
    SLCHROBJKIND_P1HANDICAP,
    SLCHROBJKIND_P2HANDICAP,
    SLCHROBJKIND_P3HANDICAP,
    SLCHROBJKIND_P4HANDICAP,
};
enum CSSPuckState
{
    SLCHRPUCK_0,
    SLCHRPUCK_1,
    SLCHRPUCK_2,
    SLCHRPUCK_3,
};
struct CSSPuck
{
    GOBJ *gobj;       // 0x0,
    u8 door_id;       // 0x4, port this puck belongs to
    u8 state;         // 0x5,
    u8 kind;          // 0x6, port that this puck is being held by
    u8 anim_timer;    // 0x7, Animation Timer. resets animation when this hits 39, checked for @ 80262790
    Vec2 pos_proj;    // 0x8, where the puck should be
    Vec2 pos_correct; // 0x10, where the puck is corrected to
};

struct MnSlChrIcon
{
    u8 ft_hudindex; // 0x0, used for getting combo count @ 8025c0c4
    u8 c_kind;      // 0x1, icons external ID
    u8 state;       // 0x2, Dictates whether icon can be chosen. 0x0 = Not Unlocked, 0x1 = Unlocked (temp value), 0x2 = Unlocked and Displayed
    u8 anim_timer;  // 0x3, is made to be 0xC when the character is chosen.
    u8 joint_id_vs; // 0x4, vs icon background jobj ID
    u8 joint_id_1p; // 0x5, 1p icon background jobj ID
    int sfx;        // 0x8,
    float bound_l;  // 0xC
    float bound_r;  // 0x10
    float bound_u;  // 0x14
    float bound_d;  // 0x18
};

struct MnSlChrDoor
{
    u8 emblem_joint;            // 0x0
    u8 costume_joint;           // 0x1
    u8 team_joint;              // 0x2
    u8 door_joint;              // 0x3
    u8 bg_joint;                // 0x4
    u8 player_indicator_joint;  // 0x5, nametag window joint id (to scroll and choose a name)
    u8 slidername_joint;        // 0x6, slider name joint
    u8 cpuslider_joint;         // 0x7, used when only CPU is showing
    u8 cpuslider2_joint;        // 0x8, used when handicap is also showing
    u8 selected_since_load;     // 0x9, used to determine when the player made a selection since the CSS loaded
    u8 team;                    // 0xa
    u8 p_kind;                  // 0xb, PlayerKind, 0x0 = HMN, 0x1 = CPU, 0x3 = Closed
    u8 p_kind_prev;             // 0xc
    u8 costume;                 // 0xd
    u8 sel_icon;                // 0xe, icon this player has selected
    u8 sel_icon_prev;           // 0xf
    u8 dooranim_timer;          // 0x10
    u8 slideranim_timer;        // 0x11
    u8 is_hold_cpu_slider;      // 0x12
    u8 is_hold_handicap_slider; // 0x13
    float togglebtn_left;       // 0x14, HMN button bound
    float togglebtn_right;      // 0x18, HMN button bound
    float teambtn_left;         // 0x1C, team button bound
    float teambtn_right;        // 0x20, team button bound
};

struct MnSlChrTagData
{
    Text *name;         // 0x0
    Text *namelist;     // 0x4, used when opening the tag window
    float x8;           // 0x8
    float scroll_amt;   // xC, Text Y Offset to scroll up each frame
    float scroll_force; // x10
    int timer;          // x14, state timer
    u8 next_tag;        // 0x18, index of next empty tag
    u8 port;
    u8 state;   // 0x1a, is nametag window open bool
    u8 use_tag; // 0x1b, is using a nametag bool
};

struct MnSlChrTag
{
    MnSlChrTagData *tag_data; // 0x0
    u8 x4;                    // 0x4
    u8 list_joint;            // 0x5
    u8 name_joint;            // 0x6
    u8 x7;                    // 0x7
    u8 kostartext_joint;      // 0x8
    u8 x9;
    u8 xa;
    u8 xb;
};

struct MnSlChrKOStar
{
    Text *text; // 0x0
    float x4;   // 0x4
    u8 joint;   // 0x8
    int xc;
    int x10;
    int x14;
    int x18;
    int x1c;
};

struct MnSlChrKindData
{
    u16 mode_ffa_frame;   // 0x0, anim frame used for the top left mode texture
    u16 mode_teams_frame; // 0x2, anim frame used for the top left mode texture
    int enter_sfx;        // 0x4, announcer sfx used when entering the CSS
};

struct MnSlChrData
{
    u8 GaWName[0x1C];              // 0x0
    MnSlChrKindData kind_data[24]; // 0x1c
    MnSlChrIcon icons[25 + 1];     // 0xDC
    MnSlChrDoor doors[4];          // 0x3b4
    MnSlChrTag tags[4];            // 0x444
    u8 x474;                       // 0x474
    u8 x475;                       // 0x475
    u8 x476;                       // 0x476
    u8 x477;                       // 0x477
    u8 x478;                       // 0x478
    u8 name_list_joint;            // 0x479, for singleplayer
    u8 tag_box_joint;              // 0x47a, for singleplayer
    u8 x47b;                       // 0x47b
    u8 x47c;                       // 0x47c
    u8 x47d;                       // 0x47d
    u8 x47e;                       // 0x47e
    u8 x47f;                       // 0x47f
    u8 x480;                       // 0x480
    u8 x481;                       // 0x481
    u8 x482;                       // 0x482
    u8 x483;                       // 0x483
    int x484;                      // 0x484
    int x488;                      // 0x488
    float x48c;                    // 0x48c
    float x490;                    // 0x490
    float x494;                    // 0x494
    float x498;                    // 0x498
    float x49c;                    // 0x49c
    float x4a0;                    // 0x4a0
    u8 x4a4[5];                    // 0x4a4
    float x4ac;                    // 0x4ac
    float x4b0;                    // 0x4b0
    float x4b4;                    // 0x4b4
    float x4b8;                    // 0x4b8
    float x4bc;                    // 0x4bc
    float x4c0;                    // 0x4c0
    MnSlChrKOStar ko_stars[4];     // 0x4c4
};

struct VSMinorData
{
    u16 snglply_port; // 0x0, port index for single player (1 indexed)
    u8 css_kind;      // 0x2, 0 = VS
    u8 exit_kind;     // 0x3, 1 = advance, 2 = leave
    u8 *ko_data;      // 0x4, used for displaying KO stars on CSS
    ScDataVS vs_data; // 0x8
};
struct SSSMinorData
{
    u8 x0; // is made 0 upon entry
    u8 x1;
    u8 x2;
    u8 auto_random_stage; // is -1 if random stage in rules is disabled
    u8 exit_kind;         //  0 = back, 1 = advance
    ScDataVS vs_data;
};

/*** Variables ***/
static MnSlChrData *stc_css_data = (void *)0x803F0A48;         // 0x803f0a48
static VSMinorData **stc_css_minorscene = (void *)0x804D6CB0;  // -0x49F0
static u8 *stc_css_regtagnum = (void *)0x804D6CF8;             // -0x49A8, number of registered tags
static s8 *stc_css_name_ply = (void *)0x804D6CF9;              // -0x49A7, index of the player using the name entry menu
static HSD_Archive **stc_css_archive = (void *)0x804D6CD0;     // -0x49D0
static HSD_Archive **stc_css_menuarchive = (void *)0x804D6CD4; // -0x49CC, ptr to MnMaExt archive
static u8 *stc_css_custom_rules = (void *)0x804D6CF4;          // -0x49AC
static GOBJ **stc_css_menugobj = (void *)0x804D6CBC;           // -0x49E4
static JOBJ **stc_css_menumodel = (void *)0x804D6CC0;          // -0x49E0
static JOBJ **stc_css_trainingmodel = (void *)0x804D6CC4;      // -0x49DC
static JOBJ **stc_css_highscoremodel = (void *)0x804D6CC8;     // -0x49D8
static JOBJ **stc_css_cameramodel = (void *)0x804D6CCC;        // -0x49D4
static CSSCursor **stc_css_cursors = (void *)0x804A0BC0;       // 0x804a0bc0
static CSSPuck **stc_css_pucks = (void *)0x804A0BD0;           // 0x804a0bd0

static s8 *stc_css_hmnport = (void *)0x804D6CF0;                      // -0x49B0
static s8 *stc_css_cpuport = (void *)0x804D6CF1;                      // -0x49AF
static u8 *stc_css_delay = (void *)0x804D6CF2;                        // -0x49AE
static u8 *stc_css_exitkind = (void *)0x804D6CF6;                     // -0x49AA
static u8 *stc_css_maxply = (void *)0x804D6CF5;                       // -0x49AB
static u8 *stc_css_is_ready_timer = (void *)0x804D6CF7;               // -0x49A9
static u8 *stc_css_singeplyport = (void *)0x804D6CF0;                 // -0x4DE0
static u8 *stc_menu_singeplyport = (void *)0x804D6598;                // -0x4DB8
static Text **stc_css_ply1_combo_text = (void *)0x804D6CDC;           // -0x49C4
static Text **stc_css_ply2_combo_text = (void *)0x804D6CE0;           // -0x49C0
static Text **stc_css_ply3_combo_text = (void *)0x804D6CE4;           // -0x49BC
static Text **stc_css_ply4_combo_text = (void *)0x804D6CE8;           // -0x49B8
static int *stc_css_bgtimer = (void *)0x804D6CEC;                     // -0x49B4
static u8 *stc_css_hasreleasedb = (void *)0x804D6CF3;                 // -0x49AD
static MnSelectChrDataTable **stc_css_datatable = (void *)0x804D6CB4; // -0x49EC
static JOBJSet **stc_css_jobjsets = (void *)0x804D6CD8;               // -0x49C8
static COBJDesc **stc_css_cobjdesc = (void *)0x804D6BC4;              // -0x4ADC
static GOBJ **stc_css_camgobj = (void *)0x804D6CB8;                   // -0x49E8
static HSD_Pad *stc_css_pad = (void *)0x804C20BC;                     // 0x804c20bc
static u8 *stc_css_unkarr = (void *)0x804D50C8;                       // 0x804d50c8

/*** Functions ***/
void MainMenu_CamRotateThink(GOBJ *gobj);
int MainMenu_GetPadDown(int controller_index);  // returns HSD_BUTTON vals
u64 MainMenu_GetPadRapid(int controller_index); // returns HSD_BUTTON vals
int MainMenu_CheckForLRA();
void MainMenu_DestroyAllTextCanvases(); // destroys all SIS canvases
void CSS_FreeText();                    //
int CSS_GetNametagRumble(int player, u8 tag);
void CSS_InitPlayerData(PlayerData *player);
void CSS_MenuModelThink(GOBJ *gobj);
void CSS_CursorThink(GOBJ *gobj);
void CSS_PuckThink(GOBJ *gobj);
void CSS_TagThink(GOBJ *gobj);
void CSS_UpdateRulesText();
void CSS_UpdateKOStars(int ply, int unk);
void CSS_StartThink(GOBJ *gobj);
int CSS_GetHandicapValue(int ply, int nametag_id);
void CSS_SetModeTexture(int css_kind);
int CSS_ReturnPuck(int ply);
int CSS_SetRandomFighter(int ply, int unk);
void CSS_UpdateCSP(int ply);
int CSS_GetCostumeNum(int ext_id);
int CSS_GetCostumeRed(int c_kind);
int CSS_GetCostumeBlue(int c_kind);
int CSS_GetCostumeGreen(int c_kind);
void CSS_PlayFighterName(int ext_id);
void CSS_CostumeChange(int port, int button_down);
void CSS_UpdateCSPTexture(int port, int costume, int is_none);
#endif
